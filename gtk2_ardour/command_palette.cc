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
#include <cctype>
#include <set>

#include <glibmm/markup.h>
#include <ydk/gdkkeysyms.h>

#include <ytkmm/eventbox.h>

#include "pbd/whitespace.h"
#include "pbd/xml++.h"

#include "ardour/audioengine.h"
#include "ardour/location.h"
#include "ardour/midi_track.h"
#include "ardour/plugin.h"
#include "ardour/plugin_insert.h"
#include "ardour/plugin_manager.h"
#include "ardour/rc_configuration.h"
#include "ardour/route.h"
#include "ardour/session.h"
#include "ardour/track.h"

#include "gtkmm2ext/actions.h"
#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/colors.h"
#include "gtkmm2ext/utils.h"

#include "ardour_message.h"
#include "ardour_ui.h"
#include "command_palette.h"
#include "gui_thread.h"
#include "mixer_strip.h"
#include "mixer_ui.h"
#include "public_editor.h"
#include "route_time_axis.h"
#include "selection.h"
#include "text_match.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace std;
using namespace ARDOUR;

std::map<std::string, std::string> CommandPalette::_key_cache;

static CommandPalette* the_palette = 0;

void
CommandPalette::toggle ()
{
	if (!the_palette) {
		the_palette = new CommandPalette ();
	}
	if (the_palette->is_visible ()) {
		the_palette->hide ();
	} else {
		the_palette->present ();
	}
}

CommandPalette::CommandPalette ()
	: Gtk::Window (Gtk::WINDOW_TOPLEVEL)
{
	set_title (_("Search"));
	set_decorated (false);
	set_modal (true);
	set_skip_taskbar_hint (true);
	set_skip_pager_hint (true);
	set_type_hint (Gdk::WINDOW_TYPE_HINT_DIALOG);
	set_transient_for (ARDOUR_UI::instance ()->main_window ());
	set_border_width (1);

	const float scale = UIConfiguration::instance ().get_ui_scale ();

	_entry.set_has_frame (false);
	_entry.signal_changed ().connect (sigc::mem_fun (*this, &CommandPalette::refilter));
	_entry.signal_key_press_event ().connect (sigc::mem_fun (*this, &CommandPalette::entry_key_press), false);
	_entry.signal_activate ().connect ([this] () { int r = selected_row (); if (r >= 0) { run (r); } });

	_model = Gtk::ListStore::create (_columns);
	_view.set_model (_model);
	_view.set_headers_visible (false);
	_view.set_enable_search (false);
	_view.set_can_focus (false);

	Gtk::CellRendererText* text = manage (new Gtk::CellRendererText);
	text->property_ypad () = rintf (5 * scale);
	text->property_xpad () = rintf (10 * scale);
	text->property_ellipsize () = Pango::ELLIPSIZE_END;
	Gtk::TreeViewColumn* c1 = manage (new Gtk::TreeViewColumn ("", *text));
	c1->add_attribute (text->property_markup (), _columns.text);
	c1->set_expand (true);
	_view.append_column (*c1);

	Gtk::CellRendererText* key = manage (new Gtk::CellRendererText);
	key->property_xalign () = 1.0;
	key->property_xpad () = rintf (12 * scale);
	Gtk::TreeViewColumn* c2 = manage (new Gtk::TreeViewColumn ("", *key));
	c2->add_attribute (key->property_markup (), _columns.key);
	_view.append_column (*c2);

	_view.get_selection ()->set_select_function ([this] (Glib::RefPtr<Gtk::TreeModel> const&, Gtk::TreeModel::Path const& p, bool) {
		Gtk::TreeModel::iterator i = _model->get_iter (p);
		return i && (bool) (*i)[_columns.selectable];
	});
	_view.add_events (Gdk::BUTTON_RELEASE_MASK);
	_view.signal_button_release_event ().connect (sigc::mem_fun (*this, &CommandPalette::view_button_release), false);

	_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	_scroller.add (_view);
	_scroller.set_size_request (rintf (680 * scale), rintf (440 * scale));

	_footer.set_alignment (0.0, 0.5);

	Gtk::HBox* search_row = manage (new Gtk::HBox (false, 8));
	Gtk::Label* glass = manage (new Gtk::Label);
	glass->set_markup ("<span alpha=\"60%\">\xF0\x9F\x94\x8D</span>"); /* magnifying glass */
	search_row->set_border_width (rintf (10 * scale));
	search_row->pack_start (*glass, false, false);
	search_row->pack_start (_entry, true, true);

	Gtk::HBox* footer_row = manage (new Gtk::HBox);
	footer_row->set_border_width (rintf (8 * scale));
	footer_row->pack_start (_footer, true, true);

	_box.pack_start (*search_row, false, false);
	_box.pack_start (_scroller, true, true);
	_box.pack_start (*footer_row, false, false);

	/* a thin border around a panel that stands out from the editor */
	Gtk::EventBox* panel = manage (new Gtk::EventBox);
	panel->add (_box);
	add (*panel);
	UIConfiguration& uic (UIConfiguration::instance ());
	modify_bg (Gtk::STATE_NORMAL, Gtkmm2ext::gdk_color_from_rgba (uic.color ("gtk_bg_selected")));
	panel->modify_bg (Gtk::STATE_NORMAL, Gtkmm2ext::gdk_color_from_rgba (uic.color ("widget:bg")));
	_view.modify_base (Gtk::STATE_NORMAL, Gtkmm2ext::gdk_color_from_rgba (uic.color ("widget:bg")));
	_entry.modify_base (Gtk::STATE_NORMAL, Gtkmm2ext::gdk_color_from_rgba (uic.color ("widget:bg")));
	panel->show_all ();

	ARDOUR_UI_UTILS::enlarge_fonts (*this, 1.25);
	{
		/* the search text a bit larger still */
		Pango::FontDescription fd = UIConfiguration::instance ().get_NormalFont ();
		fd.set_size (rintf (fd.get_size () * 1.6));
		_entry.modify_font (fd);
	}

	Gtkmm2ext::Bindings::BindingsChanged.connect (_bindings_connection, invalidator (*this), [] (Gtkmm2ext::Bindings*) { _key_cache.clear (); }, gui_context ());

	load_recent ();
}

