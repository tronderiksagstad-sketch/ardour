/*
 * Copyright (C) 2026 Trond
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <set>

#include <pango/pangocairo.h>
#include <glibmm/fileutils.h>
#include <glibmm/markup.h>
#include <glibmm/miscutils.h>
#include <ytkmm/adjustment.h>
#include <ytkmm/alignment.h>
#include <ytkmm/separator.h>
#include <ytkmm/scrolledwindow.h>

#include "pbd/xml++.h"

#include "ardour/audioengine.h"
#include "ardour/filesystem_paths.h"
#include "ardour/midi_track.h"
#include "ardour/plugin_insert.h"
#include "ardour/plugin_manager.h"
#include "ardour/presentation_info.h"
#include "ardour/rc_configuration.h"
#include "ardour/route.h"
#include "ardour/session.h"

#include "gtkmm2ext/actions.h"
#include "gtkmm2ext/colors.h"
#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/utils.h"

#include "ardour_message.h"
#include "ardour_ui.h"
#include "gui_thread.h"
#include "mixer_strip.h"
#include "mixer_ui.h"
#include "plugin_browser.h"
#include "plugin_kinds.h"
#include "public_editor.h"
#include "route_time_axis.h"
#include "selection.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/utf8_utils.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using std::string;
using std::vector;

/* ====================================================================== *
 * the kinds
 * ====================================================================== */

static const PluginBrowser::KindInfo kind_table[] = {
	/* effects */
	{ "eq", N_("EQ & Filter"), "EQ", N_("Shapes bass, middle and treble."), N_("Makes bass, middle or treble louder or quieter. Use it to take out boom or add clarity."), 0x6ee7a8, PluginBrowser::Curve, false, "eq equaliser equalizer filter highpass lowpass tone treble bass" },
	{ "comp", N_("Compressor"), "compressor", N_("Evens out loud and quiet parts."), N_("Evens out loud and quiet parts, so a voice or instrument sits steady in the mix."), 0xf2b04d, PluginBrowser::Meter, false, "compressor dynamics squash" },
	{ "limit", N_("Limiter"), "limiter", N_("Stops peaks going over a set level."), N_("Stops the sound going over a set level. Usually last on the master, to make a mix loud without clipping."), 0xf2b04d, PluginBrowser::Meter, false, "limiter maximizer maximiser loudness" },
	{ "gate", N_("Gate"), "gate", N_("Silences the track when it gets quiet."), N_("Silences a track whenever it gets quiet, to hide noise or spill between hits."), 0xf2b04d, PluginBrowser::Meter, false, "gate expander noise" },
	{ "reverb", N_("Reverb"), "reverb", N_("Adds the sound of a room or hall."), N_("Puts the sound of a room or a hall around the track."), 0x6cb6ff, PluginBrowser::Tail, false, "reverb verb room hall space ambience" },
	{ "delay", N_("Delay & Echo"), "echo", N_("Repeats the sound a moment later."), N_("Repeats the sound a moment later."), 0x6cb6ff, PluginBrowser::Echo, false, "delay echo repeat" },
	{ "dist", N_("Distortion & Amp"), "amp", N_("Adds grit and warmth."), N_("Adds grit and warmth: guitar amps, tape, overdrive."), 0xff8a6b, PluginBrowser::Clip, false, "distortion overdrive fuzz amp saturation tape guitar drive" },
	{ "mod", N_("Chorus & Modulation"), "chorus", N_("Makes the sound wider or swirling."), N_("Chorus, flanger, phaser: make a sound wider, thicker or swirling."), 0xc79bff, PluginBrowser::Wave, false, "modulation chorus flanger phaser tremolo" },
	{ "pitch", N_("Pitch & Tuning"), "tuner", N_("Corrects or shifts the pitch."), N_("Corrects out-of-tune notes, or moves the pitch up or down."), 0xc79bff, PluginBrowser::Wave, false, "pitch tune autotune tuning" },
	{ "tools", N_("Stereo & Tools"), "tool", N_("A helper for width, balance or phase."), N_("Width, balance, phase and other helpers."), 0x9aa1ac, PluginBrowser::Wave, false, "stereo width balance phase utility tools" },
	{ "meter", N_("Meters"), "meter", N_("Shows level or loudness. Changes nothing."), N_("Show loudness, level or frequencies. They do not change the sound."), 0xf2b04d, PluginBrowser::Meter, false, "meter analyser analyzer spectrum level" },
	{ "other", N_("Other"), "effect", N_("Not filed under a kind yet."), N_("Plugins that do not say what kind they are."), 0x9aa1ac, PluginBrowser::Wave, false, "other misc" },
	/* instruments */
	{ "synth", N_("Synth"), "synth", N_("Makes sound electronically."), N_("Makes sound electronically: leads, pads, basses."), 0x5fe3e0, PluginBrowser::Wave, true, "synth synthesizer synthesiser pad lead" },
	{ "keys", N_("Keys & Organ"), "keyboard", N_("Piano, electric piano or organ."), N_("Pianos, electric pianos and organs."), 0xf1ead8, PluginBrowser::Keys, true, "piano keys organ keyboard" },
	{ "drums", N_("Drums"), "drum kit", N_("Drum kit or drum machine."), N_("Drum kits and drum machines."), 0xffa24a, PluginBrowser::Pads, true, "drums kit percussion beat" },
	{ "sampler", N_("Sampler"), "sampler", N_("Plays recorded sounds."), N_("Plays recorded sounds: SoundFonts, SFZ and sample libraries."), 0x6ee7a8, PluginBrowser::Sample, true, "sampler soundfont samples" }
};

using namespace PluginKinds; /* FirstEffect ... NKinds, lower(), tokens(), guess_kind(), fit() */

const PluginBrowser::KindInfo*
PluginBrowser::kinds ()
{
	return kind_table;
}

int
PluginBrowser::n_kinds ()
{
	return NKinds;
}

/* ====================================================================== *
 * what a plugin entry says about itself
 * ====================================================================== */

string
PluginBrowser::Entry::description () const
{
	if (!note.empty ()) {
		return note;
	}
	if (!builtin.empty ()) {
		return builtin;
	}
	return _(kind_table[kind].brief);
}

bool
PluginBrowser::Entry::is_new () const
{
	return first_seen > 0 && uses == 0 && (time (0) - first_seen) < 21 * 86400;
}

static unsigned
hash_of (string const& s)
{
	unsigned h = 7;
	for (char c : s) {
		h = h * 33 + (unsigned char) c;
	}
	return h;
}

static uint32_t
hsl (double h, double s, double l)
{
	h = fmod (h, 360.0) / 60.0;
	const double c = (1 - fabs (2 * l - 1)) * s;
	const double x = c * (1 - fabs (fmod (h, 2.0) - 1));
	double r = 0, g = 0, b = 0;
	if (h < 1) { r = c; g = x; } else if (h < 2) { r = x; g = c; } else if (h < 3) { g = c; b = x; }
	else if (h < 4) { g = x; b = c; } else if (h < 5) { r = x; b = c; } else { r = c; b = x; }
	const double m = l - c / 2;
	return ((uint32_t) lrint ((r + m) * 255) << 16) | ((uint32_t) lrint ((g + m) * 255) << 8) | (uint32_t) lrint ((b + m) * 255);
}

static void
skin_for (string const& lmaker, uint32_t& light, uint32_t& dark, uint32_t& ink)
{
	for (const PluginBrowser::Skin* k = PluginBrowser::skins; k->maker; ++k) {
		if (lmaker.find (k->maker) != string::npos) {
			light = k->light;
			dark  = k->dark;
			ink   = k->ink;
			return;
		}
	}
	const double h = hash_of (lmaker.empty () ? string ("x") : lmaker) % 360;
	light = hsl (h, 0.26, 0.32);
	dark  = hsl (h, 0.30, 0.17);
	ink   = 0xe6e8eb;
}

/* ====================================================================== *
 * drawing
 * ====================================================================== */

typedef Cairo::RefPtr<Cairo::Context> Cr;

static const uint32_t col_accent = 0x4c9aff;
static const uint32_t col_panel  = 0x16181c;
static const uint32_t col_bg     = 0x1a1d22;
static const uint32_t col_line   = 0x2a2e36;
static const uint32_t col_sel    = 0x2b3a52;

static void
rgb (Cr const& cr, uint32_t c, double a = 1.0)
{
	cr->set_source_rgba (((c >> 16) & 0xff) / 255.0, ((c >> 8) & 0xff) / 255.0, (c & 0xff) / 255.0, a);
}

static void
rounded (Cr const& cr, double x, double y, double w, double h, double r)
{
	cr->begin_new_sub_path ();
	cr->arc (x + w - r, y + r, r, -M_PI / 2, 0);
	cr->arc (x + w - r, y + h - r, r, 0, M_PI / 2);
	cr->arc (x + r, y + h - r, r, M_PI / 2, M_PI);
	cr->arc (x + r, y + r, r, M_PI, 3 * M_PI / 2);
	cr->close_path ();
}

static Glib::RefPtr<Pango::Layout>
make_layout (Cr const& cr, Gtk::Widget& w, double px, bool bold)
{
	Glib::RefPtr<Pango::Layout> l = Glib::wrap (pango_cairo_create_layout (cr->cobj ()));
	Pango::FontDescription fd = w.get_pango_context ()->get_font_description ();
	fd.set_absolute_size (px * PANGO_SCALE);
	fd.set_weight (bold ? Pango::WEIGHT_BOLD : Pango::WEIGHT_NORMAL);
	l->set_font_description (fd);
	return l;
}

static void
show_text (Cr const& cr, Glib::RefPtr<Pango::Layout> const& l, double x, double y)
{
	cr->move_to (x, y);
	pango_cairo_show_layout (cr->cobj (), l->gobj ());
}

/** text in one line: left aligned at x, or centred about x when `centre` */
static void
text_at (Cr const& cr, Gtk::Widget& w, string const& markup, double px, bool bold, uint32_t colour, double alpha, double x, double y, double width, bool centre)
{
	Glib::RefPtr<Pango::Layout> l = make_layout (cr, w, px, bold);
	l->set_markup (markup);
	l->set_width ((int) (width * PANGO_SCALE));
	l->set_ellipsize (Pango::ELLIPSIZE_END);
	l->set_alignment (centre ? Pango::ALIGN_CENTER : Pango::ALIGN_LEFT);
	rgb (cr, colour, alpha);
	show_text (cr, l, x, y);
}

static void
glyph (Cr const& cr, PluginBrowser::Show kind, double ox, double oy, uint32_t glow)
{
	cr->save ();
	cr->translate (ox, oy);
	cr->set_line_width (2);
	cr->set_line_cap (Cairo::LINE_CAP_ROUND);
	cr->set_line_join (Cairo::LINE_JOIN_ROUND);
	rgb (cr, glow);

	switch (kind) {
	case PluginBrowser::Meter:
		rgb (cr, glow, 0.4);
		cr->begin_new_sub_path ();
		cr->arc (55, 44, 41, M_PI, 2 * M_PI);
		cr->stroke ();
		cr->move_to (20, 30); cr->line_to (24, 33);
		cr->move_to (36, 16); cr->line_to (38.5, 20.5);
		cr->move_to (55, 11); cr->line_to (55, 16);
		cr->move_to (74, 16); cr->line_to (71.5, 20.5);
		cr->move_to (90, 30); cr->line_to (86, 33);
		cr->stroke ();
		rgb (cr, glow);
		cr->move_to (55, 46); cr->line_to (77, 15);
		cr->stroke ();
		break;
	case PluginBrowser::Curve:
		rgb (cr, glow, 0.25);
		cr->set_dash (std::vector<double> { 2, 4 }, 0);
		cr->move_to (4, 30); cr->line_to (106, 30);
		cr->stroke ();
		cr->unset_dash ();
		rgb (cr, glow);
		cr->move_to (4, 30);
		cr->curve_to (18, 30, 24, 12, 36, 13);
		cr->curve_to (48, 14, 52, 40, 64, 34);
		cr->curve_to (76, 28, 86, 16, 106, 22);
		cr->stroke ();
		break;
	case PluginBrowser::Tail: {
		static const double top[] = { 8, 14, 19, 24, 28, 32, 35, 38, 40, 42 };
		for (int i = 0; i < 10; ++i) {
			cr->move_to (10 + i * 10, 44); cr->line_to (10 + i * 10, top[i]);
		}
		cr->stroke ();
		break;
	}
	case PluginBrowser::Echo: {
		static const double top[] = { 10, 20, 29, 37 };
		cr->set_line_width (6);
		cr->set_line_cap (Cairo::LINE_CAP_BUTT);
		for (int i = 0; i < 4; ++i) {
			cr->move_to (14 + i * 26, 44); cr->line_to (14 + i * 26, top[i]);
		}
		cr->stroke ();
		break;
	}
	case PluginBrowser::Clip:
		cr->move_to (4, 26);
		cr->curve_to (10, 26, 12, 12, 18, 12); cr->line_to (30, 12);
		cr->curve_to (36, 12, 38, 40, 44, 40); cr->line_to (56, 40);
		cr->curve_to (62, 40, 64, 12, 70, 12); cr->line_to (82, 12);
		cr->curve_to (88, 12, 90, 40, 96, 40); cr->line_to (106, 40);
		cr->stroke ();
		break;
	case PluginBrowser::Wave:
		cr->move_to (4, 26);
		cr->curve_to (14, 2, 24, 2, 34, 26);
		cr->curve_to (44, 50, 54, 50, 64, 26);
		cr->curve_to (74, 2, 84, 2, 94, 26);
		cr->curve_to (104, 50, 104, 44, 106, 40);
		cr->stroke ();
		break;
	case PluginBrowser::Keys:
		rgb (cr, 0xe9e4d8);
		for (int i = 0; i < 8; ++i) {
			cr->rectangle (6 + i * 12, 8, 10, 36);
		}
		cr->fill ();
		rgb (cr, 0x090b0d);
		for (double x : { 17, 29, 53, 65, 77 }) {
			cr->rectangle (x - 3.5, 8, 7, 22);
		}
		cr->fill ();
		break;
	case PluginBrowser::Pads:
		for (int i = 0; i < 8; ++i) {
			static const double px[] = { 17, 42, 92, 67, 92, 17, 42, 67 };
			static const double py[] = { 16, 16, 38, 16, 16, 38, 38, 38 };
			rgb (cr, glow, i < 3 ? 1.0 : 0.35);
			cr->begin_new_sub_path ();
			cr->arc (px[i], py[i], 8.5, 0, 2 * M_PI);
			cr->fill ();
		}
		break;
	case PluginBrowser::Sample: {
		static const double a[] = { 25, 20, 12, 18, 8, 16, 22, 14, 10, 20, 16, 23, 18, 21, 24, 22 };
		static const double b[] = { 27, 32, 40, 34, 44, 36, 30, 38, 42, 32, 36, 29, 34, 31, 28, 30 };
		for (int i = 0; i < 16; ++i) {
			cr->move_to (8 + i * 6, a[i]); cr->line_to (8 + i * 6, b[i]);
		}
		cr->stroke ();
		break;
	}
	}
	cr->restore ();
}

