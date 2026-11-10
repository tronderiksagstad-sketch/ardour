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

/* Editor side of "Quantize Audio to Grid": collect the regions, pick the
 * grid, and hand over to AudioWarp. */

#include <ytkmm/label.h>
#include <ytkmm/adjustment.h>
#include <ytkmm/scale.h>
#include <ytkmm/stock.h>

#include "ardour/audioregion.h"

#include "ardour_dialog.h"
#include "audio_region_view.h"
#include "audio_warp.h"
#include "editor.h"
#include "gui_thread.h"
#include "mouse_cursors.h"
#include "ui_config.h"
#include "utils.h"

#include "pbd/i18n.h"

using namespace ARDOUR;
using namespace Temporal;

/* the editor's grid as a note length; a 1/16 note when the grid is off */
static Beats
quantize_grid (Editor const& ed, timepos_t const& pos)
{
	bool  ok = false;
	Beats g  = ed.get_grid_type_as_beats (ok, pos);
	if (!ok || g <= Beats ()) {
		g = Beats::ticks (ticks_per_beat / 4);
	}
	return g;
}

static std::string
grid_name (Beats const& g)
{
	const double quarters = (double) g.to_ticks () / ticks_per_beat;
	if (quarters >= 1.0 && quarters == floor (quarters)) {
		return quarters == 1.0 ? _("1/4 note") : string_compose (_("%1 beats"), (int) quarters);
	}
	const int div = (int) llround (4.0 / quarters);
	return string_compose (_("1/%1 note"), div);
}

void
Editor::quantize_audio_to_grid ()
{
	RegionSelection rs = get_regions_from_selection_and_entered ();
	std::vector<std::shared_ptr<AudioRegion> > regions;
	for (RegionView* rv : rs) {
		if (std::shared_ptr<AudioRegion> ar = std::dynamic_pointer_cast<AudioRegion> (rv->region ())) {
			regions.push_back (ar);
		}
	}
	if (regions.empty ()) {
		return;
	}

	const Beats  grid     = quantize_grid (*this, regions.front ()->position ());
	const double strength = std::max (0, std::min (100, UIConfiguration::instance ().get_audio_quantize_strength ())) / 100.0;

	/* this takes a moment for long regions */
	CursorRAII busy (*this, cursors ()->wait);
	gdk_flush ();
	AudioWarp::quantize (regions, grid, strength);
}

void
Editor::audio_quantize_settings ()
{
	RegionSelection rs = get_regions_from_selection_and_entered ();
	const Beats grid = quantize_grid (*this, rs.empty () ? timepos_t () : rs.front ()->region ()->position ());

	ArdourDialog d (_("Audio Quantize Settings"), true);
	Gtk::VBox* v = d.get_vbox ();
	v->set_spacing (12);
	v->set_border_width (16);

	Gtk::Label* g = manage (new Gtk::Label);
	g->set_markup (string_compose (_("<b>Grid:</b> %1, from the editor's Grid setting"), grid_name (grid)));
	g->set_alignment (0.0, 0.5);

	Gtk::Label* sl = manage (new Gtk::Label);
	sl->set_markup (_("<b>Strength</b>: 100% puts every hit exactly on the grid; less keeps some feel"));
	sl->set_alignment (0.0, 0.5);

	Gtk::Adjustment adj (UIConfiguration::instance ().get_audio_quantize_strength (), 0, 100, 5, 10, 0);
	Gtk::HScale     strength (adj);
	strength.set_digits (0);
	strength.set_value_pos (Gtk::POS_RIGHT);
	strength.set_size_request (360, -1);

	Gtk::Label* how = manage (new Gtk::Label (_("Right-click a region > Quantize Audio to Grid uses these settings.\n"
	                                            "Afterwards, drag the blue markers on the region to move single hits.")));
	how->set_alignment (0.0, 0.5);

	v->pack_start (*g, false, false);
	v->pack_start (*sl, false, false);
	v->pack_start (strength, false, false);
	v->pack_start (*how, false, false);

	d.add_button (Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	if (!rs.empty ()) {
		d.add_button (_("Quantize now"), Gtk::RESPONSE_ACCEPT);
	}
	d.add_button (_("Save"), Gtk::RESPONSE_OK);
	d.set_default_response (rs.empty () ? Gtk::RESPONSE_OK : Gtk::RESPONSE_ACCEPT);
	v->show_all ();
	ARDOUR_UI_UTILS::enlarge_fonts (d, 1.2);

	const int r = d.run ();
	if (r == Gtk::RESPONSE_CANCEL || r == Gtk::RESPONSE_DELETE_EVENT) {
		return;
	}
	UIConfiguration::instance ().set_audio_quantize_strength ((int32_t) strength.get_value ());
	d.hide ();
	if (r == Gtk::RESPONSE_ACCEPT) {
		quantize_audio_to_grid ();
	}
}