/* ---- finding things -------------------------------------------------- */

static std::string
clean_label (std::string s)
{
	/* drop GTK mnemonic underscores ("_Save" -> "Save") */
	std::string out;
	for (size_t i = 0; i < s.size (); ++i) {
		if (s[i] == '_') {
			if (i + 1 < s.size () && s[i + 1] == '_') {
				out += '_';
				++i;
			}
			continue;
		}
		out += s[i];
	}
	return out;
}

static std::string
nice_group (std::string const& g)
{
	static const std::map<std::string, std::string> names = {
		{ "Common", _("General") },     { "Main", _("Session") },        { "Editor", _("Editor") },
		{ "Editing", _("Editing") },    { "Region", _("Region") },       { "Transport", _("Transport") },
		{ "Mixer", _("Mixer") },        { "MIDI", _("MIDI") },           { "Notes", _("MIDI notes") },
		{ "Window", _("Window") },      { "Zoom", _("Zoom") },           { "MouseMode", _("Tools") },
		{ "Rulers", _("Rulers") },      { "Monitor Section", _("Monitor") },
	};
	std::map<std::string, std::string>::const_iterator i = names.find (g);
	return i == names.end () ? g : i->second;
}

void
CommandPalette::collect_commands ()
{
	std::vector<std::string>                 paths, labels, tooltips, keys;
	std::vector<Glib::RefPtr<Gtk::Action> > acts;
	ActionManager::get_all_actions (paths, labels, tooltips, keys, acts);

	if (_key_cache.empty ()) {
		/* shortcut for every action, from all key-binding sets */
		for (size_t n = 0; n < acts.size (); ++n) {
			for (Gtkmm2ext::Bindings* b : Gtkmm2ext::Bindings::bindings) {
				Gtkmm2ext::Bindings::Operation op;
				std::string k = b->get_binding_for_action (acts[n], op).display_label ();
				if (!k.empty ()) {
					_key_cache[paths[n]] = k;
					break;
				}
			}
		}
		_key_cache[""] = ""; /* mark as filled */
	}

	std::set<std::string> seen;

	for (size_t n = 0; n < acts.size (); ++n) {
		Glib::RefPtr<Gtk::Action> act = acts[n];
		std::string label = clean_label (labels[n]);

		if (label.empty ()) {
			continue;
		}
		if (paths[n].find ("SessionDialog") == 0 || paths[n].find ("RecentSessions") == 0) {
			continue;
		}

		std::string group = paths[n].substr (0, paths[n].find ('/'));
		std::map<std::string, std::string>::const_iterator k = _key_cache.find (paths[n]);
		std::string key = (k == _key_cache.end ()) ? std::string () : k->second;

		/* the same command can be registered in several places; show it once */
		if (!seen.insert (label).second) {
			continue;
		}

		Item i;
		i.kind   = Command;
		i.title  = label;
		i.where  = nice_group (group);
		i.key    = key;
		i.id     = "a:" + paths[n];
		i.action = act;
		i.available = act->get_sensitive ();
		if (!i.available) {
			i.where += " · " + std::string (_("not available right now"));
		}
		_items.push_back (i);
	}
}