static void
knob (Cr const& cr, double cx, double cy, double r, double angle_deg)
{
	Cairo::RefPtr<Cairo::RadialGradient> g = Cairo::RadialGradient::create (cx - r * 0.3, cy - r * 0.4, r * 0.1, cx, cy, r);
	g->add_color_stop_rgb (0, 0x70 / 255.0, 0x76 / 255.0, 0x7f / 255.0);
	g->add_color_stop_rgb (0.72, 0x1b / 255.0, 0x1e / 255.0, 0x23 / 255.0);
	cr->begin_new_sub_path ();
	cr->arc (cx, cy, r, 0, 2 * M_PI);
	cr->set_source (g);
	cr->fill ();
	if (r > 8) {
		cr->save ();
		cr->translate (cx, cy);
		cr->rotate (angle_deg * M_PI / 180.0);
		rgb (cr, 0xf4f4f4);
		cr->rectangle (-1, -r + 3, 2, 9);
		cr->fill ();
		cr->restore ();
	}
}

static void
screw (Cr const& cr, double x, double y)
{
	Cairo::RefPtr<Cairo::RadialGradient> g = Cairo::RadialGradient::create (x - 1, y - 1.2, 0.5, x, y, 3.5);
	g->add_color_stop_rgb (0, 0xd8 / 255.0, 0xdb / 255.0, 0xe0 / 255.0);
	g->add_color_stop_rgb (1, 0x4b / 255.0, 0x4f / 255.0, 0x57 / 255.0);
	cr->begin_new_sub_path ();
	cr->arc (x, y, 3.5, 0, 2 * M_PI);
	cr->set_source (g);
	cr->fill ();
}

/* the front panel: gradient in the maker's colour with a fine grain */
static void
panel (Cr const& cr, double x, double y, double w, double h, double r, PluginBrowser::Entry const& e, bool lift)
{
	if (lift) {
		rgb (cr, 0x000000, 0.35);
		rounded (cr, x + 2, y + 6, w - 4, h - 2, r);
		cr->fill ();
	}
	Cairo::RefPtr<Cairo::LinearGradient> g = Cairo::LinearGradient::create (0, y, 0, y + h);
	g->add_color_stop_rgb (0, ((e.light >> 16) & 0xff) / 255.0, ((e.light >> 8) & 0xff) / 255.0, (e.light & 0xff) / 255.0);
	g->add_color_stop_rgb (1, ((e.dark >> 16) & 0xff) / 255.0, ((e.dark >> 8) & 0xff) / 255.0, (e.dark & 0xff) / 255.0);
	rounded (cr, x, y, w, h, r);
	cr->set_source (g);
	cr->fill_preserve ();
	cr->save ();
	cr->clip ();
	cr->set_line_width (1);
	rgb (cr, 0xffffff, 0.04);
	for (double gx = x + 1; gx < x + w; gx += 3) {
		cr->move_to (gx + 0.5, y); cr->line_to (gx + 0.5, y + h);
	}
	cr->stroke ();
	rgb (cr, 0xffffff, 0.24);
	cr->move_to (x, y + 0.5); cr->line_to (x + w, y + 0.5);
	cr->stroke ();
	rgb (cr, 0x000000, 0.55);
	cr->move_to (x, y + h - 0.5); cr->line_to (x + w, y + h - 0.5);
	cr->stroke ();
	cr->restore ();
}

static void
led (Cr const& cr, double x, double y, double r, bool on)
{
	if (on) {
		rgb (cr, 0x6ee7a8, 0.25);
		cr->begin_new_sub_path ();
		cr->arc (x, y, r * 2.2, 0, 2 * M_PI);
		cr->fill ();
		rgb (cr, 0x6ee7a8);
	} else {
		rgb (cr, 0x000000, 0.5);
	}
	cr->begin_new_sub_path ();
	cr->arc (x, y, r, 0, 2 * M_PI);
	cr->fill ();
}

/** the big panel, 304 x 120 */
static void
draw_big (Cr const& cr, Gtk::Widget& w, double x, double y, PluginBrowser::Entry const& e, PluginBrowser::KindInfo const& k, bool frame = true)
{
	const double W = 304, H = 120;
	if (frame) {
		panel (cr, x, y, W, H, 7, e, true);
		screw (cr, x + 9.5, y + 9.5);
		screw (cr, x + W - 9.5, y + 9.5);
		screw (cr, x + 9.5, y + H - 9.5);
		screw (cr, x + W - 9.5, y + H - 9.5);
	}

	string maker = Gtkmm2ext::markup_escape_text (e.maker.empty () ? string (_("Plugin")) : e.maker);
	text_at (cr, w, "<span letter_spacing=\"1400\">" + string (g_utf8_strup (maker.c_str (), -1)) + "</span>", 9, true, e.ink, 0.8, x + 30, y + 8, W - 60, true);

	/* the screen */
	rgb (cr, 0x090b0d);
	rounded (cr, x + 22, y + 30, 110, 52, 4);
	cr->fill_preserve ();
	rgb (cr, 0xffffff, 0.1);
	cr->set_line_width (1);
	cr->stroke ();
	cr->save ();
	rounded (cr, x + 22, y + 30, 110, 52, 4);
	cr->clip ();
	glyph (cr, k.show, x + 22, y + 30, k.glow);
	cr->restore ();

	/* knobs */
	static const double cx[] = { 167, 216, 265 };
	for (int i = 0; i < 3; ++i) {
		knob (cr, x + cx[i], y + 57, 15, (int) ((e.seed >> (i * 3)) % 260) - 130);
	}

	/* the name, engraved */
	string name = Gtkmm2ext::markup_escape_text (e.name);
	text_at (cr, w, name, 14, true, 0x000000, 0.5, x + 30, y + H - 28.5, W - 60, true);
	text_at (cr, w, name, 14, true, e.ink, 1.0, x + 30, y + H - 27, W - 60, true);

	led (cr, x + W - 24, y + H - 20, 3.5, e.uses > 0);
}

/** the small panel, 88 x 40 */
static void
draw_small (Cr const& cr, double x, double y, PluginBrowser::Entry const& e, PluginBrowser::KindInfo const& k)
{
	panel (cr, x, y, 88, 40, 4, e, true);
	rgb (cr, 0x0a0c0e);
	rounded (cr, x + 8, y + 11, 24, 18, 2);
	cr->fill_preserve ();
	rgb (cr, 0xffffff, 0.08);
	cr->set_line_width (1);
	cr->stroke ();
	rgb (cr, k.glow);
	cr->rectangle (x + 12.5, y + 19, 15, 2);
	cr->fill ();
	const int n = 2 + (e.seed % 2);
	for (int i = 0; i < n; ++i) {
		knob (cr, x + 42 + i * 15, y + 20, 5, 0);
	}
	led (cr, x + 88 - 8, y + 7, 2, e.uses > 0);
}

/* ====================================================================== *
 * the list
 * ====================================================================== */

class PluginList : public Gtk::DrawingArea
{
public:
	PluginList (PluginBrowser& b)
		: _b (b)
		, _adj (0, 0, 100, 20, 200, 100)
		, _total (0)
	{
		add_events (Gdk::BUTTON_PRESS_MASK | Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
		_adj.signal_value_changed ().connect ([this] () { queue_draw (); });
		set_size_request (scaled (560), scaled (300));
	}

	Gtk::Adjustment& adjustment () { return _adj; }

	double scaled (double v) const { return v * UIConfiguration::instance ().get_ui_scale (); }
	double s () const { return UIConfiguration::instance ().get_ui_scale (); }

	void rows_changed ()
	{
		_y.clear ();
		double y = 0;
		for (PluginBrowser::Row const& r : _b._rows) {
			_y.push_back (y);
			y += r.header ? 40 : 58;
		}
		_total = y;
		update_adjustment ();
		_adj.set_value (0);
		queue_draw ();
	}

	void ensure_visible (int row)
	{
		if (row < 0 || row >= (int) _y.size ()) {
			return;
		}
		const double h = _b._rows[row].header ? 40 : 58;
		double       v = _adj.get_value ();
		const double page = _adj.get_page_size ();
		/* show the section header above the first plugin of a section */
		double top = _y[row];
		if (row > 0 && _b._rows[row - 1].header) {
			top = _y[row - 1];
		}
		if (top < v) {
			v = top;
		} else if (_y[row] + h > v + page) {
			v = _y[row] + h - page;
		}
		_adj.set_value (std::max (0.0, std::min (v, std::max (0.0, _total - page))));
	}

protected:
	void on_size_allocate (Gtk::Allocation& a)
	{
		Gtk::DrawingArea::on_size_allocate (a);
		update_adjustment ();
	}

	bool on_expose_event (GdkEventExpose* ev)
	{
		Cr cr = get_window ()->create_cairo_context ();
		cr->rectangle (ev->area.x, ev->area.y, ev->area.width, ev->area.height);
		cr->clip ();
		rgb (cr, col_bg);
		cr->paint ();
		cr->scale (s (), s ());

		const double W   = get_allocation ().get_width () / s ();
		const double top = _adj.get_value ();
		const double H   = get_allocation ().get_height () / s ();

		if (_b._rows.empty ()) {
			Glib::RefPtr<Pango::Layout> l = make_layout (cr, *this, 15, false);
			l->set_text (string_compose (_("Nothing like “%1”.\nTry the kind of thing instead: “reverb”, “echo”, “piano”."), std::string (_b._search.get_text ())));
			l->set_width ((int) ((W - 60) * PANGO_SCALE));
			l->set_wrap (Pango::WRAP_WORD_CHAR);
			l->set_alignment (Pango::ALIGN_CENTER);
			rgb (cr, 0x9aa1ac);
			show_text (cr, l, 30, 70);
			return true;
		}

		for (size_t i = 0; i < _y.size (); ++i) {
			const double h = _b._rows[i].header ? 40 : 58;
			if (_y[i] + h < top || _y[i] > top + H) {
				continue;
			}
			cr->save ();
			cr->translate (0, _y[i] - top);
			if (_b._rows[i].header) {
				draw_header (cr, _b._rows[i], W);
			} else {
				draw_item (cr, _b._rows[i], (int) i == _b._sel, W);
			}
			cr->restore ();
		}
		return true;
	}

	bool on_motion_notify_event (GdkEventMotion* ev)
	{
		const int r = row_at (ev->y);
		if (r >= 0 && !_b._rows[r].header && r != _b._sel) {
			_b.select (r);
		}
		return true;
	}

	bool on_button_press_event (GdkEventButton* ev)
	{
		if (ev->button != 1) {
			return false;
		}
		const int r = row_at (ev->y);
		if (r < 0 || _b._rows[r].header) {
			return true;
		}
		const double W = get_allocation ().get_width () / s ();
		if (r != _b._sel) {
			_b.select (r);
		}
		if (ev->x / s () > W - 44) {
			_b.toggle_star (_b._rows[r].entry);
		} else {
			_b.add_entry (_b._rows[r].entry);
		}
		return true;
	}

	bool on_scroll_event (GdkEventScroll* ev)
	{
		double v = _adj.get_value ();
		if (ev->direction == GDK_SCROLL_UP) {
			v -= 58;
		} else if (ev->direction == GDK_SCROLL_DOWN) {
			v += 58;
		}
		_adj.set_value (std::max (0.0, std::min (v, std::max (0.0, _total - _adj.get_page_size ()))));
		return true;
	}

private:
	PluginBrowser&                _b;
	Gtk::Adjustment               _adj;
	std::vector<double>           _y;
	double                        _total;

	void update_adjustment ()
	{
		const double page = get_allocation ().get_height () / s ();
		_adj.set_lower (0);
		_adj.set_upper (std::max (_total, page));
		_adj.set_page_size (page);
		_adj.set_page_increment (page * 0.9);
		_adj.set_step_increment (58);
	}

	int row_at (double ydev) const
	{
		const double y = ydev / s () + _adj.get_value ();
		for (size_t i = 0; i < _y.size (); ++i) {
			const double h = _b._rows[i].header ? 40 : 58;
			if (y >= _y[i] && y < _y[i] + h) {
				return (int) i;
			}
		}
		return -1;
	}

	void draw_header (Cr const& cr, PluginBrowser::Row const& r, double W)
	{
		Glib::RefPtr<Pango::Layout> l = make_layout (cr, *this, 11, true);
		string m = "<span letter_spacing=\"700\">" + Gtkmm2ext::markup_escape_text (r.group) + "</span>";
		if (!r.note.empty ()) {
			m += "  <span weight=\"normal\" letter_spacing=\"0\" foreground=\"#6b7380\">" + Gtkmm2ext::markup_escape_text (r.note) + "</span>";
		}
		l->set_markup (m);
		int tw, th;
		l->get_pixel_size (tw, th);
		const double cx = W / 2;
		rgb (cr, 0x9aa1ac);
		show_text (cr, l, cx - tw / 2.0, 26 - th / 2.0);
		rgb (cr, col_line);
		cr->set_line_width (1);
		cr->move_to (14, 26.5); cr->line_to (cx - tw / 2.0 - 12, 26.5);
		cr->move_to (cx + tw / 2.0 + 12, 26.5); cr->line_to (W - 14, 26.5);
		cr->stroke ();
	}

	void draw_item (Cr const& cr, PluginBrowser::Row const& r, bool selected, double W)
	{
		PluginBrowser::Entry const&    e = _b._entries[r.entry];
		PluginBrowser::KindInfo const& k = kind_table[e.kind];

		if (selected) {
			rgb (cr, col_sel);
			rounded (cr, 5, 1, W - 10, 56, 9);
			cr->fill_preserve ();
			rgb (cr, col_accent);
			cr->set_line_width (1);
			cr->stroke ();
		}
		draw_small (cr, 14, 9, e, k);

		/* the right edge: star, then a badge */
		double right = W - 14;
		{
			Glib::RefPtr<Pango::Layout> l = make_layout (cr, *this, 17, false);
			l->set_text ("\xE2\x98\x85");
			rgb (cr, e.favourite ? 0xe5c84a : 0x3d434d);
			show_text (cr, l, W - 36, 18);
			right = W - 44;
		}
		if (!r.badge.empty ()) {
			Glib::RefPtr<Pango::Layout> l = make_layout (cr, *this, 9.5, true);
			l->set_markup ("<span letter_spacing=\"600\">" + r.badge + "</span>");
			int tw, th;
			l->get_pixel_size (tw, th);
			const double bw = tw + 14, bx = right - bw - 4;
			rounded (cr, bx, 19, bw, 20, 5);
			if (r.badge == "NEW") {
				rgb (cr, 0x3ccf8e);
				cr->fill ();
				rgb (cr, 0x0d1117);
			} else {
				rgb (cr, col_accent);
				cr->set_line_width (1);
				cr->stroke ();
				rgb (cr, col_accent);
			}
			show_text (cr, l, bx + 7, 29 - th / 2.0);
			right = bx - 8;
		}

		/* name, maker, then what it is */
		const double tx = 118;
		string markup;
		{
			string name = e.name;
			string out;
			size_t pos = 0;
			vector<std::pair<int,int> > hits = e.hits;
			if (e.name.size () != e.lname.size ()) {
				hits.clear (); /* lower-casing changed the length: places would be off */
			}
			std::sort (hits.begin (), hits.end ());
			for (auto const& h : hits) {
				if (h.first < (int) pos || h.first + h.second > (int) name.size ()) {
					continue;
				}
				out += Gtkmm2ext::markup_escape_text (name.substr (pos, h.first - pos));
				out += "<span foreground=\"#4c9aff\">" + Gtkmm2ext::markup_escape_text (name.substr (h.first, h.second)) + "</span>";
				pos = h.first + h.second;
			}
			out += Gtkmm2ext::markup_escape_text (name.substr (pos));
			markup = out;
		}
		if (!e.maker.empty ()) {
			markup += "  <span weight=\"normal\" size=\"smaller\" foreground=\"#8b93a0\">" + Gtkmm2ext::markup_escape_text (e.maker) + "</span>";
		}
		text_at (cr, *this, markup, 15, true, 0xe6e8eb, 1.0, tx, 10, right - tx, false);

		string d = e.description ();
		text_at (cr, *this, e.note.empty () ? Gtkmm2ext::markup_escape_text (d) : "\xE2\x9C\x8E " + Gtkmm2ext::markup_escape_text (d), 12.5, false, e.note.empty () ? 0x8f97a3 : 0xc9d6ea, 1.0, tx, 33, right - tx, false);
	}
};

/* ====================================================================== *
 * the plugin you are pointing at, picked up
 * ====================================================================== */

class PluginPlate : public Gtk::DrawingArea
{
public:
	/** one step up (-1) or down (+1) the list: the wheel, or a click on a neighbour */
	sigc::signal<void, int> Step;

