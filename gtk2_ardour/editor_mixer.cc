/*
 * Copyright (C) 2005-2008 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2005-2017 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2011 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2011 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2014-2017 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015-2016 Tim Mayberry <mojofunk@gmail.com>
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

#ifdef WAF_BUILD
#include "gtk2ardour-config.h"
#endif

#include <glibmm/miscutils.h>
#include <ytkmm/messagedialog.h>
#include <gtkmm2ext/utils.h>
#include <gtkmm2ext/window_title.h>

#include "pbd/enumwriter.h"

#include "ardour/rc_configuration.h"

#include "control_protocol/control_protocol.h"

#include "actions.h"
#include "ardour_ui.h"
#include "audio_time_axis.h"
#include "automation_time_axis.h"
#include "editor.h"
#include "editor_route_groups.h"
#include "editor_regions.h"
#include "enums_convert.h"
#include "gui_thread.h"
#include "midi_time_axis.h"
#include "mixer_strip.h"
#include "mixer_ui.h"
#include "selection.h"
#include "ui_config.h"

#include "widgets/tooltips.h"

#include "pbd/i18n.h"

using namespace std;
using namespace Gtkmm2ext;
using namespace PBD;

void
Editor::showhide_att_left (bool yn)
{
	Tabbable::showhide_att_left (yn);
	show_editor_mixer (yn);
}

void
Editor::show_editor_mixer (bool yn)
{
	std::shared_ptr<ARDOUR::Route> r;

	show_editor_mixer_when_tracks_arrive = false;

	if (yn) {
		Gtk::Window* toplevel = current_toplevel();
		Glib::RefPtr<Gdk::Window> win;
		Glib::RefPtr<Gdk::Screen> screen;

		if (toplevel) {
			win = toplevel->get_window();
		}

		if (win) {
			screen = win->get_screen();
		} else {
			screen = Gdk::Screen::get_default();
		}

		if (g_getenv ("ARDOUR_LOVES_STUPID_TINY_SCREENS") == 0 && screen && screen->get_height() < 700) {
			Gtk::MessageDialog msg (_("This screen is not tall enough to display the editor mixer"));
			msg.run ();
			return;
		}
	}

	if (!_session) {
		show_editor_mixer_when_tracks_arrive = yn;
		return;
	}

	if (yn) {

		if (selection->tracks.empty()) {

			if (track_views.empty()) {
				show_editor_mixer_when_tracks_arrive = true;
				return;
			}

			for (TrackViewList::iterator i = track_views.begin(); i != track_views.end(); ++i) {
				RouteTimeAxisView* atv;

				if ((atv = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {
					r = atv->route();
					if (r && r->is_hidden ()) {
						r.reset ();
						continue;
					}
					break;
				}
			}

		} else {
			sort_track_selection (selection->tracks);

			for (TrackSelection::iterator i = selection->tracks.begin(); i != selection->tracks.end(); ++i) {
				RouteTimeAxisView* atv;

				if ((atv = dynamic_cast<RouteTimeAxisView*> (*i)) != 0) {
					r = atv->route();
					break;
				}
			}
		}

		if (r) {
			if (current_mixer_strip == 0) {
				create_editor_mixer ();
			}
		}

		if (current_mixer_strip && inspector_box.get_parent() == 0) {
			content_att_left.add (inspector_box);
			inspector_box.show ();
			current_mixer_strip->show ();
		}

		if (r) {
			current_mixer_strip->set_route (r);
			current_mixer_strip->set_width_enum (editor_mixer_strip_width, (void*) this);
			update_inspector_header ();
		}

	} else {

		if (inspector_box.get_parent() != 0) {
			content_att_left.remove ();
		}
	}

#ifdef __APPLE__
	/* XXX gtk problem here */
	ensure_all_elements_drawn();
#endif
}

std::shared_ptr<ARDOUR::Route>
Editor::current_mixer_stripable () const
{
	if (current_mixer_strip) {
		return current_mixer_strip->route ();
	}
	return std::shared_ptr<ARDOUR::Route> ();
}

#ifdef __APPLE__
void
Editor::ensure_all_elements_drawn ()
{
	controls_layout.queue_draw ();
	time_bars_event_box.queue_draw ();
}
#endif

void
Editor::create_editor_mixer ()
{
	current_mixer_strip = new MixerStrip (*ARDOUR_UI::instance()->the_mixer(), _session, false);
	current_mixer_strip->Hiding.connect (sigc::mem_fun(*this, &Editor::current_mixer_strip_hidden));
	current_mixer_strip->WidthChanged.connect (sigc::mem_fun (*this, &Editor::mixer_strip_width_changed));

#ifdef __APPLE__
	current_mixer_strip->WidthChanged.connect (sigc::mem_fun(*this, &Editor::ensure_all_elements_drawn));
#endif
	current_mixer_strip->set_embedded (true);

	setup_inspector ();
	inspector_box.pack_start (*current_mixer_strip, true, true);
}