void
CommandPalette::collect_tracks ()
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s) {
		return;
	}
	std::shared_ptr<RouteList const> routes = s->get_routes ();
	for (std::shared_ptr<Route> const& r : *routes) {
		if (r->is_master () || r->is_monitor () || r->is_surround_master () || r->is_hidden ()) {
			continue;
		}
		Item i;
		i.kind  = Track;
		i.title = r->name ();
		i.where = std::dynamic_pointer_cast<ARDOUR::Track> (r) ? _("Select and show this track") : _("Select and show this bus");
		i.id    = "t:" + r->id ().to_s ();
		i.route = r;
		_items.push_back (i);
	}
}

void
CommandPalette::collect_markers ()
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s) {
		return;
	}
	for (Location* l : s->locations ()->list ()) {
		if (!(l->is_mark () || l->is_section ()) || l->is_xrun () || l->is_cue_marker () || l->is_session_range ()) {
			continue;
		}
		Item i;
		i.kind  = Marker;
		i.title = l->name ();
		i.where = _("Move the playhead here");
		i.id    = "m:" + l->id ().to_s ();
		i.pos   = l->start ();
		_items.push_back (i);
	}
}

void
CommandPalette::collect_plugins ()
{
	PluginManager& pm = PluginManager::instance ();
	PluginInfoList all;
	for (PluginInfoList const* l : { &pm.lv2_plugin_info (), &pm.vst3_plugin_info (), &pm.lxvst_plugin_info (), &pm.ladspa_plugin_info (),
	                                 &pm.lua_plugin_info (), &pm.windows_vst_plugin_info (), &pm.mac_vst_plugin_info (), &pm.au_plugin_info () }) {
		all.insert (all.end (), l->begin (), l->end ());
	}
	std::set<std::string> seen;
	for (PluginInfoPtr const& p : all) {
		PluginManager::PluginStatusType st = PluginManager::instance ().get_status (p);
		if (st == PluginManager::Hidden || st == PluginManager::Concealed) {
			continue;
		}
		if (!seen.insert (p->name).second) {
			continue; /* same plugin in several formats */
		}
		Item i;
		i.kind   = Plugin;
		i.title  = p->name;
		i.where  = p->is_instrument () ? _("Instrument: goes on the selected MIDI track, or a new one") : _("Effect: goes on the selected track");
		i.id     = "p:" + p->unique_id;
		i.plugin = p;
		_items.push_back (i);
	}
}