	PluginPlate () : _target (0), _pos (0), _vel (0), _compact (false)
	{
		add_events (Gdk::BUTTON_PRESS_MASK | Gdk::SCROLL_MASK);
		apply_size ();
	}

	~PluginPlate () { _timer.disconnect (); }

	/** the units in the rack, and which one is in the middle. A new list is put
	 * in place at once; moving within the same list slides. */
	void set_rack (std::vector<PluginBrowser::Entry const*> const& list, int index)
	{
		const bool same = list == _list;
		_list           = list;
		_target         = index;
		if (!same || index < 0) {
			_pos = std::max (0, index);
			_vel = 0;
			queue_draw ();
		} else if (!_timer.connected ()) {
			_last   = g_get_monotonic_time ();
			_timer  = Glib::signal_timeout ().connect (sigc::mem_fun (*this, &PluginPlate::tick), 16);
		}
	}

	/** while notes are written only the middle unit is shown, so the text box fits */
	void set_compact (bool yn)
	{
		if (yn != _compact) {
			_compact = yn;
			apply_size ();
			queue_draw ();
		}
	}

protected:
	static const int PH = 120, PITCH = 128;

	void apply_size ()
	{
		const float s = UIConfiguration::instance ().get_ui_scale ();
		set_size_request (rintf (350 * s), rintf ((_compact ? 150 : 224) * s));
	}

	/* a spring with a very small overshoot: a heavy unit settling into its slot */
	bool tick ()
	{
		const gint64 now = g_get_monotonic_time ();
		double       dt  = std::min (0.05, (now - _last) / 1e6);
		_last            = now;
		const double k = 260, c = 24;
		_vel += (k * (_target - _pos) - c * _vel) * dt;
		_pos += _vel * dt;
		queue_draw ();
		if (fabs (_target - _pos) < 0.002 && fabs (_vel) < 0.02) {
			_pos = _target;
			_vel = 0;
			return false;
		}
		return true;
	}

	double centre_y (double H) const { return (H - PH) / 2; }

	bool on_scroll_event (GdkEventScroll* ev)
	{
		const gint64 now = g_get_monotonic_time ();
		if (now - _wheel_at < 110000) {
			return true;
		}
		_wheel_at = now;
		if (ev->direction == GDK_SCROLL_DOWN) {
			Step (1);
		} else if (ev->direction == GDK_SCROLL_UP) {
			Step (-1);
		}
		return true;
	}

	bool on_button_press_event (GdkEventButton* ev)
	{
		if (ev->button != 1 || _compact) {
			return false;
		}
		const float  s = UIConfiguration::instance ().get_ui_scale ();
		const double H = get_allocation ().get_height () / s;
		const double y = ev->y / s;
		const double top = centre_y (H);
		if (y < top - 2) {
			Step (-1);
		} else if (y > top + PH + 2) {
			Step (1);
		}
		return true;
	}

	void blank (Cr const& cr, double W, double y)
	{
		Cairo::RefPtr<Cairo::LinearGradient> g = Cairo::LinearGradient::create (0, y, 0, y + PH);
		g->add_color_stop_rgb (0, 0x42 / 255.0, 0x46 / 255.0, 0x4e / 255.0);
		g->add_color_stop_rgb (1, 0x26 / 255.0, 0x2a / 255.0, 0x30 / 255.0);
		cr->rectangle (0, y, W, PH);
		cr->set_source (g);
		cr->fill ();
		for (int i = 0; i < 5; ++i) {
			rgb (cr, 0x0b0c0e);
			rounded (cr, 70, y + 34 + i * 11, W - 140, 4, 2);
			cr->fill ();
		}
		ears (cr, W, y);
	}

	void ears (Cr const& cr, double W, double y)
	{
		rgb (cr, 0x000000, 0.45);
		cr->rectangle (22, y, 1, PH);
		cr->rectangle (W - 23, y, 1, PH);
		cr->fill ();
		rgb (cr, 0xffffff, 0.12);
		cr->rectangle (23, y, 1, PH);
		cr->rectangle (W - 24, y, 1, PH);
		cr->fill ();
		for (double sy : { 14.5, 105.5 }) {
			screw (cr, 11.5, y + sy);
			screw (cr, W - 11.5, y + sy);
		}
	}

	bool on_expose_event (GdkEventExpose*)
	{
		Cr cr = get_window ()->create_cairo_context ();
		const float  s = UIConfiguration::instance ().get_ui_scale ();
		const double W = get_allocation ().get_width () / s, H = get_allocation ().get_height () / s;
		cr->scale (s, s);
		cr->rectangle (0, 0, W, H);
		cr->clip ();

		/* the inside of the rack */
		Cairo::RefPtr<Cairo::LinearGradient> bg = Cairo::LinearGradient::create (0, 0, 0, H);
		bg->add_color_stop_rgb (0, 0x0b / 255.0, 0x0c / 255.0, 0x0e / 255.0);
		bg->add_color_stop_rgb (0.5, 0x10 / 255.0, 0x12 / 255.0, 0x16 / 255.0);
		bg->add_color_stop_rgb (1, 0x0b / 255.0, 0x0c / 255.0, 0x0e / 255.0);
		cr->rectangle (0, 0, W, H);
		cr->set_source (bg);
		cr->fill ();
		for (double rx : { 0.0, W - 22 }) {
			Cairo::RefPtr<Cairo::LinearGradient> r = Cairo::LinearGradient::create (rx, 0, rx + 22, 0);
			r->add_color_stop_rgb (rx == 0 ? 0 : 1, 0x0c / 255.0, 0x0d / 255.0, 0x0f / 255.0);
			r->add_color_stop_rgb (rx == 0 ? 1 : 0, 0x1d / 255.0, 0x20 / 255.0, 0x25 / 255.0);
			cr->rectangle (rx, 0, 22, H);
			cr->set_source (r);
			cr->fill ();
		}

		const int    n   = (int) _list.size ();
		const double top = centre_y (H);
		for (int pass = 2; pass >= 0; --pass) {
			/* the far ones first, the one in the middle last */
			for (int i = (int) floor (_pos) - 2; i <= (int) ceil (_pos) + 2; ++i) {
				const double d = i - _pos;
				if ((int) std::min (2.0, floor (fabs (d) + 0.5)) != pass) {
					continue;
				}
				if (i < -1 || i > n) {
					continue;
				}
				if (_compact && fabs (d) > 0.02) {
					continue;
				}
				const double y = top + d * PITCH;
				if (y > H || y + PH < 0) {
					continue;
				}
				if (i < 0 || i >= n) {
					blank (cr, W, y);
				} else {
					PluginBrowser::Entry const& e = *_list[i];
					panel (cr, 0, y, W, PH, 0, e, true);
					ears (cr, W, y);
					draw_big (cr, *this, (W - 304) / 2, y, e, kind_table[e.kind], false);
				}
				/* neighbours are dimmer */
				const double a = fabs (d);
				const double dark = a < 1 ? 0.38 * a : std::min (0.7, 0.38 + 0.32 * (a - 1));
				rgb (cr, 0x000000, dark);
				cr->rectangle (0, y, W, PH);
				cr->fill ();
			}
		}

		/* soft edges, as if the rack went on above and below */
		for (int up = 0; up < 2; ++up) {
			Cairo::RefPtr<Cairo::LinearGradient> f = Cairo::LinearGradient::create (0, up ? H - 30 : 0, 0, up ? H : 30);
			f->add_color_stop_rgba (up ? 1 : 0, 0x0b / 255.0, 0x0c / 255.0, 0x0e / 255.0, 0.8);
			f->add_color_stop_rgba (up ? 0 : 1, 0x0b / 255.0, 0x0c / 255.0, 0x0e / 255.0, 0);
			cr->rectangle (0, up ? H - 30 : 0, W, 30);
			cr->set_source (f);
			cr->fill ();
		}
		rgb (cr, col_line);
		cr->rectangle (0, H - 1, W, 1);
		cr->fill ();
		return true;
	}

private:
	std::vector<PluginBrowser::Entry const*> _list;
	int            _target;
	double         _pos, _vel;
	bool           _compact;
	gint64         _last = 0;
	gint64         _wheel_at = 0;
	sigc::connection _timer;
};

/* ====================================================================== *
 * the list of tracks to add to
 * ====================================================================== */

/** A small list that opens under the "Add to" button when the pointer rests
 * on it. It shows eight tracks at a time; holding the pointer near its top
 * or bottom edge scrolls it, faster the nearer the edge, so a long list
 * needs neither a click to open nor the wheel to reach the end. A click
 * picks a track; leaving the list closes it.
 */
class TrackPicker : public Gtk::Window
{
public:
	struct Item {
		std::weak_ptr<Route> route;
		string               name;
		string               what;
		uint32_t             colour;
		bool                 current;
	};

	TrackPicker ()
		: Gtk::Window (Gtk::WINDOW_POPUP)
		, _scroll (0)
		, _vel (0)
		, _hover (-1)
		, _py (-1)
	{
		_area.add_events (Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::SCROLL_MASK);
		_area.signal_expose_event ().connect (sigc::mem_fun (*this, &TrackPicker::expose));
		_area.signal_motion_notify_event ().connect (sigc::mem_fun (*this, &TrackPicker::motion));
		_area.signal_button_press_event ().connect (sigc::mem_fun (*this, &TrackPicker::press));
		_area.signal_scroll_event ().connect (sigc::mem_fun (*this, &TrackPicker::wheel));
		_area.signal_enter_notify_event ().connect ([this] (GdkEventCrossing*) { Entered (); return false; });
		_area.signal_leave_notify_event ().connect ([this] (GdkEventCrossing*) {
			_vel   = 0;
			_hover = -1;
			_py    = -1;
			_area.queue_draw ();
			Left ();
			return false;
		});
		add (_area);
		_area.show ();
	}

	sigc::signal<void, std::shared_ptr<Route> > Picked;
	sigc::signal<void>                          Entered;
	sigc::signal<void>                          Left;

	/** show the list with its top edge at (x_centre, y_top), in screen pixels */
	void open (vector<Item> const& items, int x_centre, int y_top)
	{
		_items = items;
		_vel   = 0;
		_hover = -1;
		_py    = -1;
		/* open with the current track in view */
		_scroll = 0;
		for (size_t i = 0; i < _items.size (); ++i) {
			if (_items[i].current) {
				_scroll = i * Row - view_h () / 2 + Row / 2;
			}
		}
		clamp ();

		const double s = scale ();
		const int w = (int) rint (Width * s), h = (int) rint ((ViewTop + view_h () + PadBottom) * s);
		_area.set_size_request (w, h);
		resize (w, h);
		Glib::RefPtr<Gdk::Screen> sc = Gdk::Screen::get_default ();
		int x = x_centre - w / 2;
		x = std::max (0, std::min (x, sc->get_width () - w));
		move (x, std::max (0, std::min (y_top, sc->get_height () - h)));
		show ();
		_area.queue_draw ();
		if (!_tick.connected ()) {
			_tick = Glib::signal_timeout ().connect (sigc::mem_fun (*this, &TrackPicker::tick), 30);
		}
	}

	void close ()
	{
		_tick.disconnect ();
		hide ();
	}

private:
	/* sizes before UI scaling */
	enum { Width = 310, ViewTop = 48, Row = 40, Visible = 8, PadBottom = 8 };