void
Editor::setup_inspector ()
{
	if (inspector_header.get_parent ()) {
		return; /* already built */
	}

	inspector_caption.set_markup (string_compose ("<span size=\"smaller\" alpha=\"70%%\">%1</span>", Gtkmm2ext::markup_escape_text (_("Selected track"))));
	inspector_caption.set_alignment (0.0, 0.5);
	inspector_caption.set_name (X_("InspectorCaption"));

	inspector_name.set_alignment (0.0, 0.5);
	inspector_name.set_ellipsize (Pango::ELLIPSIZE_END);
	inspector_name.set_name (X_("InspectorName"));

	Gtk::VBox* labels = manage (new Gtk::VBox);
	labels->set_spacing (1);
	labels->pack_start (inspector_caption, false, false);
	labels->pack_start (inspector_name, false, false);

	inspector_mixer_button.set_name ("generic button");
	inspector_mixer_button.set_text (_("Open mixer"));
	inspector_mixer_button.set_corner_radius (5);
	inspector_mixer_button.set_related_action (ActionManager::get_action (X_("Common"), X_("show-mixer")));
	ArdourWidgets::set_tooltip (inspector_mixer_button, _("Show the Mixer window"));

	inspector_header.set_spacing (6);
	inspector_header.set_border_width (6);
	inspector_header.pack_start (*labels, true, true);
	inspector_header.pack_end (inspector_mixer_button, false, false);
	inspector_header.show_all ();

	inspector_box.set_spacing (2);
	inspector_box.pack_start (inspector_header, false, false);
}

void
Editor::update_inspector_header ()
{
	inspector_name_connection.disconnect ();

	std::shared_ptr<ARDOUR::Route> r = current_mixer_strip ? current_mixer_strip->route () : std::shared_ptr<ARDOUR::Route> ();

	if (!r) {
		inspector_name.set_text ("");
		return;
	}

	inspector_name.set_markup (string_compose ("<b>%1</b>", Gtkmm2ext::markup_escape_text (r->name ())));

	r->PropertyChanged.connect (inspector_name_connection, invalidator (*this),
	                            std::bind (&Editor::update_inspector_header, this), gui_context ());
}

void
Editor::set_selected_mixer_strip (TimeAxisView& view)
{
	if (!_session) {
		return;
	}

	// if this is an automation track, then the mixer strip should
	// show the parent

	std::shared_ptr<ARDOUR::Stripable> stripable;
	AutomationTimeAxisView* atv;

	if ((atv = dynamic_cast<AutomationTimeAxisView*>(&view)) != 0) {
		AudioTimeAxisView *parent = dynamic_cast<AudioTimeAxisView*>(view.get_parent());
		if (parent) {
			stripable = parent->stripable ();
		}
	} else {
		StripableTimeAxisView* stav = dynamic_cast<StripableTimeAxisView*> (&view);
		if (stav) {
			stripable = stav->stripable();
		}
	}

	Glib::RefPtr<Gtk::Action> act = ActionManager::get_action (X_("Editor"), X_("show-editor-mixer"));

	if (act) {
		Glib::RefPtr<Gtk::ToggleAction> tact = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(act);
		if (!tact || !tact->get_active()) {
			/* not showing mixer strip presently */
			return;
		}
	}

	if (current_mixer_strip == 0) {
		create_editor_mixer ();
	}

	std::shared_ptr<ARDOUR::Route> route = std::dynamic_pointer_cast<ARDOUR::Route> (stripable);
	if (current_mixer_strip->route() == route) {
		return;
	}

	if (route) {
		current_mixer_strip->set_route (route);
		current_mixer_strip->set_width_enum (editor_mixer_strip_width, (void*) this);
		update_inspector_header ();
	}
}

void
Editor::current_mixer_strip_hidden ()
{
	Glib::RefPtr<Gtk::Action> act = ActionManager::get_action (X_("Editor"), X_("show-editor-mixer"));
	if (act) {
		Glib::RefPtr<Gtk::ToggleAction> tact = Glib::RefPtr<Gtk::ToggleAction>::cast_dynamic(act);
		tact->set_active (false);
	}
}

void
Editor::maybe_add_mixer_strip_width (XMLNode& node) const
{
	if (current_mixer_strip) {
		node.set_property ("mixer-width", editor_mixer_strip_width);
	}
}

void
Editor::mixer_strip_width_changed ()
{
#ifdef __APPLE__
	ensure_all_elements_drawn ();
#endif

	editor_mixer_strip_width = current_mixer_strip->get_width_enum ();
}