void
CommandPalette::collect ()
{
	_items.clear ();
	collect_commands ();
	collect_tracks ();
	collect_markers ();
	collect_plugins ();
}

std::string
CommandPalette::highlighted (Item const& it) const
{
	std::set<size_t>         h (it.hits.begin (), it.hits.end ());
	std::vector<std::string> chars;
	TextMatch::split_chars (it.title, chars);
	std::string out;
	bool        on = false;
	for (size_t i = 0; i < chars.size (); ++i) {
		const bool want = h.count (i) > 0;
		if (want != on) {
			/* no colour of its own: it has to stay readable on the selected row too */
			out += want ? "<span weight=\"heavy\" underline=\"single\">" : "</span>";
			on = want;
		}
		out += Gtkmm2ext::markup_escape_text (chars[i]);
	}
	if (on) {
		out += "</span>";
	}
	std::vector<std::string> where;
	TextMatch::split_chars (it.where, where); /* nothing but proper text reaches the markup */
	std::string w;
	for (std::string const& c : where) {
		w += c;
	}
	return string_compose ("<span weight=\"bold\">%1</span>\n<span size=\"smaller\" alpha=\"65%%\">%2</span>", out, Gtkmm2ext::markup_escape_text (w));
}

void
CommandPalette::refilter ()
{
	std::string q = _entry.get_text ();
	PBD::strip_whitespace_edges (q);

	_shown.clear ();
	_model->clear ();

	static const char* headers[] = { "", N_("COMMANDS"), N_("TRACKS"), N_("MARKERS"), N_("PLUGINS") };
	std::vector<Item> found;

	if (q.empty ()) {
		/* recently used, newest first */
		for (std::string const& id : _recent) {
			for (Item const& it : _items) {
				if (it.id == id) {
					found.push_back (it);
					found.back ().hits.clear ();
					break;
				}
			}
		}
		if (!found.empty ()) {
			Item h;
			h.kind  = Header;
			h.title = _("RECENTLY USED");
			_shown.push_back (h);
		}
		for (Item const& it : found) {
			_shown.push_back (it);
		}
	} else {
		for (Item const& it : _items) {
			Item c = it;
			if (TextMatch::match (c.title, q, c.hits, c.score)) {
				if (!c.available) {
					c.score -= 250;
				}
				found.push_back (c);
			}
		}
		/* when the search text appears as a whole somewhere, don't show
		 * the looser letter-by-letter matches */
		const size_t q_chars = g_utf8_strlen (q.c_str (), -1);
		auto whole = [q_chars] (Item const& it) {
			/* the matched letters are one unbroken piece of the title */
			return !it.hits.empty () && it.hits.back () - it.hits.front () + 1 == q_chars;
		};
		bool any_whole = false;
		for (Item const& it : found) {
			any_whole |= whole (it);
		}
		if (any_whole) {
			found.erase (std::remove_if (found.begin (), found.end (), [&whole] (Item const& it) { return !whole (it); }), found.end ());
		}

		/* groups in the order of their best result, best results first */
		std::map<int, int> best;
		for (Item const& it : found) {
			best[it.kind] = std::max (best.count (it.kind) ? best[it.kind] : -100000, it.score);
		}
		std::stable_sort (found.begin (), found.end (), [&best] (Item const& a, Item const& b) {
			if (a.kind != b.kind) {
				return best[a.kind] > best[b.kind];
			}
			return a.score > b.score;
		});
		/* at most 12 of each kind */
		std::map<int, int> per_kind;
		int last = -1;
		for (Item const& it : found) {
			if (++per_kind[it.kind] > 12) {
				continue;
			}
			if (it.kind != last) {
				Item h;
				h.kind  = Header;
				h.title = _(headers[it.kind]);
				_shown.push_back (h);
				last = it.kind;
			}
			_shown.push_back (it);
		}
	}

	for (size_t n = 0; n < _shown.size (); ++n) {
		Item const& it = _shown[n];
		Gtk::TreeModel::Row row = *_model->append ();
		row[_columns.index] = n;
		if (it.kind == Header) {
			row[_columns.text]       = string_compose ("<span size=\"small\" weight=\"bold\" alpha=\"55%%\">%1</span>", Gtkmm2ext::markup_escape_text (it.title));
			row[_columns.key]        = "";
			row[_columns.selectable] = false;
		} else {
			row[_columns.text]       = it.available ? highlighted (it) : string_compose ("<span foreground=\"#8a919c\">%1</span>", highlighted (it));
			row[_columns.key]        = it.key.empty () ? std::string () : string_compose ("<span font_family=\"monospace\" alpha=\"80%%\">%1</span>", Gtkmm2ext::markup_escape_text (it.key));
			row[_columns.selectable] = true;
		}
	}

	int results = 0;
	for (Item const& it : _shown) {
		results += it.kind != Header;
	}
	std::string count;
	if (q.empty ()) {
		count = _recent.empty () ? _("Type to search commands, tracks, markers and plugins") : _("Recently used");
	} else if (results == 0) {
		count = string_compose (_("Nothing matches \"%1\""), q);
	} else {
		count = string_compose (P_("%1 result", "%1 results", results), results);
	}
	_footer.set_markup (string_compose ("<span alpha=\"65%%\">%1    \xE2\x86\x91 \xE2\x86\x93 %2    Enter %3    Esc %4</span>",
	                                    Gtkmm2ext::markup_escape_text (count), _("move"), _("run"), _("close")));

	/* select the first real result, with its heading in view */
	select_row (-1);
	step (1);
	if (_scroller.get_vadjustment ()) {
		_scroller.get_vadjustment ()->set_value (0);
	}
}