	Gtk::DrawingArea _area;
	vector<Item>     _items;
	double           _scroll;
	double           _vel;   ///< pixels per tick
	int              _hover;
	double           _py;    ///< pointer height in the list, or -1
	sigc::connection _tick;

	double scale () const { return UIConfiguration::instance ().get_ui_scale (); }
	double view_h () const { return std::min<size_t> (_items.size (), Visible) * Row - 2.0; }
	double max_scroll () const { return std::max (0.0, _items.size () * Row - 2.0 - view_h ()); }
	void   clamp () { _scroll = std::max (0.0, std::min (_scroll, max_scroll ())); }

	int row_at (double y) const
	{
		if (y < ViewTop || y >= ViewTop + view_h ()) {
			return -1;
		}
		const int r = (int) floor ((y - ViewTop + _scroll) / Row);
		return (r >= 0 && r < (int) _items.size ()) ? r : -1;
	}

	bool tick ()
	{
		if (_vel != 0) {
			const double before = _scroll;
			_scroll += _vel;
			clamp ();
			if (_scroll != before) {
				/* the list moved under a pointer that stayed still */
				_hover = _py >= 0 ? row_at (_py) : -1;
				_area.queue_draw ();
			}
		}
		return true;
	}

	bool motion (GdkEventMotion* ev)
	{
		_py = ev->y / scale ();
		const double f    = (_py - ViewTop) / view_h ();
		const double zone = 0.28;
		if (max_scroll () <= 0 || f < 0 || f > 1) {
			_vel = 0;
		} else if (f < zone) {
			_vel = -std::min (1.0, (zone - f) / zone) * 16;
		} else if (f > 1 - zone) {
			_vel = std::min (1.0, (f - (1 - zone)) / zone) * 16;
		} else {
			_vel = 0;
		}
		const int h = row_at (_py);
		if (h != _hover) {
			_hover = h;
			_area.queue_draw ();
		}
		return true;
	}

	bool wheel (GdkEventScroll* ev)
	{
		_scroll += (ev->direction == GDK_SCROLL_UP) ? -Row : (ev->direction == GDK_SCROLL_DOWN) ? Row : 0;
		clamp ();
		_hover = _py >= 0 ? row_at (_py) : -1;
		_area.queue_draw ();
		return true;
	}

	bool press (GdkEventButton* ev)
	{
		if (ev->button != 1) {
			return false;
		}
		const int r = row_at (ev->y / scale ());
		if (r >= 0) {
			std::shared_ptr<Route> route = _items[r].route.lock ();
			close ();
			if (route) {
				Picked (route);
			}
		}
		return true;
	}