/* ---- moving and running ---------------------------------------------- */

int
CommandPalette::selected_row () const
{
	Gtk::TreeModel::iterator i = const_cast<Gtk::TreeView&> (_view).get_selection ()->get_selected ();
	if (!i) {
		return -1;
	}
	return (*i)[_columns.index];
}

void
CommandPalette::select_row (int r)
{
	if (r < 0 || r >= (int) _shown.size ()) {
		_view.get_selection ()->unselect_all ();
		return;
	}
	Gtk::TreeModel::Path p;
	p.push_back (r);
	_view.get_selection ()->select (p);
	_view.scroll_to_row (p);
}

void
CommandPalette::step (int delta)
{
	int r = selected_row ();
	for (int n = r + delta; n >= 0 && n < (int) _shown.size (); n += delta) {
		if (_shown[n].kind != Header) {
			select_row (n);
			return;
		}
	}
	if (r >= 0 && delta < 0) {
		/* already at the first result: show its header too */
		_view.scroll_to_point (0, 0);
	}
}

void
CommandPalette::run (size_t n)
{
	if (n >= _shown.size () || _shown[n].kind == Header) {
		return;
	}
	Item const it = _shown[n];
	hide ();
	remember (it.id);

	Session*       s  = ARDOUR_UI::instance ()->the_session ();
	PublicEditor&  ed = ARDOUR_UI::instance ()->the_editor ();

	switch (it.kind) {
	case Command:
		if (it.action) {
			it.action->activate ();
		}
		break;

	case Track:
		if (std::shared_ptr<Route> r = it.route.lock ()) {
			if (TimeAxisView* tav = ed.time_axis_view_from_stripable (r)) {
				ed.set_selected_track (*tav);
				ed.ensure_time_axis_view_is_visible (*tav, false);
			}
		}
		break;

	case Marker:
		if (s) {
			s->request_locate (it.pos.samples ());
		}
		break;

	case Plugin:
		add_plugin (it.plugin);
		break;

	case Header:
		break;
	}
}

/* ---- plugins ---------------------------------------------------------- */