	bool expose (GdkEventExpose*)
	{
		Cr cr = _area.get_window ()->create_cairo_context ();
		const double s = scale ();
		cr->scale (s, s);
		const double W = Width, VH = view_h (), H = ViewTop + VH + PadBottom;

		rgb (cr, 0x1e2127);
		cr->paint ();
		rgb (cr, 0x3a404a);
		cr->set_line_width (1);
		cr->rectangle (0.5, 0.5, W - 1, H - 1);
		cr->stroke ();

		text_at (cr, _area, string ("<span letter_spacing=\"700\">") + Gtkmm2ext::markup_escape_text (_("ADD TO WHICH TRACK?")) + "</span>", 11, true, 0x9aa1ac, 1.0, 8, 10, W - 16, true);
		if (max_scroll () > 0) {
			text_at (cr, _area, Gtkmm2ext::markup_escape_text (_("Hold near the top or bottom to scroll")), 11.5, false, 0x6b7380, 1.0, 8, 27, W - 16, true);
		}

		cr->save ();
		cr->rectangle (0, ViewTop, W, VH);
		cr->clip ();
		for (size_t i = 0; i < _items.size (); ++i) {
			const double y = ViewTop + i * Row - _scroll;
			if (y + Row < ViewTop || y > ViewTop + VH) {
				continue;
			}
			Item const& it = _items[i];
			if (it.current || (int) i == _hover) {
				rgb (cr, it.current ? col_sel : 0x2a2f37);
				rounded (cr, 8, y, W - 16, Row - 2, 8);
				cr->fill ();
			}
			rgb (cr, it.colour);
			cr->begin_new_sub_path ();
			cr->arc (23, y + 19, 5, 0, 2 * M_PI);
			cr->fill ();

			double right = W - 18;
			if (it.current) {
				Glib::RefPtr<Pango::Layout> l = make_layout (cr, _area, 13, true);
				l->set_text ("\xE2\x9C\x93");
				int tw, th;
				l->get_pixel_size (tw, th);
				rgb (cr, col_accent);
				show_text (cr, l, right - tw, y + 19 - th / 2.0);
				right -= tw + 8;
			}
			{
				Glib::RefPtr<Pango::Layout> l = make_layout (cr, _area, 12, false);
				l->set_text (it.what);
				int tw, th;
				l->get_pixel_size (tw, th);
				rgb (cr, 0x8b93a0);
				show_text (cr, l, right - tw, y + 19 - th / 2.0);
				right -= tw + 10;
			}
			{
				Glib::RefPtr<Pango::Layout> l = make_layout (cr, _area, 14, true);
				l->set_text (it.name);
				l->set_width ((int) ((right - 38) * PANGO_SCALE));
				l->set_ellipsize (Pango::ELLIPSIZE_END);
				int tw, th;
				l->get_pixel_size (tw, th);
				rgb (cr, 0xe6e8eb);
				show_text (cr, l, 38, y + 19 - th / 2.0);
			}
		}

		/* more above, more below */
		for (int top = 1; top >= 0; --top) {
			if (top ? _scroll <= 2 : _scroll >= max_scroll () - 2) {
				continue;
			}
			const double y0 = top ? ViewTop : ViewTop + VH - 26;
			Cairo::RefPtr<Cairo::LinearGradient> g = Cairo::LinearGradient::create (0, y0, 0, y0 + 26);
			const double r = 0x1e / 255.0, gr = 0x21 / 255.0, b = 0x27 / 255.0;
			g->add_color_stop_rgba (top ? 0.2 : 0.0, r, gr, b, top ? 1.0 : 0.0);
			g->add_color_stop_rgba (top ? 1.0 : 0.8, r, gr, b, top ? 0.0 : 1.0);
			cr->rectangle (0, y0, W, 26);
			cr->set_source (g);
			cr->fill ();
			rgb (cr, 0x9aa1ac);
			const double cx = W / 2, ay = top ? y0 + 4 : y0 + 22;
			cr->move_to (cx - 5, ay + (top ? 5 : -5));
			cr->line_to (cx + 5, ay + (top ? 5 : -5));
			cr->line_to (cx, ay);
			cr->close_path ();
			cr->fill ();
		}
		cr->restore ();
		return true;
	}
};

/* ====================================================================== *
 * the browser
 * ====================================================================== */

static PluginBrowser* the_browser = 0;

void
PluginBrowser::show_for (std::shared_ptr<Route> route, int placement)
{
	if (!ARDOUR_UI::instance ()->the_session ()) {
		return;
	}
	if (!the_browser) {
		the_browser = new PluginBrowser ();
	}
	if (!route) {
		for (TimeAxisView* tav : ARDOUR_UI::instance ()->the_editor ().get_selection ().tracks) {
			if (RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (tav)) {
				if (rtv->route () && !rtv->route ()->is_master () && !rtv->route ()->is_monitor ()) {
					route = rtv->route ();
					break;
				}
			}
		}
	}
	the_browser->_origin    = route;
	the_browser->_placement = placement;
	if (the_browser->is_visible ()) {
		/* already open: just turn to the new track */
		if (route) {
			the_browser->set_target (route);
		}
		the_browser->present ();
		return;
	}
	the_browser->_target.reset ();
	if (route) {
		the_browser->_target = route;
	}
	the_browser->present ();
}

void
PluginBrowser::show_for_selected ()
{
	show_for (std::shared_ptr<Route> (), -1);
}

PluginBrowser::PluginBrowser ()
	: Gtk::Window (Gtk::WINDOW_TOPLEVEL)
	, _top (0)
	, _list (0)
	, _scroll (0)
	, _plate (0)
	, _rack (0)
	, _right (0)
	, _picker (0)
	, _loaded (false)
	, _placement (-1)
	, _instruments (false)
	, _filter (-1)
	, _sel (-1)
	, _editing (false)
	, _rack_press (-1)
	, _rack_press_x (0)
	, _rack_grab_dx (0)
	, _rack_dragging (false)
	, _rack_drag_x (0)
{
	set_title (_("Add plugin"));
	set_decorated (false);
	set_skip_taskbar_hint (true);
	set_type_hint (Gdk::WINDOW_TYPE_HINT_DIALOG);
	set_transient_for (ARDOUR_UI::instance ()->main_window ());
	set_border_width (1);

	const float scale = UIConfiguration::instance ().get_ui_scale ();
	auto px = [scale] (double v) { return (int) rintf (v * scale); };
	auto grey = [] (Gtk::Widget& w, const char* c) { w.modify_fg (Gtk::STATE_NORMAL, Gdk::Color (c)); };
	auto bg = [] (Gtk::Widget& w, const char* c) { w.modify_bg (Gtk::STATE_NORMAL, Gdk::Color (c)); };

	/* ---- top: one thing centred over each section ---- */
	_tab_fx.set_text (_("Effects"));
	_tab_inst.set_text (_("Instruments"));
	for (ArdourWidgets::ArdourButton* b : { &_tab_fx, &_tab_inst }) {
		b->set_name ("tab button");
		b->set_size_request (px (116), px (30));
	}
	_tab_fx.signal_clicked.connect ([this] () { set_tab (false); });
	_tab_inst.signal_clicked.connect ([this] () { set_tab (true); });
	Gtk::HBox* tabs = manage (new Gtk::HBox (true, 2));
	tabs->pack_start (_tab_fx);
	tabs->pack_start (_tab_inst);
	Gtk::Alignment* tabs_c = manage (new Gtk::Alignment (0.5, 0.5, 0, 0));
	tabs_c->add (*tabs);
	tabs_c->set_size_request (px (350), px (68));

	_search.set_size_request (px (460), px (42));
	_search.set_alignment (0.5);
	{
		Pango::FontDescription fd = UIConfiguration::instance ().get_NormalFont ();
		fd.set_size (rintf (fd.get_size () * 1.5));
		_search.modify_font (fd);
	}
	_search.signal_changed ().connect (sigc::mem_fun (*this, &PluginBrowser::search_changed));
	/* GTK entries have no placeholder: draw one on the text area while it is empty */
	_search.signal_expose_event ().connect ([this] (GdkEventExpose* ev) {
		GdkWindow* tw = gtk_entry_get_text_window (_search.gobj ());
		if (ev->window != tw || !_search.get_text ().empty ()) {
			return false;
		}
		Glib::RefPtr<Gdk::Window> win = Glib::RefPtr<Gdk::Window>::cast_static (Glib::wrap (tw, true));
		Cr cr = win->create_cairo_context ();
		Glib::RefPtr<Pango::Layout> l = Glib::wrap (pango_cairo_create_layout (cr->cobj ()));
		l->set_font_description (_search.get_pango_context ()->get_font_description ());
		l->set_text (_instruments ? _("Search my instruments") : _("Search my effects"));
		int tw_, th_, ww, wh;
		l->get_pixel_size (tw_, th_);
		win->get_size (ww, wh);
		rgb (cr, 0x7d8590);
		show_text (cr, l, (ww - tw_) / 2.0, (wh - th_) / 2.0);
		return false;
	}, true);
	_search.signal_key_press_event ().connect (sigc::mem_fun (*this, &PluginBrowser::search_key), false);
	Gtk::Alignment* search_c = manage (new Gtk::Alignment (0.5, 0.5, 0, 0));
	search_c->add (_search);

	_chip.set_name ("generic button");
	_chip.set_size_request (px (230), px (36));
	_chip.signal_clicked.connect ([this] () {
		_picker_open_timer.disconnect ();
		if (_picker && _picker->is_visible ()) {
			close_picker ();
		} else {
			choose_target ();
		}
	});
	/* resting the pointer on it opens the list: no click needed */
	_chip.signal_enter_notify_event ().connect ([this] (GdkEventCrossing*) {
		_picker_close_timer.disconnect ();
		if (!(_picker && _picker->is_visible ()) && !_picker_open_timer.connected ()) {
			_picker_open_timer = Glib::signal_timeout ().connect ([this] () { choose_target (); return false; }, 150);
		}
		return false;
	}, false);
	_chip.signal_leave_notify_event ().connect ([this] (GdkEventCrossing*) {
		_picker_open_timer.disconnect ();
		picker_left ();
		return false;
	}, false);
	Gtk::Alignment* chip_c = manage (new Gtk::Alignment (0.5, 0.5, 0, 0));
	chip_c->add (_chip);
	chip_c->set_size_request (px (350), px (68));

	Gtk::HBox* top_row = manage (new Gtk::HBox);
	top_row->pack_start (*tabs_c, false, false);
	top_row->pack_start (*search_c, true, true);
	top_row->pack_start (*chip_c, false, false);

	/* ---- left: kinds ---- */
	_tile_all.set_name ("tab button");
	_tile_fav.set_name ("tab button");
	_tile_new.set_name ("tab button");
	_tile_fav.set_text (string ("\xE2\x98\x85 ") + _("Favourites"));
	_tile_new.set_text (_("New"));
	_tile_all.signal_clicked.connect ([this] () { set_filter (-1); });
	_tile_fav.signal_clicked.connect ([this] () { set_filter (-2); });
	_tile_new.signal_clicked.connect ([this] () { set_filter (-3); });
	auto hover = [this] (ArdourWidgets::ArdourButton& b, int filter) {
		b.signal_enter_notify_event ().connect ([this, filter] (GdkEventCrossing*) { set_foot (tile_tip (filter)); return false; }, false);
		b.signal_leave_notify_event ().connect ([this] (GdkEventCrossing*) { set_foot (""); return false; }, false);
	};
	hover (_tile_all, -1);
	hover (_tile_fav, -2);
	hover (_tile_new, -3);
	for (int i = 0; i < NKinds; ++i) {
		ArdourWidgets::ArdourButton* b = new ArdourWidgets::ArdourButton;
		b->set_name ("tab button");
		b->set_text (_(kind_table[i].label));
		b->signal_clicked.connect ([this, i] () { set_filter (i); });
		hover (*b, i);
		_tile_kind.push_back (b);
	}
	for (ArdourWidgets::ArdourButton* b : { &_tile_all, &_tile_fav, &_tile_new }) {
		b->set_size_request (-1, px (44));
	}
	for (ArdourWidgets::ArdourButton* b : _tile_kind) {
		b->set_size_request (-1, px (44));
	}
	_tiles.set_homogeneous (true);
	_tiles.set_spacings (px (6));

	_manage.set_name ("generic button");
	_manage.set_text (_("Manage plugins…"));
	_manage.set_size_request (-1, px (28));
	_manage.signal_clicked.connect (sigc::mem_fun (*this, &PluginBrowser::open_manager));

	Gtk::VBox* left = manage (new Gtk::VBox (false, px (8)));
	left->set_border_width (px (14));
	left->pack_start (_tiles, false, false);
	left->pack_end (_manage, false, false);
	Gtk::EventBox* left_box = manage (new Gtk::EventBox);
	left_box->add (*left);
	left_box->set_size_request (px (350), -1);
	bg (*left_box, "#16181c");

	/* ---- middle: the collection ---- */
	_list   = manage (new PluginList (*this));
	_scroll = manage (new Gtk::VScrollbar (_list->adjustment ()));
	Gtk::HBox* mid = manage (new Gtk::HBox);
	mid->pack_start (*_list, true, true);
	mid->pack_start (*_scroll, false, false);

	/* ---- right: the one you are pointing at ---- */
	_plate = manage (new PluginPlate);
	_plate->Step.connect ([this] (int d) { step (d); });
	_name.set_justify (Gtk::JUSTIFY_CENTER);
	_kind_maker.set_justify (Gtk::JUSTIFY_CENTER);
	grey (_kind_maker, "#9aa1ac");
	grey (_name, "#e6e8eb");
	grey (_desc, "#dfe3e8");
	_desc.set_line_wrap (true);
	_desc.set_justify (Gtk::JUSTIFY_CENTER);
	_desc.set_size_request (px (300), -1);
	_format.set_name ("generic button");
	_format.set_size_request (px (70), px (24));
	_format.signal_clicked.connect (sigc::mem_fun (*this, &PluginBrowser::cycle_format));
	for (Gtk::Label* l : { &_fact_goto, &_fact_since, &_fact_used }) {
		grey (*l, "#c3c8d0");
		l->set_justify (Gtk::JUSTIFY_CENTER);
	}
	_facts.set_spacing (px (4));
	_facts.pack_start (_fact_goto, false, false);
	_facts.pack_start (_fact_since, false, false);
	_facts.pack_start (_fact_used, false, false);

	_notes_label.set_markup (string_compose ("<span foreground=\"#7fb6ff\">\xE2\x9C\x8E %1</span>", _("Write notes about this plugin")));
	_notes_link.add (_notes_label);
	_notes_link.set_visible_window (false);
	_notes_link.add_events (Gdk::BUTTON_PRESS_MASK);
	_notes_link.signal_button_press_event ().connect ([this] (GdkEventButton*) { start_edit (); return true; });

	_note_text.set_wrap_mode (Gtk::WRAP_WORD);
	_note_text.set_size_request (px (300), px (78));
	Gtk::ScrolledWindow* note_scroll = manage (new Gtk::ScrolledWindow);
	note_scroll->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	note_scroll->add (_note_text);
	ArdourWidgets::ArdourButton* save   = manage (new ArdourWidgets::ArdourButton (_("Save")));
	ArdourWidgets::ArdourButton* cancel = manage (new ArdourWidgets::ArdourButton (_("Cancel")));
	save->set_name ("generic button");
	cancel->set_name ("generic button");
	save->set_size_request (px (80), px (28));
	cancel->set_size_request (px (80), px (28));
	save->signal_clicked.connect ([this] () { end_edit (true); });
	cancel->signal_clicked.connect ([this] () { end_edit (false); });
	Gtk::HBox* edit_buttons = manage (new Gtk::HBox (false, px (8)));
	Gtk::Alignment* eb_c = manage (new Gtk::Alignment (0.5, 0.5, 0, 0));
	edit_buttons->pack_start (*save, false, false);
	edit_buttons->pack_start (*cancel, false, false);
	eb_c->add (*edit_buttons);
	_note_edit.set_spacing (px (6));
	_note_edit.pack_start (*note_scroll, false, false);
	_note_edit.pack_start (*eb_c, false, false);

	Gtk::HSeparator* sep = manage (new Gtk::HSeparator);
	_right = manage (new Gtk::VBox (false, px (10)));
	Gtk::VBox* info = manage (new Gtk::VBox (false, px (10)));
	info->set_border_width (px (16));
	Gtk::Alignment* fmt_c = manage (new Gtk::Alignment (0.5, 0.5, 0, 0));
	fmt_c->add (_format);
	info->pack_start (_name, false, false);
	info->pack_start (_kind_maker, false, false);
	info->pack_start (*fmt_c, false, false);
	info->pack_start (_desc, false, false);
	info->pack_start (*sep, false, false, px (4));
	info->pack_start (_facts, false, false);
	info->pack_start (_notes_link, false, false);
	info->pack_start (_note_edit, false, false);
	_right->pack_start (*_plate, false, false);
	_right->pack_start (*info, false, false);
	Gtk::EventBox* right_box = manage (new Gtk::EventBox);
	right_box->add (*_right);
	right_box->set_size_request (px (350), -1);
	bg (*right_box, "#16181c");

	Gtk::HBox* body = manage (new Gtk::HBox);
	body->pack_start (*left_box, false, false);
	body->pack_start (*mid, true, true);
	body->pack_start (*right_box, false, false);

	/* ---- bottom: a line of help, and the rack ---- */
	_foot.set_alignment (0.5, 0.5);
	grey (_foot, "#c3c8d0");
	Gtk::EventBox* foot_box = manage (new Gtk::EventBox);
	Gtk::HBox*     foot_row = manage (new Gtk::HBox);
	foot_row->set_border_width (px (10));
	foot_row->pack_start (_foot, true, true);
	foot_box->add (*foot_row);
	bg (*foot_box, "#16181c");

	_rack_title.set_alignment (0.5, 0.5);
	_rack_msg.set_alignment (0.5, 0.5);
	grey (_rack_title, "#c3c8d0");
	grey (_rack_msg, "#c3c8d0");
	_rack = manage (new Gtk::DrawingArea);
	_rack->set_size_request (-1, px (52));
	_rack->signal_expose_event ().connect (sigc::mem_fun (*this, &PluginBrowser::rack_expose));
	_rack->set_can_focus (true);
	_rack->add_events (Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::BUTTON1_MOTION_MASK);
	_rack->signal_button_press_event ().connect (sigc::mem_fun (*this, &PluginBrowser::rack_press));
	_rack->signal_button_release_event ().connect (sigc::mem_fun (*this, &PluginBrowser::rack_release));
	_rack->signal_motion_notify_event ().connect (sigc::mem_fun (*this, &PluginBrowser::rack_motion));
	Gtk::VBox* rack_box = manage (new Gtk::VBox (false, px (6)));
	rack_box->set_border_width (px (12));
	rack_box->pack_start (_rack_title, false, false);
	rack_box->pack_start (*_rack, false, false);
	rack_box->pack_start (_rack_msg, false, false);
	Gtk::EventBox* rack_eb = manage (new Gtk::EventBox);
	rack_eb->add (*rack_box);
	bg (*rack_eb, "#1a1d22");

	Gtk::VBox* root = manage (new Gtk::VBox);
	_top = manage (new Gtk::EventBox);
	_top->add (*top_row);
	bg (*_top, "#1a1d22");
	root->pack_start (*_top, false, false);
	root->pack_start (*manage (new Gtk::HSeparator), false, false);
	root->pack_start (*body, true, true);
	root->pack_start (*manage (new Gtk::HSeparator), false, false);
	root->pack_start (*foot_box, false, false);
	root->pack_start (*rack_eb, false, false);

	Gtk::EventBox* frame = manage (new Gtk::EventBox);
	frame->add (*root);
	bg (*frame, "#1a1d22");
	modify_bg (Gtk::STATE_NORMAL, Gdk::Color ("#343943"));
	Gtk::Window::add (*frame);
	frame->show_all ();

	_notes_link.show_all ();
	_note_edit.hide ();

	{
		Gdk::Rectangle r;
		Glib::RefPtr<Gdk::Screen> sc = Gdk::Screen::get_default ();
		const int sw = sc->get_width (), sh = sc->get_height ();
		set_default_size (std::min (px (1340), sw - 40), std::min (px (800), sh - 60));
	}
}

PluginBrowser::~PluginBrowser ()
{
	delete _picker;
	for (ArdourWidgets::ArdourButton* b : _tile_kind) {
		delete b;
	}
}

/* ---- the data ---------------------------------------------------------- */

static string
meta_path ()
{
	return Glib::build_filename (user_config_directory (), "plugin_browser.xml");
}

void
PluginBrowser::load_meta ()
{
	_meta.clear ();
	const string path = meta_path ();
	if (!Glib::file_test (path, Glib::FILE_TEST_EXISTS)) {
		return;
	}
	XMLTree tree;
	if (!tree.read (path)) {
		return;
	}
	for (XMLNode* n : tree.root ()->children ()) {
		string name, note;
		int64_t seen = 0;
		if (n->get_property ("name", name)) {
			n->get_property ("note", note);
			n->get_property ("first-seen", seen);
			_meta[name] = std::make_pair (note, seen);
		}
	}
}

void
PluginBrowser::save_meta () const
{
	XMLNode* root = new XMLNode ("PluginBrowser");
	root->set_property ("version", 1);
	for (auto const& m : _meta) {
		XMLNode* n = root->add_child ("Plugin");
		n->set_property ("name", m.first);
		if (!m.second.first.empty ()) {
			n->set_property ("note", m.second.first);
		}
		n->set_property ("first-seen", m.second.second);
	}
	XMLTree tree;
	tree.set_root (root);
	tree.write (meta_path ());
}

static int
format_rank (PluginType t)
{
	switch (t) {
	case LV2: return 0;
	case VST3: return 1;
	case AudioUnit: return 1;
	case LXVST: return 2;
	case Windows_VST: return 3;
	case MacVST: return 3;
	case LADSPA: return 4;
	default: return 5;
	}
}

void
PluginBrowser::load_entries ()
{
	PluginManager& pm = PluginManager::instance ();
	PluginInfoList all;
	for (PluginInfoList const* l : { &pm.lv2_plugin_info (), &pm.vst3_plugin_info (), &pm.lxvst_plugin_info (), &pm.ladspa_plugin_info (),
	                                 &pm.lua_plugin_info (), &pm.windows_vst_plugin_info (), &pm.mac_vst_plugin_info (), &pm.au_plugin_info () }) {
		all.insert (all.end (), l->begin (), l->end ());
	}

	_entries.clear ();
	std::map<string, size_t> index;
	for (PluginInfoPtr const& p : all) {
		PluginManager::PluginStatusType st = pm.get_status (p);
		if (st == PluginManager::Hidden || st == PluginManager::Concealed) {
			continue;
		}
		const bool instrument = p->is_instrument ();
		/* some plugins give their name or maker in an old encoding: keep only what is proper text */
		const string pname = PBD::sanitize_utf8 (p->name);
		const string key   = lower (pname) + (instrument ? "|i" : "|e");
		std::map<string, size_t>::iterator i = index.find (key);
		if (i != index.end ()) {
			_entries[i->second].variants.push_back (p);
			continue;
		}
		Entry e;
		e.name       = pname;
		e.lname      = lower (pname);
		e.maker      = PBD::sanitize_utf8 (p->creator);
		while (!e.maker.empty () && g_ascii_isspace (e.maker[e.maker.size () - 1])) {
			e.maker.erase (e.maker.size () - 1);
		}
		e.lmaker     = lower (e.maker);
		e.instrument = instrument;
		e.favourite  = false;
		e.go_to      = false;
		e.last_used  = 0;
		e.uses       = 0;
		e.first_seen = 0;
		e.variant    = 0;
		e.score      = 0;
		e.seed       = hash_of (e.name);
		string tags;
		for (string const& t : pm.get_tags (p)) {
			tags += lower (PBD::sanitize_utf8 (t)) + " ";
		}
		e.tags = tags;
		e.kind = guess_kind (PBD::sanitize_utf8 (p->category) + " " + pname + " " + tags, instrument, p->is_analyzer ());
		e.variants.push_back (p);
		string plain = e.lname;
		for (const char* suffix : { " (stereo)", " (mono)", " stereo", " mono" }) {
			const size_t n = strlen (suffix);
			if (plain.size () > n && plain.compare (plain.size () - n, n, suffix) == 0) {
				plain.erase (plain.size () - n);
			}
		}
		for (const Described* d = descriptions; d->name; ++d) {
			if (e.lname == d->name || plain == d->name) {
				e.builtin = _(d->text);
				break;
			}
		}
		skin_for (e.lmaker, e.light, e.dark, e.ink);
		index[key] = _entries.size ();
		_entries.push_back (e);
	}
	for (Entry& e : _entries) {
		std::stable_sort (e.variants.begin (), e.variants.end (), [] (PluginInfoPtr const& a, PluginInfoPtr const& b) { return format_rank (a->type) < format_rank (b->type); });
	}
	std::sort (_entries.begin (), _entries.end (), [] (Entry const& a, Entry const& b) { return a.lname < b.lname; });

	/* what you wrote, and when it came into your collection */
	const bool first_run = !Glib::file_test (meta_path (), Glib::FILE_TEST_EXISTS);
	load_meta ();
	bool dirty = first_run;
	const int64_t now = time (0);
	for (Entry& e : _entries) {
		std::map<string, std::pair<string, int64_t> >::iterator m = _meta.find (e.lname);
		if (m == _meta.end ()) {
			/* the very first time we look, everything is "from before"; after that, new plugins are new */
			_meta[e.lname] = std::make_pair (string (), first_run ? (int64_t) 0 : now);
			m              = _meta.find (e.lname);
			dirty          = true;
		}
		e.note       = m->second.first;
		e.first_seen = m->second.second;
	}
	if (dirty) {
		save_meta ();
	}
	refresh_stats ();
	_loaded = true;
}

void
PluginBrowser::refresh_stats ()
{
	PluginManager& pm = PluginManager::instance ();
	for (Entry& e : _entries) {
		e.uses      = 0;
		e.last_used = 0;
		e.favourite = false;
		e.go_to     = false;
		for (PluginInfoPtr const& v : e.variants) {
			int64_t  lru;
			uint64_t n;
			if (pm.stats (v, lru, n)) {
				e.uses += n;
				e.last_used = std::max (e.last_used, lru);
			}
			if (pm.get_status (v) == PluginManager::Favorite) {
				e.favourite = true;
			}
		}
	}
	/* the one you use most of each kind (when you really use it) is your go-to */
	for (int k = 0; k < NKinds; ++k) {
		Entry* best = 0;
		for (Entry& e : _entries) {
			if (e.kind == k && e.uses >= 5 && (!best || e.uses > best->uses)) {
				best = &e;
			}
		}
		if (best) {
			best->go_to = true;
		}
	}
}

/* ---- searching --------------------------------------------------------- */

/* what a search word is held against, for one plugin */
static Subject
subject_of (PluginBrowser::Entry const& e)
{
	PluginBrowser::KindInfo const& k = kind_table[e.kind];
	Subject sub = { e.lname, e.lmaker, e.tags, e.description (), !e.note.empty (), _(k.label), k.words };
	return sub;
}

static vector<int>
search (vector<PluginBrowser::Entry>& entries, vector<int> const& among, vector<string> const& words)
{
	vector<int> out;
	for (int idx : among) {
		PluginBrowser::Entry& e = entries[idx];
		double sc = 0;
		bool   ok = true;
		e.hits.clear ();
		const Subject sub = subject_of (e);
		for (string const& t : words) {
			Fit f = fit (sub, t);
			if (!f.best) {
				ok = false;
				break;
			}
			sc += f.best;
			if (f.hit_start >= 0) {
				e.hits.push_back (std::make_pair (f.hit_start, f.hit_len));
			}
		}
		if (ok) {
			e.score = sc + (e.favourite ? 12 : 0) + std::min<double> (e.uses, 30) * 0.6;
			out.push_back (idx);
		} else {
			e.hits.clear ();
		}
	}
	std::sort (out.begin (), out.end (), [&entries] (int a, int b) {
		return entries[a].score != entries[b].score ? entries[a].score > entries[b].score : entries[a].lname < entries[b].lname;
	});
	return out;
}

void
PluginBrowser::rebuild ()
{
	_rows.clear ();
	for (Entry& e : _entries) {
		e.hits.clear ();
	}

	vector<int> mine, other;
	for (size_t i = 0; i < _entries.size (); ++i) {
		(_entries[i].instrument == _instruments ? mine : other).push_back ((int) i);
	}
	auto in_filter = [this] (Entry const& e) {
		if (_filter == -1) { return true; }
		if (_filter == -2) { return e.favourite; }
		if (_filter == -3) { return e.is_new (); }
		return e.kind == _filter;
	};
	auto az = [this] (int a, int b) { return _entries[a].lname < _entries[b].lname; };

	auto section = [this] (string const& group, string const& note, vector<int> const& items) {
		if (items.empty ()) {
			return;
		}
		Row h = { true, group, note, -1, "", false };
		_rows.push_back (h);
		const bool quiet = (group == _("RECENT") || group == _("FAVOURITES"));
		for (int i : items) {
			Entry const& e = _entries[i];
			Row r = { false, "", "", i, (e.go_to && !quiet) ? string (_("MY GO-TO")) : (e.is_new () ? string ("NEW") : string ()), quiet };
			if (r.badge == "NEW") { r.badge = "NEW"; }
			_rows.push_back (r);
		}
	};

	const string q = lower (_search.get_text ());
	vector<string> words;
	{
		string cur;
		for (char c : q) {
			if (g_ascii_isspace (c)) {
				if (!cur.empty ()) { words.push_back (cur); cur.clear (); }
			} else {
				cur += c;
			}
		}
		if (!cur.empty ()) { words.push_back (cur); }
	}

	const string noun = _instruments ? _("INSTRUMENTS") : _("EFFECTS");
	if (words.empty ()) {
		if (_filter == -1) {
			vector<int> recent, fresh, favs;
			for (int i : mine) {
				if (_entries[i].last_used > 0) { recent.push_back (i); }
				if (_entries[i].is_new ()) { fresh.push_back (i); }
				if (_entries[i].favourite) { favs.push_back (i); }
			}
			std::sort (recent.begin (), recent.end (), [this] (int a, int b) { return _entries[a].last_used > _entries[b].last_used; });
			if (recent.size () > 5) { recent.resize (5); }
			std::sort (fresh.begin (), fresh.end (), az);
			std::sort (favs.begin (), favs.end (), az);
			section (_("RECENT"), "", recent);
			section (_("NEW IN MY COLLECTION"), _("not tried yet"), fresh);
			section (_("FAVOURITES"), "", favs);
			vector<int> everything = mine;
			std::sort (everything.begin (), everything.end (), az);
			section (string_compose (_("ALL MY %1 A–Z"), noun), string_compose ("%1", mine.size ()), everything);
		} else if (_filter == -2) {
			vector<int> v;
			for (int i : mine) { if (_entries[i].favourite) { v.push_back (i); } }
			std::sort (v.begin (), v.end (), az);
			section (_("FAVOURITES"), _("click a star to add or remove one"), v);
		} else if (_filter == -3) {
			vector<int> v;
			for (int i : mine) { if (_entries[i].is_new ()) { v.push_back (i); } }
			std::sort (v.begin (), v.end (), az);
			section (_("NEW IN MY COLLECTION"), _("installed lately, not tried yet"), v);
		} else {
			vector<int> v;
			for (int i : mine) { if (_entries[i].kind == _filter) { v.push_back (i); } }
			std::sort (v.begin (), v.end (), [this, &az] (int a, int b) {
				Entry const& x = _entries[a]; Entry const& y = _entries[b];
				if (x.favourite != y.favourite) { return x.favourite; }
				if (x.uses != y.uses) { return x.uses > y.uses; }
				return x.lname < y.lname;
			});
			gchar* up = g_utf8_strup (_(kind_table[_filter].label), -1);
			section (up, _("most used first"), v);
			g_free (up);
		}
	} else {
		vector<int> inside, outside;
		for (int i : mine) { (in_filter (_entries[i]) ? inside : outside).push_back (i); }
		string where;
		if (_filter >= 0) { where = string_compose (_("in %1"), _(kind_table[_filter].label)); }
		else if (_filter == -2) { where = _("in Favourites"); }
		else if (_filter == -3) { where = _("in New"); }
		section (_("BEST MATCHES"), where, search (_entries, inside, words));
		if (_filter != -1) {
			section (_("ALSO IN OTHER KINDS"), "", search (_entries, outside, words));
		}
		vector<int> o = search (_entries, other, words);
		if (o.size () > 6) { o.resize (6); }
		section (string_compose (_("ALSO IN %1"), _instruments ? string (_("EFFECTS")) : string (_("INSTRUMENTS"))), "", o);

		if (_rows.empty ()) {
			/* typing mistakes: the letters in order, anywhere in the name */
			const string want = squash (q);
			vector<int> loose;
			if (want.size () > 2) {
				for (size_t i = 0; i < _entries.size () && loose.size () < 8; ++i) {
					const string s = squash (_entries[i].name);
					size_t from = 0;
					bool ok = true;
					for (char c : want) {
						size_t p = s.find (c, from);
						if (p == string::npos) { ok = false; break; }
						from = p + 1;
					}
					if (ok) { loose.push_back ((int) i); }
				}
			}
			section (_("DID YOU MEAN"), "", loose);
		}
	}

	_sel = -1;
	for (size_t i = 0; i < _rows.size (); ++i) {
		if (!_rows[i].header) { _sel = (int) i; break; }
	}
	_list->rows_changed ();
	show_selected ();
}

/* ---- the parts ----------------------------------------------------------- */

std::string
PluginBrowser::tile_tip (int filter) const
{
	if (filter == -1) { return _("Your whole collection, with the ones you used lately and your favourites at the top."); }
	if (filter == -2) { return _("The ones you starred. Click the star on any row to add it here."); }
	if (filter == -3) { return _("Installed lately and not tried yet."); }
	return string_compose ("%1: %2", _(kind_table[filter].label), _(kind_table[filter].tip));
}

void
PluginBrowser::set_foot (string const& text)
{
	if (!text.empty ()) {
		_foot.set_text (text);
		return;
	}
	std::shared_ptr<Route> r = _target.lock ();
	const string track = r ? r->name () : string (_("a track"));
	if (_search.get_text ().empty ()) {
		_foot.set_text (string_compose (_("Search by name, maker or kind: “%1”. Click one to put it on %2. Esc closes."),
		                                _instruments ? _("piano, drums, surge") : _("echo, lsp comp, verb hall"), track));
	} else {
		_foot.set_text (string_compose (_("Click one to put it on %1. Esc closes."), track));
	}
}

void
PluginBrowser::update_tiles ()
{
	Glib::ListHandle<Gtk::Widget*> kids = _tiles.get_children ();
	for (Gtk::Widget* w : kids) {
		_tiles.remove (*w);
	}
	int n_new = 0;
	for (Entry const& e : _entries) {
		if (e.instrument == _instruments && e.is_new ()) { ++n_new; }
	}
	if (_filter == -3 && n_new == 0) {
		_filter = -1;
	}
	_tile_all.set_text (_instruments ? _("All my instruments") : _("All my effects"));
	_tiles.attach (_tile_all, 0, 2, 0, 1);
	if (n_new) {
		_tiles.attach (_tile_fav, 0, 1, 1, 2);
		_tiles.attach (_tile_new, 1, 2, 1, 2);
	} else {
		_tiles.attach (_tile_fav, 0, 2, 1, 2);
	}
	int n = 0;
	for (int i = 0; i < NKinds; ++i) {
		if (kind_table[i].instrument != _instruments) { continue; }
		_tiles.attach (*_tile_kind[i], n % 2, n % 2 + 1, 2 + n / 2, 3 + n / 2);
		++n;
	}
	_tiles.set_row_spacing (1, (guint) rintf (16 * UIConfiguration::instance ().get_ui_scale ()));
	_tiles.show_all ();
	if (!n_new) { _tile_new.hide (); }

	auto state = [] (ArdourWidgets::ArdourButton& b, bool on) { b.set_active_state (on ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off); };
	state (_tile_all, _filter == -1);
	state (_tile_fav, _filter == -2);
	state (_tile_new, _filter == -3);
	for (int i = 0; i < NKinds; ++i) {
		state (*_tile_kind[i], _filter == i);
	}
	state (_tab_fx, !_instruments);
	state (_tab_inst, _instruments);
}

void
PluginBrowser::set_tab (bool instruments)
{
	_instruments = instruments;
	_filter      = -1;
	update_tiles ();
	rebuild ();
	set_foot ("");
	_search.grab_focus ();
}

void
PluginBrowser::set_filter (int f)
{
	_filter = f;
	update_tiles ();
	rebuild ();
}

void
PluginBrowser::search_changed ()
{
	if (_editing) {
		end_edit (false);
	}
	rebuild ();
	set_foot ("");
}

void
PluginBrowser::update_chip ()
{
	std::shared_ptr<Route> r = _target.lock ();
	if (!r) {
		_chip.set_text (string_compose ("%1  <b>%2</b>  \xE2\x96\xBE", _("Add to"), _("choose a track")), true);
		return;
	}
	char col[16];
	snprintf (col, sizeof (col), "#%06x", (unsigned) (r->presentation_info ().color () >> 8));
	_chip.set_text (string_compose ("%1  <span foreground=\"%2\">\xE2\x97\x8F</span>  <b>%3</b>  \xE2\x96\xBE", _("Add to"), col, Gtkmm2ext::markup_escape_text (r->name ())), true);
}

void
PluginBrowser::rack_say (string const& text)
{
	_rack_note = text;
	if (!text.empty ()) {
		_rack_msg.set_text (text);
		return;
	}
	/* nothing to report: say what can be done here */
	std::shared_ptr<Route> r = _target.lock ();
	if (r && r->nth_plugin (0)) {
		_rack_msg.set_markup (string_compose ("<span foreground=\"#7d8590\">%1</span>",
		                                      Gtkmm2ext::markup_escape_text (_("Double-click a unit to open it. Click to select it, Delete takes it off. Drag to change the order. Ctrl+Z undoes."))));
	} else {
		_rack_msg.set_text ("");
	}
}

void
PluginBrowser::rack_changed ()
{
	std::shared_ptr<Route> r = _target.lock ();
	if (r) {
		char col[16];
		snprintf (col, sizeof (col), "#%06x", (unsigned) (r->presentation_info ().color () >> 8));
		_rack_title.set_markup (string_compose ("<span foreground=\"%1\">\xE2\x97\x8F</span> <b>%2</b>   <span foreground=\"#7d8590\">%3</span>",
		                                        col, Gtkmm2ext::markup_escape_text (r->name ()), _("the track's plugin list")));
	} else {
		_rack_title.set_text ("");
	}
	if (_rack_note.empty ()) {
		rack_say ("");
	}
	if (_rack) {
		_rack->queue_draw ();
	}
}

/** the track's plugins as units, laid out centred in `W` (unscaled pixels) */
void
PluginBrowser::rack_layout (double W)
{
	vector<RackUnit> units;
	std::shared_ptr<Route> r = _target.lock ();
	if (r) {
		r->foreach_processor ([&units] (std::weak_ptr<Processor> wp) {
			std::shared_ptr<PluginInsert> pi = std::dynamic_pointer_cast<PluginInsert> (wp.lock ());
			if (!pi || !pi->display_to_user ()) {
				return;
			}
			RackUnit u;
			u.proc = pi;
			u.name = PBD::sanitize_utf8 (pi->name ());
			u.glow = kind_table[OtherEffect].glow;
			string maker;
			if (pi->plugin () && pi->plugin ()->get_info ()) {
				PluginInfoPtr info = pi->plugin ()->get_info ();
				maker  = lower (info->creator);
				u.glow = kind_table[guess_kind (info->category + " " + info->name, info->is_instrument (), info->is_analyzer ())].glow;
			}
			skin_for (maker, u.light, u.dark, u.ink);
			u.x = u.w = 0;
			units.push_back (u);
		});
	}
	const double gap = 8;
	double pw = 184;
	if (!units.empty () && units.size () * (pw + gap) - gap > W) {
		pw = std::max (70.0, (W - (units.size () - 1) * gap) / units.size ());
	}
	double x = (W - (units.size () * (pw + gap) - gap)) / 2;
	for (RackUnit& u : units) {
		u.x = x;
		u.w = pw;
		x += pw + gap;
	}
	_rack_units.swap (units);

	/* a selection that is no longer on this track is no selection */
	std::shared_ptr<Processor> sel = _rack_sel.lock ();
	bool found = false;
	for (RackUnit const& u : _rack_units) {
		found = found || (sel && u.proc.lock () == sel);
	}
	if (!found) {
		_rack_sel.reset ();
	}
}

int
PluginBrowser::rack_unit_at (double x) const
{
	for (size_t i = 0; i < _rack_units.size (); ++i) {
		if (x >= _rack_units[i].x && x < _rack_units[i].x + _rack_units[i].w) {
			return (int) i;
		}
	}
	return -1;
}

/** while dragging: where the unit would land, counted among the other units */
int
PluginBrowser::rack_drop_slot () const
{
	if (_rack_press < 0 || _rack_press >= (int) _rack_units.size ()) {
		return -1;
	}
	const double centre = _rack_drag_x - _rack_grab_dx + _rack_units[_rack_press].w / 2;
	int slot = 0;
	for (size_t i = 0; i < _rack_units.size (); ++i) {
		if ((int) i != _rack_press && _rack_units[i].x + _rack_units[i].w / 2 < centre) {
			++slot;
		}
	}
	return slot;
}

bool
PluginBrowser::rack_expose (GdkEventExpose*)
{
	Cr cr = _rack->get_window ()->create_cairo_context ();
	const float  s = UIConfiguration::instance ().get_ui_scale ();
	const double W = _rack->get_allocation ().get_width () / s, H = _rack->get_allocation ().get_height () / s;
	cr->scale (s, s);
	rgb (cr, col_bg);
	cr->paint ();

	if (!_rack_dragging) {
		rack_layout (W);
	}
	std::shared_ptr<Route> r = _target.lock ();
	if (_rack_units.empty ()) {
		Glib::RefPtr<Pango::Layout> l = make_layout (cr, *_rack, 13, false);
		l->set_text (r && std::dynamic_pointer_cast<MidiTrack> (r) ? _("No instrument yet.") : _("No plugins yet."));
		int tw, th;
		l->get_pixel_size (tw, th);
		rgb (cr, 0x7d8590);
		show_text (cr, l, (W - tw) / 2, (H - th) / 2);
		return true;
	}

	const double y = (H - 44) / 2;
	std::shared_ptr<Processor> sel = _rack_sel.lock ();

	auto draw_unit = [&] (RackUnit const& u, double x, bool selected) {
		PluginBrowser::Entry e;
		e.light = u.light;
		e.dark  = u.dark;
		e.ink   = u.ink;
		panel (cr, x, y, u.w, 44, 5, e, true);
		rgb (cr, 0x0a0c0e);
		rounded (cr, x + 9, y + 14, 20, 16, 2);
		cr->fill ();
		rgb (cr, u.glow);
		cr->rectangle (x + 12, y + 21, 14, 2);
		cr->fill ();
		text_at (cr, *_rack, Gtkmm2ext::markup_escape_text (u.name), 11, true, u.ink, 1.0, x + 38, y + 14, u.w - 56, false);
		led (cr, x + u.w - 10, y + 22, 2.5, true);
		if (selected) {
			rgb (cr, col_accent);
			cr->set_line_width (2);
			rounded (cr, x - 2, y - 2, u.w + 4, 48, 7);
			cr->stroke ();
		}
	};

	for (size_t i = 0; i < _rack_units.size (); ++i) {
		RackUnit const& u = _rack_units[i];
		if (_rack_dragging && (int) i == _rack_press) {
			/* where it came from */
			rgb (cr, 0xffffff, 0.12);
			cr->set_line_width (1);
			rounded (cr, u.x + 0.5, y + 0.5, u.w - 1, 43, 5);
			cr->stroke ();
			continue;
		}
		draw_unit (u, u.x, sel && u.proc.lock () == sel);
	}

	if (_rack_dragging && _rack_press >= 0 && _rack_press < (int) _rack_units.size ()) {
		/* where it would go, then the unit itself under the pointer */
		const int slot = rack_drop_slot ();
		vector<const RackUnit*> others;
		for (size_t i = 0; i < _rack_units.size (); ++i) {
			if ((int) i != _rack_press) {
				others.push_back (&_rack_units[i]);
			}
		}
		double mx = _rack_units[_rack_press].x + _rack_units[_rack_press].w / 2;
		if (!others.empty ()) {
			mx = slot < (int) others.size () ? others[slot]->x - 4 : others.back ()->x + others.back ()->w + 4;
		}
		rgb (cr, col_accent);
		rounded (cr, mx - 1.5, y - 3, 3, 50, 1.5);
		cr->fill ();
		draw_unit (_rack_units[_rack_press], _rack_drag_x - _rack_grab_dx, true);
	}
	return true;
}

bool
PluginBrowser::rack_press (GdkEventButton* ev)
{
	if (ev->button != 1) {
		return false;
	}
	const double x = ev->x / UIConfiguration::instance ().get_ui_scale ();
	const int    u = rack_unit_at (x);
	_rack->grab_focus ();

	if (ev->type == GDK_2BUTTON_PRESS) {
		_rack_press = -1;
		if (u >= 0) {
			rack_open (_rack_units[u].proc.lock ());
		}
		return true;
	}
	if (ev->type != GDK_BUTTON_PRESS) {
		return true;
	}
	_rack_dragging = false;
	_rack_press    = u;
	_rack_press_x  = x;
	_rack_grab_dx  = u >= 0 ? x - _rack_units[u].x : 0;
	rack_select (u >= 0 ? _rack_units[u].proc.lock () : std::shared_ptr<Processor> ());
	return true;
}

bool
PluginBrowser::rack_motion (GdkEventMotion* ev)
{
	if (_rack_press < 0 || !(ev->state & GDK_BUTTON1_MASK)) {
		return false;
	}
	const double x = ev->x / UIConfiguration::instance ().get_ui_scale ();
	if (!_rack_dragging && fabs (x - _rack_press_x) > 6) {
		_rack_dragging = true;
	}
	if (_rack_dragging) {
		_rack_drag_x = x;
		_rack->queue_draw ();
	}
	return true;
}

bool
PluginBrowser::rack_release (GdkEventButton* ev)
{
	if (ev->button != 1) {
		return false;
	}
	if (_rack_dragging) {
		const int from = _rack_press;
		const int slot = rack_drop_slot ();
		_rack_dragging = false;
		_rack_press    = -1;
		rack_move (from, slot);
		_rack->queue_draw ();
	}
	_rack_press = -1;
	return true;
}

void
PluginBrowser::rack_select (std::shared_ptr<Processor> p)
{
	_rack_sel = p;
	rack_say (p ? string_compose (_("%1 is selected. Delete takes it off, double-click opens it, drag moves it."), p->name ()) : string ());
	_rack->queue_draw ();
}

static Route::ProcessorList
visible_processors (std::shared_ptr<Route> r)
{
	Route::ProcessorList l;
	r->foreach_processor ([&l] (std::weak_ptr<Processor> wp) {
		std::shared_ptr<Processor> p = wp.lock ();
		if (p && p->display_to_user ()) {
			l.push_back (p);
		}
	});
	return l;
}

void
PluginBrowser::rack_remove_selected ()
{
	std::shared_ptr<Route>     r = _target.lock ();
	std::shared_ptr<Processor> p = _rack_sel.lock ();
	if (!r || !p) {
		return;
	}
	/* remember what it was and where, so that Ctrl+Z can put it back */
	Undo u;
	u.what   = Undo::Removed;
	u.route  = r;
	u.name   = PBD::sanitize_utf8 (p->name ());
	u.active = p->enabled ();
	u.state.reset (&p->get_state ());
	u.index = 0;
	for (std::shared_ptr<Processor> const& q : visible_processors (r)) {
		if (q == p) {
			break;
		}
		++u.index;
	}

	Route::ProcessorStreams err;
	if (r->remove_processor (p, &err) != 0) {
		rack_say (string_compose (_("%1 could not be taken off %2."), u.name, r->name ()));
		return;
	}
	_undo.push_back (u);
	_rack_sel.reset ();
	rack_say (string_compose (_("%1 is off %2. Ctrl+Z puts it back."), u.name, r->name ()));
	rack_changed ();
}

void
PluginBrowser::rack_open (std::shared_ptr<Processor> p)
{
	std::shared_ptr<Route> r = _target.lock ();
	if (!r || !p) {
		return;
	}
	if (!AudioEngine::instance ()->running ()) {
		rack_say (_("The audio engine is not running, so the plugin's window cannot open."));
		return;
	}
	MixerStrip* strip = Mixer_UI::instance ()->strip_for_route (r);
	if (!strip) {
		return;
	}
	strip->edit_processor (p);
}

/** move unit `from` so that it sits at `slot`, counted among the other units */
void
PluginBrowser::rack_move (int from, int slot)
{
	std::shared_ptr<Route> r = _target.lock ();
	if (!r || from < 0 || from >= (int) _rack_units.size () || slot < 0 || slot == from) {
		return;
	}
	std::shared_ptr<Processor> moving = _rack_units[from].proc.lock ();
	vector<std::shared_ptr<Processor> > others;
	for (size_t i = 0; i < _rack_units.size (); ++i) {
		if ((int) i != from) {
			if (std::shared_ptr<Processor> q = _rack_units[i].proc.lock ()) {
				others.push_back (q);
			}
		}
	}
	if (!moving || others.empty ()) {
		return;
	}

	const Route::ProcessorList before = visible_processors (r);
	Route::ProcessorList       order  = before;
	order.remove (moving);
	if (slot < (int) others.size ()) {
		order.insert (std::find (order.begin (), order.end (), others[slot]), moving);
	} else {
		Route::ProcessorList::iterator i = std::find (order.begin (), order.end (), others.back ());
		if (i != order.end ()) {
			++i;
		}
		order.insert (i, moving);
	}

	Route::ProcessorStreams err;
	if (r->reorder_processors (order, &err) != 0) {
		rack_say (string_compose (_("%1 cannot go there: the plugins around it would not fit together."), moving->name ()));
		return;
	}
	Undo u;
	u.what  = Undo::Moved;
	u.route = r;
	u.name  = PBD::sanitize_utf8 (moving->name ());
	u.index = 0;
	u.active = true;
	for (std::shared_ptr<Processor> const& q : before) {
		u.order.push_back (q);
	}
	_undo.push_back (u);
	_rack_sel = moving;
	rack_say (string_compose (_("Moved %1. Ctrl+Z moves it back."), moving->name ()));
	rack_changed ();
}

/** Ctrl+Z: take back the last thing done to a track from this window */
void
PluginBrowser::undo ()
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	while (!_undo.empty ()) {
		Undo u = _undo.back ();
		_undo.pop_back ();
		std::shared_ptr<Route> r = u.route.lock ();
		if (!r || !s) {
			continue; /* the track is gone: try the thing before */
		}
		Route::ProcessorStreams err;

		if (u.what == Undo::Added) {
			std::shared_ptr<Processor> p = u.proc.lock ();
			if (!p) {
				continue;
			}
			if (r->remove_processor (p, &err) == 0) {
				rack_say (string_compose (_("%1 is off %2 again."), u.name, r->name ()));
			}
		} else if (u.what == Undo::Moved) {
			Route::ProcessorList order;
			for (std::weak_ptr<Processor> const& w : u.order) {
				if (std::shared_ptr<Processor> p = w.lock ()) {
					order.push_back (p);
				}
			}
			if (r->reorder_processors (order, &err) == 0) {
				rack_say (string_compose (_("%1 is back where it was."), u.name));
			}
		} else if (u.state) {
			/* build it again from what it was, the way pasting a plugin does */
			try {
				std::shared_ptr<PluginInsert> p (new PluginInsert (*s, *r));
				const PBD::ID id = p->id ();
				XMLNode       state (*u.state);
				state.remove_nodes_and_delete ("Processor");
				uint32_t count = 0;
				state.get_property ("count", count);
				state.remove_property ("count");
				state.remove_property ("custom");
				state.remove_nodes_and_delete ("ConfiguredInput");
				state.remove_nodes_and_delete ("CustomSinks");
				state.remove_nodes_and_delete ("ConfiguredOutput");
				state.remove_nodes_and_delete ("PresetOutput");
				state.remove_nodes_and_delete ("ThruMap");
				for (uint32_t i = 0; i < count; ++i) {
					state.remove_nodes_and_delete (string_compose ("InputMap-%1", i));
					state.remove_nodes_and_delete (string_compose ("OutputMap-%1", i));
				}
				{
					PBD::Stateful::ForceIDRegeneration force_ids;
					p->set_state (state, PBD::Stateful::current_state_version);
				}
				p->update_id (id);
				if (r->add_processor_by_index (p, u.index, &err, u.active) == 0) {
					_rack_sel = p;
					rack_say (string_compose (_("%1 is back on %2, with its settings."), u.name, r->name ()));
				} else {
					rack_say (string_compose (_("%1 could not be put back on %2."), u.name, r->name ()));
				}
			} catch (...) {
				rack_say (string_compose (_("%1 could not be put back on %2."), u.name, r->name ()));
			}
		}
		if (r != _target.lock ()) {
			set_target (r); /* show the track it happened on */
		}
		rack_changed ();
		return;
	}
	rack_say (_("Nothing to undo here."));
}