/* a message in the middle of the screen, in readable type */
static void
say (std::string const& text)
{
	ArdourMessageDialog msg (string_compose ("<span size=\"large\">%1</span>", Gtkmm2ext::markup_escape_text (text)), true);
	msg.set_position (Gtk::WIN_POS_CENTER);
	ARDOUR_UI_UTILS::enlarge_fonts (msg, 1.25);
	msg.run ();
}

/* the mixer strip of a track that was only just made may not exist yet: try again a few times */
static bool
open_plugin_window (std::weak_ptr<Route> wr, std::weak_ptr<Processor> wp, int tries)
{
	std::shared_ptr<Route>        r  = wr.lock ();
	std::shared_ptr<PluginInsert> pi = std::dynamic_pointer_cast<PluginInsert> (wp.lock ());
	if (!r || !pi || !AudioEngine::instance ()->running ()) {
		return false;
	}
	MixerStrip* strip = Mixer_UI::instance ()->strip_for_route (r);
	if (!strip) {
		if (tries > 0) {
			Glib::signal_timeout ().connect (sigc::bind (sigc::ptr_fun (&open_plugin_window), wr, wp, tries - 1), 250);
		}
		return false;
	}
	if (pi->plugin ()->has_editor ()) {
		strip->edit_processor (pi);
	}
	return false;
}

/** A plugin picked from the search goes where it belongs:
 *  - an effect, on every selected track or bus that carries audio;
 *    with none selected, we say so and do nothing;
 *  - an instrument, on every selected MIDI track that has no instrument yet;
 *    with none, on a new MIDI track made for it.
 */
void
CommandPalette::add_plugin (PluginInfoPtr info)
{
	Session* s = ARDOUR_UI::instance ()->the_session ();
	if (!s || !info) {
		return;
	}
	PublicEditor& ed         = ARDOUR_UI::instance ()->the_editor ();
	const bool    instrument = info->is_instrument ();

	std::vector<std::shared_ptr<Route> > targets;
	for (TimeAxisView* tav : ed.get_selection ().tracks) {
		RouteTimeAxisView* rtv = dynamic_cast<RouteTimeAxisView*> (tav);
		if (!rtv || !rtv->route () || rtv->route ()->is_monitor ()) {
			continue;
		}
		std::shared_ptr<Route> r = rtv->route ();
		if (instrument) {
			if (rtv->is_midi_track () && !r->the_instrument ()) {
				targets.push_back (r);
			}
		} else if (!rtv->is_midi_track () || r->the_instrument ()) {
			targets.push_back (r);
		}
	}

	std::shared_ptr<Route>     shown_route;
	std::shared_ptr<Processor> shown;
	size_t                     added = 0;

	if (targets.empty () && !instrument) {
		say (s->naudiotracks () == 0
		     ? string_compose (_("%1 is an effect, and this session has no audio track yet.\n\nAdd an audio track, select it, then search for the effect again."), info->name)
		     : string_compose (_("%1 is an effect: it needs a track to go on.\n\nSelect an audio track, then search for the effect again."), info->name));
		return;
	}

	if (targets.empty ()) {
		/* an instrument and no MIDI track waiting for one: make the track too */
		const ChanCount one_midi (DataType::MIDI, 1);
		std::list<std::shared_ptr<MidiTrack> > made;
		try {
			made = s->new_midi_track (one_midi, one_midi, Config->get_strict_io (), info, (ARDOUR::Plugin::PresetRecord*) 0,
			                          std::shared_ptr<RouteGroup> (), 1, info->name, PresentationInfo::max_order, ARDOUR::Normal, true);
		} catch (...) {
		}
		if (made.empty ()) {
			say (string_compose (_("A MIDI track for %1 could not be made."), info->name));
			return;
		}
		shown_route = made.front ();
		shown       = made.front ()->the_instrument ();
		added       = 1;
	} else {
		for (std::shared_ptr<Route> const& r : targets) {
			PluginPtr p = info->load (*s);
			if (!p) {
				continue;
			}
			std::shared_ptr<Processor> processor (new PluginInsert (*s, *r, p));
			Route::ProcessorStreams    err;
			/* an instrument goes first, an effect last before the fader */
			const int rv = instrument ? r->add_processor_by_index (processor, 0, &err, Config->get_new_plugins_active ())
			                          : r->add_processor (processor, PreFader, &err, Config->get_new_plugins_active ());
			if (rv == 0) {
				shown_route = r;
				shown       = processor;
				++added;
			}
		}
		if (added == 0) {
			say (string_compose (_("%1 could not be added: its inputs and outputs do not fit the track."), info->name));
			return;
		}
	}

	PluginManager::instance ().stats_use_plugin (info);

	if (added == 1 && shown && UIConfiguration::instance ().get_open_gui_after_adding_plugin ()) {
		Glib::signal_timeout ().connect (sigc::bind (sigc::ptr_fun (&open_plugin_window), std::weak_ptr<Route> (shown_route), std::weak_ptr<Processor> (shown), 12), 150);
	}
}