void
PluginBrowser::set_target (std::shared_ptr<Route> r)
{
	_target = r;
	_route_connection.disconnect ();
	if (r) {
		r->processors_changed.connect (_route_connection, invalidator (*this), [this] (RouteProcessorChange) { rack_changed (); }, gui_context ());
		/* effects for audio tracks and busses; instruments for a MIDI track that has none */
		const bool want_instruments = std::dynamic_pointer_cast<MidiTrack> (r) && !r->the_instrument ();
		_instruments = want_instruments;
	}
	_filter = -1;
	update_tiles ();
	update_chip ();
	rack_changed ();
	rack_say ("");
	rebuild ();
	set_foot ("");
}

void
PluginBrowser::choose_target ()
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s) {
		return;
	}
	vector<std::shared_ptr<Route> > routes;
	std::shared_ptr<RouteList const> rl = s->get_routes ();
	for (std::shared_ptr<Route> const& r : *rl) {
		if (r->is_singleton () || r->is_auditioner () || r->is_foldbackbus () || r->presentation_info ().hidden ()) {
			continue;
		}
		routes.push_back (r);
	}
	std::stable_sort (routes.begin (), routes.end (), [] (std::shared_ptr<Route> const& a, std::shared_ptr<Route> const& b) {
		return a->presentation_info ().order () < b->presentation_info ().order ();
	});

	if (routes.empty ()) {
		rack_say (_("This session has no tracks yet."));
		return;
	}

	vector<TrackPicker::Item> items;
	std::shared_ptr<Route> cur = _target.lock ();
	for (std::shared_ptr<Route> const& r : routes) {
		TrackPicker::Item it;
		it.route = r;
		it.name  = r->name ();
		if (std::dynamic_pointer_cast<MidiTrack> (r)) {
			it.what = r->the_instrument () ? _("MIDI") : _("MIDI · no instrument yet");
		} else if (std::dynamic_pointer_cast<Track> (r)) {
			it.what = _("Audio");
		} else {
			it.what = _("Bus");
		}
		it.colour  = r->presentation_info ().color () >> 8;
		it.current = (r == cur);
		items.push_back (it);
	}

	if (!_picker) {
		_picker = new TrackPicker;
		_picker->Picked.connect (sigc::mem_fun (*this, &PluginBrowser::picked));
		_picker->Entered.connect (sigc::mem_fun (*this, &PluginBrowser::picker_entered));
		_picker->Left.connect (sigc::mem_fun (*this, &PluginBrowser::picker_left));
	}
	_picker_open_timer.disconnect ();
	_picker_close_timer.disconnect ();

	/* just under the button, centred on it */
	int cx = 0, cy = 0, ox = 0, oy = 0;
	_chip.translate_coordinates (*this, 0, 0, cx, cy);
	get_window ()->get_origin (ox, oy);
	const float scale = UIConfiguration::instance ().get_ui_scale ();
	_picker->open (items, ox + cx + _chip.get_allocation ().get_width () / 2, oy + cy + _chip.get_allocation ().get_height () + (int) rintf (6 * scale));
}