/* ---- recently used --------------------------------------------------- */

void
CommandPalette::remember (std::string const& id)
{
	_recent.erase (std::remove (_recent.begin (), _recent.end (), id), _recent.end ());
	_recent.insert (_recent.begin (), id);
	if (_recent.size () > 8) {
		_recent.resize (8);
	}
	save_recent ();
}

void
CommandPalette::load_recent ()
{
	_recent.clear ();
	XMLNode* node = Config->instant_xml (X_("CommandPaletteRecent"));
	if (!node) {
		return;
	}
	for (XMLNode const* c : node->children ()) {
		std::string id;
		if (c->get_property (X_("id"), id)) {
			_recent.push_back (id);
		}
	}
}

void
CommandPalette::save_recent ()
{
	XMLNode node (X_("CommandPaletteRecent"));
	for (std::string const& id : _recent) {
		/* tracks and markers belong to one session; only keep commands and plugins */
		if (id.compare (0, 2, "a:") == 0 || id.compare (0, 2, "p:") == 0) {
			XMLNode* c = node.add_child (X_("Item"));
			c->set_property (X_("id"), id);
		}
	}
	Config->add_instant_xml (node);
}

/* ---- window behaviour ------------------------------------------------ */

void
CommandPalette::on_show ()
{
	collect ();
	_entry.set_text ("");
	refilter ();

	/* centred near the top of the main window */
	Gtk::Window& mw = ARDOUR_UI::instance ()->main_window ();
	int mx, my, mw_w, mw_h;
	mw.get_position (mx, my);
	mw.get_size (mw_w, mw_h);
	int w, h;
	get_size (w, h);
	move (mx + std::max (0, (mw_w - w) / 2), my + std::min (120, mw_h / 8));

	Gtk::Window::on_show ();
	_entry.grab_focus ();
}

bool
CommandPalette::entry_key_press (GdkEventKey* ev)
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
	case GDK_Escape:
		hide ();
		return true;
	}
	return false;
}

bool
CommandPalette::on_key_press_event (GdkEventKey* ev)
{
	if (ev->keyval == GDK_Escape) {
		hide ();
		return true;
	}
	/* Ctrl+Space again closes it */
	if (ev->keyval == GDK_space && (ev->state & GDK_CONTROL_MASK)) {
		hide ();
		return true;
	}
	return Gtk::Window::on_key_press_event (ev);
}

bool
CommandPalette::view_button_release (GdkEventButton* ev)
{
	if (ev->button != 1) {
		return false;
	}
	Gtk::TreeModel::Path path;
	Gtk::TreeViewColumn* col;
	int cx, cy;
	if (_view.get_path_at_pos ((int) ev->x, (int) ev->y, path, col, cx, cy)) {
		run (path[0]); /* one click runs it */
		return true;
	}
	return false;
}

bool
CommandPalette::on_focus_out_event (GdkEventFocus* ev)
{
	/* clicking elsewhere closes it */
	hide ();
	return Gtk::Window::on_focus_out_event (ev);
}