void
PluginBrowser::close_picker ()
{
	_picker_open_timer.disconnect ();
	_picker_close_timer.disconnect ();
	if (_picker) {
		_picker->close ();
	}
}

void
PluginBrowser::picker_entered ()
{
	_picker_close_timer.disconnect ();
}

/* the pointer left the button or the list: close, unless it comes back at once */
void
PluginBrowser::picker_left ()
{
	_picker_close_timer.disconnect ();
	if (_picker && _picker->is_visible ()) {
		_picker_close_timer = Glib::signal_timeout ().connect ([this] () {
			/* look where the pointer really is, rather than trusting the order
			 * in which "left the button" and "entered the list" arrive */
			int x, y;
			_picker->get_pointer (x, y);
			if (x >= 0 && y >= 0 && x < _picker->get_width () && y < _picker->get_height ()) {
				return false;
			}
			_chip.get_pointer (x, y);
			if (x >= 0 && y >= 0 && x < _chip.get_allocation ().get_width () && y < _chip.get_allocation ().get_height ()) {
				return false;
			}
			close_picker ();
			return false;
		}, 280);
	}
}

void
PluginBrowser::picked (std::shared_ptr<Route> r)
{
	_picker_close_timer.disconnect ();
	if (r != _origin.lock ()) {
		_placement = -1;
	}
	set_target (r);
	_search.grab_focus ();
}

void
PluginBrowser::select (int row)
{
	if (row == _sel) {
		return;
	}
	_sel = row;
	show_selected ();
	_list->queue_draw ();
}

void
PluginBrowser::step (int delta)
{
	if (_rows.empty ()) {
		return;
	}
	for (int n = _sel + delta; n >= 0 && n < (int) _rows.size (); n += delta) {
		if (!_rows[n].header) {
			_sel = n;
			show_selected ();
			_list->ensure_visible (n);
			_list->queue_draw ();
			return;
		}
	}
}

static string
when (int64_t t)
{
	const int64_t days = (time (0) - t) / 86400;
	if (days <= 0) { return _("today"); }
	if (days == 1) { return _("yesterday"); }
	if (days < 7) { return string_compose (_("%1 days ago"), days); }
	if (days < 14) { return _("last week"); }
	char buf[64];
	struct tm tmv;
	time_t tt = t;
	localtime_r (&tt, &tmv);
	strftime (buf, sizeof (buf), "%e %B %Y", &tmv);
	return buf;
}

/* every unit of the list is a slot in the rack; the selected one is in the middle */
void
PluginBrowser::feed_rack ()
{
	std::vector<Entry const*> list;
	int                       at = -1;
	for (size_t i = 0; i < _rows.size (); ++i) {
		if (_rows[i].header) {
			continue;
		}
		if ((int) i == _sel) {
			at = (int) list.size ();
		}
		list.push_back (&_entries[_rows[i].entry]);
	}
	_plate->set_compact (_editing);
	_plate->set_rack (list, at);
}

void
PluginBrowser::show_selected ()
{
	if (_editing) {
		end_edit (false);
	}
	feed_rack ();
	if (_sel < 0 || _sel >= (int) _rows.size () || _rows[_sel].header) {
		_name.set_text ("");
		_kind_maker.set_text ("");
		_desc.set_text ("");
		_format.hide ();
		_facts.hide ();
		_notes_link.hide ();
		return;
	}
	Entry const&    e = _entries[_rows[_sel].entry];
	KindInfo const& k = kind_table[e.kind];

	_name.set_markup (string_compose ("<span size=\"x-large\" weight=\"bold\">%1</span>", Gtkmm2ext::markup_escape_text (e.name)));
	_kind_maker.set_text (e.maker.empty () ? string (_(k.label)) : string_compose (_("%1 · by %2"), _(k.label), e.maker));
	_desc.set_text ((e.note.empty () ? "" : "\xE2\x9C\x8E ") + e.description ());
	_format.set_text (PluginManager::plugin_type_name (e.variants[e.variant]->type, true) + (e.variants.size () > 1 ? " \xE2\x96\xBE" : ""));
	_format.show ();

	if (e.go_to) {
		_fact_goto.set_markup (string_compose ("<span foreground=\"#4c9aff\" weight=\"bold\">%1</span>", string_compose (_("Your go-to %1"), _(k.one))));
		_fact_goto.show ();
	} else {
		_fact_goto.hide ();
	}
	if (e.first_seen > 0) {
		char buf[64];
		struct tm tmv;
		time_t tt = e.first_seen;
		localtime_r (&tt, &tmv);
		strftime (buf, sizeof (buf), "%B %Y", &tmv);
		_fact_since.set_text (string_compose (_("In your collection since %1"), buf));
		_fact_since.show ();
	} else {
		_fact_since.hide ();
	}
	_fact_used.set_text (e.uses ? string_compose (_("Used %1 times · last time %2"), e.uses, when (e.last_used)) : string (_("Not tried yet")));
	_fact_used.show ();
	_facts.show ();

	_notes_label.set_markup (string_compose ("<span foreground=\"#7fb6ff\">\xE2\x9C\x8E %1</span>", e.note.empty () ? _("Write notes about this plugin") : _("Edit notes about this plugin")));
	_notes_link.show ();
}

void
PluginBrowser::start_edit ()
{
	if (_sel < 0 || _rows[_sel].header) {
		return;
	}
	_editing = true;
	_plate->set_compact (true);
	_note_text.get_buffer ()->set_text (_entries[_rows[_sel].entry].note);
	_notes_link.hide ();
	_note_edit.show_all ();
	_note_text.grab_focus ();
}

void
PluginBrowser::end_edit (bool save)
{
	if (!_editing) {
		return;
	}
	_editing = false;
	if (save && _sel >= 0 && !_rows[_sel].header) {
		Entry& e = _entries[_rows[_sel].entry];
		string text = _note_text.get_buffer ()->get_text ();
		while (!text.empty () && g_ascii_isspace (text[text.size () - 1])) { text.erase (text.size () - 1); }
		while (!text.empty () && g_ascii_isspace (text[0])) { text.erase (0, 1); }
		e.note = text;
		_meta[e.lname].first = text;
		save_meta ();
	}
	_note_edit.hide ();
	show_selected ();
	_list->queue_draw ();
	_search.grab_focus ();
}

void
PluginBrowser::toggle_star (int entry)
{
	Entry& e = _entries[entry];
	PluginManager& pm = PluginManager::instance ();
	const bool on = !e.favourite;
	for (PluginInfoPtr const& v : e.variants) {
		pm.set_status (v->type, v->unique_id, on ? PluginManager::Favorite : PluginManager::Normal);
	}
	pm.save_statuses ();
	e.favourite = on;
	_list->queue_draw ();
}

void
PluginBrowser::cycle_format ()
{
	if (_sel < 0 || _rows[_sel].header) {
		return;
	}
	Entry& e = _entries[_rows[_sel].entry];
	if (e.variants.size () > 1) {
		e.variant = (e.variant + 1) % e.variants.size ();
		show_selected ();
	}
}

void
PluginBrowser::open_manager ()
{
	Glib::RefPtr<Gtk::ToggleAction> act = ActionManager::get_toggle_action ("Window", "toggle-plugin-manager", false);
	if (act && !act->get_active ()) {
		act->set_active (true);
	}
}

void
PluginBrowser::add_entry (int entry)
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s) {
		return;
	}
	std::shared_ptr<Route> route = _target.lock ();
	if (!route) {
		rack_say (_("Choose a track first: click “Add to” at the top right."));
		choose_target ();
		return;
	}
	Entry&        e    = _entries[entry];
	PluginInfoPtr info = e.variants[e.variant];

	PluginPtr p = info->load (*s);
	if (!p) {
		rack_say (string_compose (_("%1 could not be loaded."), e.name));
		return;
	}
	std::shared_ptr<Processor> processor (new PluginInsert (*s, *route, p));
	Route::ProcessorStreams    err;
	int rv;
	const bool active = Config->get_new_plugins_active ();
	if (e.instrument && !route->the_instrument ()) {
		rv = route->add_processor_by_index (processor, 0, &err, active);
	} else if (_placement >= 0 && route == _origin.lock ()) {
		rv = route->add_processor_by_index (processor, _placement, &err, active);
		if (rv == 0) {
			++_placement;
		}
	} else {
		rv = route->add_processor (processor, PreFader, &err, active);
	}
	if (rv != 0) {
		rack_say (string_compose (_("%1 could not be added: its inputs and outputs do not fit %2."), e.name, route->name ()));
		return;
	}

	PluginManager::instance ().stats_use_plugin (info);
	e.uses += 1;
	e.last_used = time (0);
	refresh_stats ();
	show_selected ();
	{
		Undo u;
		u.what   = Undo::Added;
		u.route  = route;
		u.proc   = processor;
		u.name   = e.name;
		u.index  = 0;
		u.active = true;
		_undo.push_back (u);
	}
	_rack_sel.reset ();
	rack_say (string_compose (_("%1 is on %2. Click another to add more. Ctrl+Z takes it off again."), e.name, route->name ()));
	rack_changed ();
	_list->queue_draw ();
}

/* ---- the window ---------------------------------------------------------- */

void
PluginBrowser::on_show ()
{
	load_entries ();
	_search.set_text ("");
	_editing = false;
	if (Session* s = ARDOUR_UI::instance ()->the_session ()) {
		/* its tracks go when the session closes, and so does this window */
		s->DropReferences.connect (_session_connection, invalidator (*this), [this] () { hide (); }, gui_context ());
	}
	_undo.clear ();
	_rack_sel.reset ();
	_rack_note.clear ();
	_note_edit.hide ();

	std::shared_ptr<Route> r = _target.lock ();
	_route_connection.disconnect ();
	_filter = -1;
	set_target (r);
	update_tiles ();
	rebuild ();
	rack_say ("");

	Gtk::Window& mw = ARDOUR_UI::instance ()->main_window ();
	int mx, my, mww, mwh, w, h;
	mw.get_position (mx, my);
	mw.get_size (mww, mwh);
	get_size (w, h);
	move (mx + std::max (0, (mww - w) / 2), my + std::max (0, (mwh - h) / 2));

	Gtk::Window::on_show ();
	_search.grab_focus ();
}

void
PluginBrowser::on_hide ()
{
	close_picker ();
	_undo.clear ();
	_session_connection.disconnect ();
	_route_connection.disconnect ();
	Gtk::Window::on_hide ();
}

bool
PluginBrowser::search_key (GdkEventKey* ev)
{
	switch (ev->keyval) {
	case GDK_Down:
	case GDK_KP_Down:
		step (1);
		return true;
	case GDK_Up:
	case GDK_KP_Up:
		step (-1);
		return true;
	case GDK_Page_Down:
		for (int i = 0; i < 8; ++i) { step (1); }
		return true;
	case GDK_Page_Up:
		for (int i = 0; i < 8; ++i) { step (-1); }
		return true;
	case GDK_Return:
	case GDK_KP_Enter:
		if (_sel >= 0 && !_rows[_sel].header) {
			add_entry (_rows[_sel].entry);
		}
		return true;
	}
	return false;
}

bool
PluginBrowser::on_key_press_event (GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape) {
		if (_picker && _picker->is_visible ()) {
			close_picker ();
		} else if (_editing) {
			end_edit (false);
		} else {
			hide ();
		}
		return true;
	}
	const bool primary = Gtkmm2ext::Keyboard::modifier_state_equals (ev->state, Gtkmm2ext::Keyboard::PrimaryModifier);
	if (primary && !_editing && (ev->keyval == GDK_z || ev->keyval == GDK_Z)) {
		undo ();
		return true;
	}
	if (_rack && _rack->has_focus ()) {
		switch (ev->keyval) {
		case GDK_Delete:
		case GDK_KP_Delete:
		case GDK_BackSpace:
			rack_remove_selected ();
			return true;
		case GDK_Return:
		case GDK_KP_Enter:
			rack_open (_rack_sel.lock ());
			return true;
		default:
			if (!primary && !ev->is_modifier) {
				/* anything else is for the search */
				_rack_sel.reset ();
				_rack->queue_draw ();
				_search.grab_focus ();
				_search.set_position (-1);
			}
			break;
		}
	}
	return Gtk::Window::on_key_press_event (ev);
}
