/*
 * Copyright (C) 2013-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2014-2015 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2014-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2014 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2015 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2017 Ben Loftis <ben@harrisonconsoles.com>
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

#include <algorithm>

#include <glib.h>
#include "pbd/gstdio_compat.h"
#include <glibmm.h>

#include <ytkmm/cellrendererprogress.h>
#include <ytkmm/filechooser.h>
#include <ytkmm/stock.h>
#include <ytkmm/alignment.h>
#include <ytkmm/drawingarea.h>
#include <ytkmm/eventbox.h>

#include "pbd/basename.h"
#include "pbd/downloader.h"
#include "pbd/failed_constructor.h"
#include "pbd/file_utils.h"
#include "pbd/replace_all.h"
#include "pbd/whitespace.h"
#include "pbd/stl_delete.h"
#include "pbd/openuri.h"
#include "pbd/xml++.h"

#include "temporal/superclock.h"
#include "temporal/timeline.h"

#include "gtkmm2ext/utils.h"
#include "gtkmm2ext/keyboard.h"
#include "gtkmm2ext/colors.h"

#include "widgets/tooltips.h"

#include "ardour/audioengine.h"
#include "ardour/demo_sessions.h"
#include "ardour/filesystem_paths.h"
#include "ardour/library.h"
#include "ardour/luascripting.h"
#include "ardour/recent_sessions.h"
#include "ardour/session.h"
#include "ardour/session_state_utils.h"
#include "ardour/template_utils.h"
#include "ardour/filename_extensions.h"

#include "LuaBridge/LuaBridge.h"

#include "ardour_message.h"
#include "ardour_ui.h"
#include "context_menu_helper.h"
#include "session_dialog.h"
#include "opts.h"
#include "engine_dialog.h"
#include "pbd/i18n.h"
#include "ui_config.h"
#include "utils.h"
#include "gtk2ardour-version.h"

using namespace std;
using namespace Gtk;
using namespace Gdk;
using namespace Glib;
using namespace PBD;
using namespace ARDOUR;
using namespace ArdourWidgets;
using namespace ARDOUR_UI_UTILS;

namespace {

/* Colours used for the little picture next to each recent session. */
const Gtkmm2ext::Color recent_palette[] = { 0x4fd1a5ff, 0xf2a03dff, 0x5aa9ffff, 0xe5c84aff, 0xb48cffff };

/** One row in the start window's "Recent" list: a coloured waveform
 * picture, the session name and a line of details. A single click opens
 * the session; Ctrl-click opens it with all plugins disabled (safe mode).
 */
class RecentSessionRow : public Gtk::EventBox
{
public:
	RecentSessionRow (std::string const& name, std::string const& details, std::string const& tip, Gtkmm2ext::Color color)
		: _color (color)
		, _seed (std::hash<std::string> () (name))
	{
		set_can_focus (true);
		add_events (Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK | Gdk::KEY_PRESS_MASK);

		const float scale = UIConfiguration::instance ().get_ui_scale ();
		_thumb.set_size_request (rintf (88 * scale), rintf (52 * scale));
		_thumb.signal_expose_event ().connect (sigc::mem_fun (*this, &RecentSessionRow::thumb_expose));

		_name.set_markup (string_compose ("<span size=\"large\" weight=\"bold\">%1</span>", Gtkmm2ext::markup_escape_text (name)));
		_name.set_alignment (0.0, 0.5);
		_name.set_ellipsize (Pango::ELLIPSIZE_END);
		_details.set_markup (string_compose ("<span alpha=\"65%%\">%1</span>", Gtkmm2ext::markup_escape_text (details)));
		_details.set_alignment (0.0, 0.5);
		_details.set_ellipsize (Pango::ELLIPSIZE_END);

		Gtk::VBox* labels = manage (new Gtk::VBox);
		labels->set_spacing (4);
		labels->pack_start (_name, false, false);
		labels->pack_start (_details, false, false);

		Gtk::Alignment* lalign = manage (new Gtk::Alignment (0.0, 0.5, 1.0, 0.0));
		lalign->add (*labels);

		Gtk::Alignment* talign = manage (new Gtk::Alignment (0.5, 0.5, 0.0, 0.0));
		talign->add (_thumb);

		_box.set_spacing (12);
		_box.set_border_width (4);
		_box.pack_start (*talign, false, false);
		_box.pack_start (*lalign, true, true);
		add (_box);

		set_tooltip (*this, tip + "\n\n" + _("Click to open. Ctrl-click to open with all plugins disabled."));
		show_all ();
	}

	/** emitted with true when the session should be opened in safe mode */
	sigc::signal<void, bool> Activated;

private:
	Gtk::HBox        _box;
	Gtk::DrawingArea _thumb;
	Gtk::Label       _name;
	Gtk::Label       _details;
	Gtkmm2ext::Color _color;
	size_t           _seed;

	bool on_enter_notify_event (GdkEventCrossing*)
	{
		modify_bg (Gtk::STATE_NORMAL, Gtkmm2ext::gdk_color_from_rgba (UIConfiguration::instance ().color ("generic button: fill")));
		return false;
	}

	bool on_leave_notify_event (GdkEventCrossing*)
	{
		unset_bg (Gtk::STATE_NORMAL);
		return false;
	}

	bool on_button_release_event (GdkEventButton* ev)
	{
		if (ev->button == 1) {
			Activated (Gtkmm2ext::Keyboard::modifier_state_equals (ev->state, Gtkmm2ext::Keyboard::PrimaryModifier));
			return true;
		}
		return false;
	}

	bool on_key_press_event (GdkEventKey* ev)
	{
		if (ev->keyval == GDK_Return || ev->keyval == GDK_KP_Enter || ev->keyval == GDK_space) {
			Activated (false);
			return true;
		}
		return Gtk::EventBox::on_key_press_event (ev);
	}

	bool thumb_expose (GdkEventExpose*)
	{
		Cairo::RefPtr<Cairo::Context> cr = _thumb.get_window ()->create_cairo_context ();
		const double w = _thumb.get_width ();
		const double h = _thumb.get_height ();

		Gtkmm2ext::rounded_rectangle (cr, 0, 0, w, h, 7);
		Gtkmm2ext::set_source_rgb_a (cr, _color, 0.14);
		cr->fill ();

		/* a decorative waveform, the same for the same session name */
		uint64_t s = _seed;
		auto rnd = [&s] () { s = s * 6364136223846793005ULL + 1442695040888963407ULL; return (double) ((s >> 33) & 0xffff) / 65535.0; };
		const int    n   = 40;
		const double mid = h * 0.5;
		std::vector<double> amp (n + 1);
		for (int i = 0; i <= n; ++i) {
			const double env = 0.4 + 0.6 * fabs (sin (i * 0.45 + (_seed & 7)));
			amp[i] = (0.08 + 0.34 * rnd () * env) * h;
		}
		cr->move_to (0, mid - amp[0]);
		for (int i = 1; i <= n; ++i) {
			cr->line_to (w * i / n, mid - amp[i]);
		}
		for (int i = n; i >= 0; --i) {
			cr->line_to (w * i / n, mid + amp[i]);
		}
		cr->close_path ();
		Gtkmm2ext::set_source_rgb_a (cr, _color, 0.85);
		cr->fill ();
		return true;
	}
};

/** "Today, 14:32", "Yesterday, 09:10", "3 Nov" or "3 Nov 2024" */
std::string
friendly_date (int64_t when)
{
	Glib::DateTime t     = Glib::DateTime::create_now_local (when);
	Glib::DateTime now   = Glib::DateTime::create_now_local ();
	/* whole calendar days between the two, also across New Year */
	const int      days  = (int) Glib::Date (now.get_day_of_month (), (Glib::Date::Month) now.get_month (), now.get_year ()).get_julian ()
	                       - (int) Glib::Date (t.get_day_of_month (), (Glib::Date::Month) t.get_month (), t.get_year ()).get_julian ();

	if (days == 0) {
		return string_compose (_("Today, %1"), std::string (t.format ("%H:%M")));
	} else if (days == 1) {
		return string_compose (_("Yesterday, %1"), std::string (t.format ("%H:%M")));
	} else if (t.get_year () == now.get_year ()) {
		return t.format ("%-d %b");
	}
	return t.format ("%-d %b %Y");
}

/** Read the session length and number of tracks from a session file.
 * Leaves the values at -1 when they can't be found. */
void
read_session_summary (std::string const& path, double& seconds, int& tracks)
{
	seconds = -1;
	tracks  = -1;

	XMLTree tree;
	if (!tree.read (path) || !tree.root ()) {
		return;
	}

	XMLNode const* root = tree.root ();

	/* positions are stored in the session's own time unit */
	superclock_t ticks_per_second = 282240000;
	if (XMLNode const* tm = root->child (X_("TempoMap"))) {
		superclock_t sc;
		if (tm->get_property (X_("superclocks-per-second"), sc) && sc > 0) {
			ticks_per_second = sc;
		}
	}

	if (XMLNode const* locs = root->child (X_("Locations"))) {
		for (XMLNode const* loc : locs->children ()) {
			std::string flags;
			if (loc->name () != X_("Location") || !loc->get_property (X_("flags"), flags) || flags.find (X_("IsSessionRange")) == std::string::npos) {
				continue;
			}
			std::string a, b;
			Temporal::timepos_t start, end;
			if (loc->get_property (X_("start"), a) && loc->get_property (X_("end"), b) && start.string_to (a) && end.string_to (b)
			    && start.is_superclock () && end.is_superclock () && end.val () > start.val ()) {
				seconds = (double) (end.val () - start.val ()) / ticks_per_second;
			}
			break;
		}
	}

	if (XMLNode const* routes = root->child (X_("Routes"))) {
		tracks = 0;
		for (XMLNode const* route : routes->children ()) {
			XMLNode const* pi = route->child (X_("PresentationInfo"));
			std::string    flags;
			if (pi && pi->get_property (X_("flags"), flags) && (flags.find (X_("AudioTrack")) != std::string::npos || flags.find (X_("MidiTrack")) != std::string::npos)) {
				++tracks;
			}
		}
	}
}

std::string
format_length (double seconds)
{
	const int t = (int) rint (std::min (seconds, 999.0 * 3600));
	if (t >= 3600) {
		return string_compose ("%1:%2:%3", t / 3600, PBD::to_string ((t / 60) % 60).insert (0, (t / 60) % 60 < 10 ? "0" : ""), PBD::to_string (t % 60).insert (0, t % 60 < 10 ? "0" : ""));
	}
	return string_compose ("%1:%2", t / 60, PBD::to_string (t % 60).insert (0, t % 60 < 10 ? "0" : ""));
}

} // anonymous namespace

SessionDialog::SessionDialog (DialogTab initial_tab, const std::string& session_name, const std::string& session_path, const std::string& template_name, bool cancel_not_quit)
	: ArdourDialog (initial_tab == New ? _("Session Setup") : _("Recent Sessions"), true, true)
	, _initial_tab (initial_tab)
	, new_name_was_edited (false)
	, new_folder_chooser (FILE_CHOOSER_ACTION_SELECT_FOLDER)
	, demo_folder_chooser (FILE_CHOOSER_ACTION_SELECT_FOLDER)
	, demo_name_was_edited (false)
	, download_active (false)
	, _chosen_rate (48000)
	, _chosen_format (FormatFloat)
	, _ignore_browser_selection_until (0)
{
	action_group = ActionGroup::create (X_("SessionDialog"));

	/* No string translation because these don't show anywhere */
	new_session_action = Action::create (X_("New"));
	action_group->add (new_session_action, sigc::mem_fun (*this, &SessionDialog::new_button_choice_action));
	recent_session_action = Action::create (X_("Recent"));
	action_group->add (recent_session_action, sigc::mem_fun (*this, &SessionDialog::recent_button_choice_action));
	existing_session_action = Action::create (X_("Open"));
	action_group->add (existing_session_action, sigc::mem_fun (*this, &SessionDialog::existing_button_choice_action));
	demo_session_action = Action::create (X_("DemoSessions"));
	action_group->add (demo_session_action, sigc::mem_fun (*this, &SessionDialog::demo_button_choice_action));

	set_position (WIN_POS_CENTER);
	set_default_size (rintf (1040 * UIConfiguration::instance ().get_ui_scale ()), rintf (740 * UIConfiguration::instance ().get_ui_scale ()));
	get_vbox()->set_spacing (6);
	get_vbox()->pack_start (_open_table, true, true);

	string image_path;
	Searchpath rc (ARDOUR::ardour_data_search_path());
	rc.add_subdirectory_to_paths ("resources");

	/* The two things the start window offers, besides the recent list */
	new_button.set_text (string_compose ("%1    <span alpha=\"55%%\" size=\"smaller\">%2</span>", _("New session"), _("Ctrl+N")), true);
	new_button.set_name ("tab button");
	new_button.set_alignment (0.0, 0.5);
	new_button.set_corner_radius (6);
	new_button.set_can_focus (true);
	new_button.set_related_action (new_session_action);

	existing_button.set_text (string_compose ("%1    <span alpha=\"55%%\" size=\"smaller\">%2</span>", _("Browse..."), _("Ctrl+O")), true);
	existing_button.set_name ("tab button");
	existing_button.set_alignment (0.0, 0.5);
	existing_button.set_corner_radius (6);
	existing_button.set_can_focus (true);
	existing_button.set_related_action (existing_session_action);

	/* kept for the code that still refers to them, but not shown */
	recent_button.set_related_action (recent_session_action);
	demo_button.set_related_action (demo_session_action);
	prefs_button.signal_button_press_event().connect (sigc::mem_fun (*this, &SessionDialog::prefs_button_pressed), false);

	int top = 0;
	int row = 0;

#if !defined (LIVETRAX) && !defined (VBM)
	/* Possible update message */
	if (ARDOUR_UI::instance()->announce_string() != "") {
		Gtk::HBox* info_hbox = manage (new HBox);
		Gtk::VBox* info_vbox = manage (new VBox);

		info_hbox->set_border_width (12);
		info_hbox->set_spacing (6);
		info_vbox->set_spacing (6);

		info_scroller_count = 0;
		info_scroller_connection = Glib::signal_timeout().connect (mem_fun(*this, &SessionDialog::info_scroller_update), 50);

		ArdourButton *updates_button = manage (new ArdourButton (_("Check the website for more...")));

		updates_button->signal_clicked.connect (mem_fun(*this, &SessionDialog::updates_button_clicked));
		set_tooltip (*updates_button, _("Click to open the program website in your web browser"));

		info_vbox->pack_start (info_scroller_label, false, false);
		info_vbox->pack_start (*updates_button, false, false);
		info_hbox->pack_start (*info_vbox, true, false);

		info_hbox->show_all ();

		/* no update message for trax, show license here */
		_open_table.attach (*info_hbox, 0, 3, row, row + 1, FILL, FILL, 0, 6); ++row; ++top;
	}
#endif

	build_left_panel ();

	_open_table.attach (_left_panel,       0,1, top, row + 1, FILL,        FILL|EXPAND, 0, 0);
	Label *vspacer = manage (new Label());
	vspacer->set_size_request(16, -1);
	_open_table.attach (*vspacer,          1,2, top, row + 1, FILL,        FILL|EXPAND, 0, 0);
	_open_table.attach (_tabs,             2,3, top, row + 1, FILL|EXPAND, FILL|EXPAND, 0, 0);

	_tabs.set_show_tabs(false);
	_tabs.set_show_border(false);

	_tabs.append_page(session_new_vbox);
	_tabs.append_page(recent_vbox);
	_tabs.append_page(existing_session_chooser);
	_tabs.append_page(demo_vbox);

	session_new_vbox.show_all();
	recent_vbox.show_all();
	existing_session_chooser.show_all();
	demo_vbox.show_all ();

	_tabs.show_all();

	cancel_button = add_button ((cancel_not_quit ? Stock::CANCEL : Stock::QUIT), RESPONSE_CANCEL);
	cancel_button->signal_button_press_event().connect (sigc::mem_fun (*this, &SessionDialog::cancel_button_pressed), false);

	open_button = add_button (Stock::OPEN, RESPONSE_ACCEPT);
	open_button->signal_button_press_event().connect (sigc::mem_fun (*this, &SessionDialog::open_button_pressed), false);

	_disable_plugins.set_label (_("Safe Mode: Disable all Plugins"));
	_disable_plugins.set_can_focus ();
	_disable_plugins.set_relief (Gtk::RELIEF_NORMAL);
	_disable_plugins.set_mode (true);
	_disable_plugins.set_active (ARDOUR::Session::get_disable_all_loaded_plugins());
	_disable_plugins.set_border_width(0);
#ifndef LIVETRAX
	_disable_plugins.signal_clicked().connect (sigc::mem_fun (*this, &SessionDialog::disable_plugins_clicked));

	cancel_button->get_parent ()->remove (*cancel_button);
	open_button->get_parent ()->remove (*open_button);
	ButtonBox* bbox = manage (new HButtonBox (BUTTONBOX_DEFAULT_STYLE, 5));
	bbox->add (*cancel_button);
	bbox->add (*open_button);

	HBox* abx = manage (new HBox (false, 5));
	abx->pack_end (*bbox, false, false);
	abx->pack_start (_disable_plugins, true, true);

	get_action_area ()->add (*abx);
#endif
	get_action_area ()->set_no_show_all (true);
	get_action_area ()->hide ();

	signal_key_press_event ().connect (sigc::mem_fun (*this, &SessionDialog::start_key_press), false);

	if (!template_name.empty()) {
		load_template_override = template_name;
	}

	/* fill data models and show/hide accordingly */

	setup_new_session_page ();
	setup_existing_box ();
	populate_session_templates ();
	setup_untitled_session ();
	setup_recent_sessions ();

	if (!Config->get_demo_session_index_url ().empty ()) {
		setup_demo_sessions ();
	}

	recent_vbox.pack_start (recent_scroller, true, true);

	get_vbox()->show_all ();
	get_action_area ()->hide ();

	if (recent_session_model) {
		int cnt = redisplay_recent_sessions ();
		fill_recent_list ();
		if (cnt > 0) {
			recent_scroller.show();
			recent_label.show ();
			recent_scroller.set_size_request (-1, 300);
		} else {
			recent_scroller.hide();
			recent_label.hide ();
		}
	}

	_tabs.signal_switch_page().connect (sigc::mem_fun (*this, &SessionDialog::tab_page_switched));
	disallow_idle ();

	/* the old recent-sessions list and the demo page are not reachable
	 * any more; hide them so they don't take up room */
	recent_vbox.hide ();
	demo_vbox.hide ();

	set_title (PROGRAM_NAME);
	ARDOUR_UI_UTILS::enlarge_fonts (*get_vbox (), 1.3);
}

SessionDialog::~SessionDialog()
{
}

void
SessionDialog::on_show ()
{
	ArdourDialog::on_show ();

	_tabs.set_current_page(4); // force change
	/* The start window always opens on the New session form (the
	 * recent sessions are on the left, one click away), unless the
	 * caller asked for the file browser.
	 */
	if (_initial_tab == Open) {
		_tabs.set_current_page(2);
	} else {
		_tabs.set_current_page(0);
	}
}

void
SessionDialog::tab_page_switched(GtkNotebookPage*, guint page_number)
{
	/* clang-format off */
	new_button.set_active_state      (page_number==0 ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	recent_button.set_active_state   (page_number==1 ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	existing_button.set_active_state (page_number==2 ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	demo_button.set_active_state     (page_number==3 ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	prefs_button.set_active_state    (page_number==4 ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	/* clang-format on */

	//check the status of each tab and sensitize the 'open' button appropriately
	open_button->set_sensitive(false);
	switch (page_number) {
		case 0:
			new_name_changed();
			new_name_entry.select_region (0, -1);
			new_name_entry.grab_focus ();
			_disable_plugins.hide ();
			break;
		case 1:
			recent_session_row_selected();
			_disable_plugins.show ();
			break;
		case 2:
			browser_folder_changed ();
			existing_file_selected();
			_disable_plugins.show ();
			break;
		case 3:
			_disable_plugins.hide ();
			query_demo_sessions ();
			break;
	}
}

uint32_t
SessionDialog::meta_master_bus_profile (std::string script_path)
{
	if (!Glib::file_test (script_path, Glib::FILE_TEST_EXISTS | Glib::FILE_TEST_IS_REGULAR)) {
		return UINT32_MAX;
	}

	LuaState lua (true, true);
	lua_State* L = lua.getState();

	lua.do_command (
			"ardourluainfo = {}"
			"function ardour (entry)"
			"  ardourluainfo['type'] = assert(entry['type'])"
			"  ardourluainfo['master_bus'] = entry['master_bus'] or 2"
			" end"
			);

	int err = -1;

	try {
		err = lua.do_file (script_path);
	} catch (luabridge::LuaException const& e) {
#ifndef NDEBUG
		cerr << "LuaException:" << e.what () << endl;
#endif
		PBD::warning << "LuaException: " << e.what () << endmsg;
		err = -1;
	}  catch (...) {
		err = -1;
	}

	if (err) {
		return UINT32_MAX;
	}

	luabridge::LuaRef nfo = luabridge::getGlobal (L, "ardourluainfo");
	if (nfo.type() != LUA_TTABLE) {
		return UINT32_MAX;
	}

	if (nfo["master_bus"].type() != LUA_TNUMBER || nfo["type"].type() != LUA_TSTRING) {
		return UINT32_MAX;
	}

	LuaScriptInfo::ScriptType type = LuaScriptInfo::str2type (nfo["type"].cast<std::string>());
	if (type != LuaScriptInfo::SessionInit) {
		return UINT32_MAX;
	}

	return nfo["master_bus"].cast<uint32_t>();
}

uint32_t
SessionDialog::master_channel_count ()
{
	if (use_session_template ()) {
		std::string tn = session_template_name();
		if (tn.substr (0, 11) == "urn:ardour:") {
			uint32_t mc = meta_master_bus_profile (tn.substr (11));
			if (mc != UINT32_MAX) {
				return mc;
			}
		}
	}
	return 2;
}

bool
SessionDialog::use_session_template () const
{
	/* a template is for a new session only, not for one that is being opened */
	if (!_chosen_recent.empty () || const_cast<Gtk::Notebook&> (_tabs).get_current_page () != 0) {
		return false;
	}

	if (template_chooser.get_selection()->count_selected_rows() > 0) {
		return true;
	}

	return false;
}

std::string
SessionDialog::session_template_name ()
{
	if (use_session_template ()) {

		TreeIter const iter = template_chooser.get_selection()->get_selected();

		if (iter) {
			string s = (*iter)[session_template_columns.path];
			return s;
		}
	}

	return string();
}

void
SessionDialog::clear_name ()
{
	_chosen_recent.clear ();
	recent_session_display.get_selection()->unselect_all();
	new_name_entry.set_text (string());
}

std::string
SessionDialog::session_name (bool& should_be_new)
{
	if (!_chosen_recent.empty ()) {
		should_be_new = false;
		if (Glib::file_test (_chosen_recent, Glib::FILE_TEST_IS_REGULAR)) {
			return PBD::basename_nosuffix (_chosen_recent);
		}
		return Glib::path_get_basename (_chosen_recent);
	}

	switch (_tabs.get_current_page()) {
	case 0: {
		should_be_new = true;
		string val = new_name_entry.get_text ();
		strip_whitespace_edges (val);
		return val;
	} break;
	case 1: {
		/* Try recent session selection */
		TreeIter iter = recent_session_display.get_selection()->get_selected();
		if (iter) {
			should_be_new = false;
			string s = (*iter)[recent_session_columns.fullpath];
			if (Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
				return PBD::basename_nosuffix (s);
			}
			return (*iter)[recent_session_columns.visible_name];
		}
	} break;
	case 2: {
		/* existing session chosen from file chooser */
		should_be_new = false;
		return existing_session_chooser.get_filename ();
	} break;
	case 3: {
		string val = demo_name_entry.get_text ();
		strip_whitespace_edges (val);
		return val;
	} break;
	}

	return "";
}

std::string
SessionDialog::session_folder ()
{
	if (!_chosen_recent.empty ()) {
		if (Glib::file_test (_chosen_recent, Glib::FILE_TEST_IS_REGULAR)) {
			return Glib::path_get_dirname (_chosen_recent);
		}
		return _chosen_recent;
	}

	switch (_tabs.get_current_page()) {
		case 0:
			{
				std::string val = new_name_entry.get_text();
				strip_whitespace_edges (val);
				std::string legal_session_folder_name = legalize_for_path (val);
				return Glib::build_filename (new_folder_chooser.get_filename (), legal_session_folder_name);
			}
		case 1:
			{
				/* Try recent session selection */
				TreeIter iter = recent_session_display.get_selection()->get_selected();
				if (iter) {
					string s = (*iter)[recent_session_columns.fullpath];
					if (Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
						return Glib::path_get_dirname (s);
					}
					return s;
				}
			}
			break;
		case 2:
			/* existing session chosen from file chooser */
			return existing_session_chooser.get_current_folder ();
		case 3:
			{
				std::string val = demo_name_entry.get_text();
				strip_whitespace_edges (val);
				std::string legal_session_folder_name = legalize_for_path (val);
				return Glib::build_filename (demo_folder_chooser.get_filename (), legal_session_folder_name);
			}
		default:
			break;
	}
	assert (0);
	return "";
}

Temporal::TimeDomain
SessionDialog::session_domain () const
{
	return timebase_chooser.get_active_row_number() == 1 ? Temporal::BeatTime : Temporal::AudioTime;
}

void
SessionDialog::setup_recent_sessions ()
{
	recent_session_model = TreeStore::create (recent_session_columns);
	recent_session_model->signal_sort_column_changed().connect (sigc::mem_fun (*this, &SessionDialog::recent_session_sort_changed));

	recent_session_display.set_model (recent_session_model);
	recent_session_display.append_column (_("Session Name"), recent_session_columns.visible_name);
	recent_session_display.append_column (_("Sample Rate"), recent_session_columns.sample_rate);
#ifdef MIXBUS
	recent_session_display.append_column (_("Modified With"), recent_session_columns.modified_with);
#else
	recent_session_display.append_column (_("File Resolution"), recent_session_columns.disk_format);
#endif
	recent_session_display.append_column (_("Last Modified"), recent_session_columns.time_formatted);
	recent_session_display.set_headers_visible (true);
	recent_session_display.get_selection()->set_mode (SELECTION_SINGLE);

	recent_session_display.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &SessionDialog::recent_session_row_selected));

	recent_scroller.add (recent_session_display);
	recent_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	recent_scroller.set_shadow_type	(Gtk::SHADOW_IN);

	recent_session_display.show();
	recent_session_display.signal_row_activated().connect (sigc::mem_fun (*this, &SessionDialog::recent_row_activated));
	recent_session_display.signal_button_press_event().connect (sigc::mem_fun (*this, &SessionDialog::recent_button_press), false);
}

void
SessionDialog::setup_existing_box ()
{
	/* Browse button */

	existing_session_chooser.set_size_request (400, 300);
	existing_session_chooser.set_current_folder(poor_mans_glob (Config->get_default_session_parent_dir()));

	FileFilter session_filter;
	session_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::statefile_suffix));
	session_filter.set_name (string_compose (_("%1 sessions"), PROGRAM_NAME));
	existing_session_chooser.add_filter (session_filter);

	FileFilter archive_filter;
	archive_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::session_archive_suffix));
	archive_filter.set_name (_("Session Archives"));
	existing_session_chooser.add_filter (archive_filter);

	FileFilter aaf_filter;
	aaf_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::advanced_authoring_format_suffix));
	aaf_filter.set_name (_("Advanced Authoring Format (AAF)"));
	existing_session_chooser.add_filter (aaf_filter);

	FileFilter all_filter;
	all_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::statefile_suffix));
	all_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::session_archive_suffix));
	all_filter.add_pattern (string_compose(X_("*%1"), ARDOUR::advanced_authoring_format_suffix));
	all_filter.set_name (_("All supported files"));
	existing_session_chooser.add_filter (all_filter);

	existing_session_chooser.set_filter (session_filter);

	Gtkmm2ext::add_volume_shortcuts (existing_session_chooser);

	existing_session_chooser.signal_selection_changed().connect (mem_fun (*this, &SessionDialog::existing_file_selected));
	existing_session_chooser.signal_current_folder_changed().connect (mem_fun (*this, &SessionDialog::browser_folder_changed));
	existing_session_chooser.signal_file_activated().connect (sigc::mem_fun (*this, &SessionDialog::existing_file_activated));
}

void
SessionDialog::existing_file_selected ()
{
	if (_tabs.get_current_page()!=2) {
		//gtk filechooser is threaded; don't allow it to mess with open_button sensitivity when it's not actually visible
		return;
	}

	open_button->set_sensitive (false);

	std::string const& s = existing_session_chooser.get_filename ();
	if (!Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
		return;
	}

	std::string suffix = s.substr (s.find_last_of ('.'));

	if (PBD::downcase (suffix).find (advanced_authoring_format_suffix) == 0) {
		// OK
	}  else if (suffix.find (session_archive_suffix) == 0) {
		// OK
	} else {
		float        sr;
		SampleFormat sf;
		string       pv;

		switch (Session::get_info_from_path (s, sr, sf, pv)) {
			case 1:
				/* OK */
				break;
			case 0:
				//TODO: display the rate somewhere? check that our engine can open this rate?
				/* OK */
				break;
			case -1:
				error << string_compose (_("Session file %1 does not exist"), s) << endmsg;
				return;
				break;
			case -3:
				error << string_compose (_("Session %1 is from a newer version of %2"), s, PROGRAM_NAME) << endmsg;
				return;
				break;
			default:
				error << string_compose (_("Cannot get existing session information from %1"), s) << endmsg;
				return;
				break;
		}
	}

	open_button->set_sensitive(true);  //still potentially openable; checks for session archives, .ptf, and .aaf will have to occur later

	/* one click opens it */
	if (g_get_monotonic_time () > _ignore_browser_selection_until) {
		response (RESPONSE_ACCEPT);
	}
}

void
SessionDialog::session_selected ()
{
}

void
SessionDialog::new_button_choice_action ()
{
	_tabs.set_current_page(0);
}

void
SessionDialog::recent_button_choice_action ()
{
	_tabs.set_current_page(1);
}

void
SessionDialog::existing_button_choice_action ()
{
	_tabs.set_current_page(2);
}

void
SessionDialog::demo_button_choice_action ()
{
	_tabs.set_current_page(3);
}

bool
SessionDialog::prefs_button_pressed (GdkEventButton*)
{
	_tabs.set_current_page(4);

	open_button->set_sensitive(false);  //do not allow to open a session from this page

	return true;
}
bool
SessionDialog::cancel_button_pressed (GdkEventButton* ev)
{
	if (download_active) {
		cancel_download = true;
		return true;
	}

	response (RESPONSE_CANCEL);
	return true;
}

bool
SessionDialog::open_button_pressed (GdkEventButton* ev)
{
	if (ev && Gtkmm2ext::Keyboard::modifier_state_equals (ev->state, Gtkmm2ext::Keyboard::PrimaryModifier)) {
		_disable_plugins.set_active();
	}
	if (_tabs.get_current_page() == 3) {
		if (!deploy_demo_session ()) {
			return true;
		}
	}
	response (RESPONSE_ACCEPT);
	return true;
}

void
SessionDialog::setup_untitled_session ()
{
	new_name_entry.set_text (string_compose (_("Untitled-%1"), Glib::DateTime::create_now_local().format ("%F-%H-%M-%S")));
	new_name_was_edited = false;
}

void
SessionDialog::delete_selected_template ()
{
	Gtk::TreeModel::const_iterator current_selection = template_chooser.get_selection()->get_selected ();

	if (!current_selection) {
		return;
	}

	if (!current_selection->get_value (session_template_columns.removable)) {
		ArdourMessageDialog msg (("This type of template cannot be deleted"));
		msg.run ();
		return;  //cannot delete built-in scripts
	}

	PBD::remove_directory (current_selection->get_value (session_template_columns.path));

	template_model->erase (current_selection);

	populate_session_templates ();
}
bool
SessionDialog::template_button_press (GdkEventButton* ev)
{
	if (Gtkmm2ext::Keyboard::is_context_menu_event (ev)) {
		show_template_context_menu (ev->button, ev->time);
		/* return false to select item under the mouse */
	}
	return false;
}

void
SessionDialog::show_template_context_menu (int button, int time)
{
	using namespace Gtk::Menu_Helpers;
	Gtk::Menu* menu = ARDOUR_UI::instance()->shared_popup_menu ();
	MenuList&  items = menu->items ();
	items.push_back (MenuElem (_("Delete the selected Template"), hide_return (sigc::mem_fun (*this, &SessionDialog::delete_selected_template))));
	menu->popup (button, time);
}

void
SessionDialog::populate_session_templates ()
{
	vector<TemplateInfo> templates;

	find_session_templates (templates, true);

	template_model->clear ();

	/* Get Lua Scripts dedicated to session-setup */
	LuaScriptList scripts (LuaScripting::instance ().scripts (LuaScriptInfo::SessionInit));

	/* Add Lua Action Scripts which can also be used for session-setup */
	LuaScriptList& as (LuaScripting::instance ().scripts (LuaScriptInfo::EditorAction));
	for (LuaScriptList::const_iterator s = as.begin(); s != as.end(); ++s) {
		if ((*s)->subtype & LuaScriptInfo::SessionSetup) {
			scripts.push_back (*s);
		}
	}

	std::sort (scripts.begin(), scripts.end(), LuaScripting::Sorter());

	for (LuaScriptList::const_iterator s = scripts.begin(); s != scripts.end(); ++s) {
		TreeModel::Row row = *(template_model->append ());
		row[session_template_columns.name] = (*s)->name;
		row[session_template_columns.path] = "urn:ardour:" + (*s)->path;
		row[session_template_columns.description] = (*s)->description;
		row[session_template_columns.modified_with_short] = string_compose ("{%1}", _("Factory Template"));
		row[session_template_columns.modified_with_long] = string_compose ("{%1}", _("Factory Template"));
		row[session_template_columns.removable] = false;
	}

	//Add any "template sessions" found in the user's preferences folder
	for (vector<TemplateInfo>::iterator x = templates.begin(); x != templates.end(); ++x) {
		TreeModel::Row row;

		row = *(template_model->append ());

		row[session_template_columns.name] = (*x).name;
		row[session_template_columns.path] = (*x).path;
		row[session_template_columns.description] = (*x).description;
		row[session_template_columns.modified_with_long] = (*x).modified_with;
		row[session_template_columns.modified_with_short] = (*x).modified_with.substr(0, (*x).modified_with.find(" "));
		row[session_template_columns.removable] = true;
	}

	//Add an explicit 'Empty Template' item
	TreeModel::Row row = *template_model->prepend ();
	row[session_template_columns.name] = (_("Empty Template"));
	row[session_template_columns.path] = string();
	row[session_template_columns.description] = _("An empty session with factory default settings.\n\nSelect this option if you are importing files to mix.");
	row[session_template_columns.modified_with_short] = ("");
	row[session_template_columns.modified_with_long] = ("");
	row[session_template_columns.removable] = false;

	//auto-select the first item in the list
	Gtk::TreeModel::Row first = template_model->children()[0];
	if(first) {
		template_chooser.get_selection()->select(first);
	}

	rebuild_template_buttons ();
}

/* A small bold caption above each field of the new-session form */
static Gtk::Label*
form_caption (std::string const& text)
{
	Gtk::Label* l = manage (new Gtk::Label);
	l->set_markup (string_compose ("<span weight=\"bold\" alpha=\"80%%\">%1</span>", Gtkmm2ext::markup_escape_text (text)));
	l->set_alignment (0.5, 0.5);
	return l;
}

static Gtk::VBox*
form_field (std::string const& caption, Gtk::Widget& w)
{
	Gtk::VBox* v = manage (new Gtk::VBox);
	v->set_spacing (4);
	v->pack_start (*form_caption (caption), false, false);
	v->pack_start (w, false, false);
	return v;
}

void
SessionDialog::setup_new_session_page ()
{
	/* One column, top to bottom: name, location, start from, sample
	 * rate, bit depth, then "Create session". Everything else (time
	 * domain, template descriptions, ...) uses its default and can be
	 * changed later in Session > Properties.
	 */
	const float scale = UIConfiguration::instance ().get_ui_scale ();

	new_name_entry.signal_key_press_event().connect (sigc::mem_fun (*this, &SessionDialog::name_edited), false);
	new_name_entry.signal_changed().connect (sigc::mem_fun (*this, &SessionDialog::new_name_changed));
	new_name_entry.signal_activate().connect (sigc::mem_fun (*this, &SessionDialog::new_name_activated));

	if (ARDOUR_UI::instance()->the_session ()) {
		// point the new session file chooser at the parent directory of the current session
		string session_parent_dir = Glib::path_get_dirname(ARDOUR_UI::instance()->the_session()->path());
		new_folder_chooser.set_current_folder (session_parent_dir);
		string default_session_folder = poor_mans_glob (Config->get_default_session_parent_dir());

		try {
			/* add_shortcut_folder throws an exception if the folder being added already has a shortcut */
			new_folder_chooser.add_shortcut_folder (default_session_folder);
		}
		catch (Glib::Error & e) {
			std::cerr << "new_folder_chooser.add_shortcut_folder (" << default_session_folder << ") threw Glib::Error " << e.what() << std::endl;
		}
	} else {
		new_folder_chooser.set_current_folder (poor_mans_glob (Config->get_default_session_parent_dir()));
	}
	new_folder_chooser.show ();
	new_folder_chooser.set_title (_("Select folder for session"));
	Gtkmm2ext::add_volume_shortcuts (new_folder_chooser);

	/* Not shown: the time domain uses the preference default */
	timebase_chooser.append (_("Audio Time"));
	timebase_chooser.append (_("Beat Time"));
	timebase_chooser.set_active (Config->get_preferred_time_domain() == Temporal::BeatTime ? 1 : 0);

	/* The template list is kept (hidden) as the record of which
	 * template is chosen; the visible choice is a column of buttons.
	 */
	template_desc.set_editable (false);
	template_model = TreeStore::create (session_template_columns);
	template_chooser.set_model (template_model);
	template_chooser.append_column (_("Template"), session_template_columns.name);
	template_chooser.set_headers_visible (false);
	template_chooser.get_selection()->set_mode (SELECTION_SINGLE);
	template_chooser.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &SessionDialog::template_row_selected));
	_template_radio_box.set_spacing (6);

	/* Sample rate: preselect what the audio interface is running at */
	_rates = { 44100, 48000, 88200, 96000 };
	if (AudioEngine::instance ()->running ()) {
		const uint32_t sr = AudioEngine::instance ()->sample_rate ();
		if (std::find (_rates.begin (), _rates.end (), sr) == _rates.end ()) {
			_rates.push_back (sr);
		}
		_chosen_rate = sr;
	}
	Gtk::HBox* rate_box = manage (new Gtk::HBox (true, 4));
	for (uint32_t r : _rates) {
		ArdourButton* b = manage (new ArdourButton (rate_as_string (r)));
		b->set_name ("tab button");
		b->set_corner_radius (6);
		b->signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &SessionDialog::rate_clicked), r));
		if (AudioEngine::instance ()->running ()) {
			/* a session started from a running engine uses its rate */
			b->set_sensitive (false);
			set_tooltip (*b, _("The audio interface is already running at this rate. Change it in Window > Audio/MIDI Setup."));
		}
		rate_box->pack_start (*b, true, true);
		_rate_buttons.push_back (b);
	}
	rate_clicked (_chosen_rate);

	/* Bit depth */
	_formats = { FormatInt16, FormatInt24, FormatFloat };
	Gtk::HBox* depth_box = manage (new Gtk::HBox (true, 4));
	for (SampleFormat f : _formats) {
		ArdourButton* b = manage (new ArdourButton (f == FormatInt16 ? _("16-bit") : f == FormatInt24 ? _("24-bit") : _("32-bit float")));
		b->set_name ("tab button");
		b->set_corner_radius (6);
		b->signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &SessionDialog::depth_clicked), f));
		depth_box->pack_start (*b, true, true);
		_depth_buttons.push_back (b);
	}
	depth_clicked (_chosen_format);

	/* Create */
	_create_button.set_text (_("Create session"));
	_create_button.set_name ("generic button");
	_create_button.set_corner_radius (7);
	_create_button.set_fixed_colors (UIConfiguration::instance ().color ("gtk_bg_selected"), UIConfiguration::instance ().color ("gtk_bg_selected"));
	_create_button.set_size_request (-1, rintf (44 * scale));
	_create_button.signal_clicked.connect (sigc::mem_fun (*this, &SessionDialog::new_name_activated));

	Gtk::Label* heading = manage (new Gtk::Label);
	heading->set_markup (string_compose ("<span size=\"x-large\" weight=\"bold\">%1</span>", _("New session")));
	heading->set_alignment (0.5, 0.5);
	new_name_entry.set_alignment (0.5);

	Gtk::VBox* column = manage (new Gtk::VBox);
	column->set_spacing (12);
	column->set_size_request (rintf (480 * scale), -1);
	column->pack_start (*heading, false, false);
	column->pack_start (*form_field (_("Name"), new_name_entry), false, false);
	column->pack_start (*form_field (_("Location"), new_folder_chooser), false, false);
	if (load_template_override.empty ()) {
		column->pack_start (*form_field (_("Start from"), _template_radio_box), false, false);
	}
	column->pack_start (*form_field (_("Sample rate"), *rate_box), false, false);
	column->pack_start (*form_field (_("Bit depth"), *depth_box), false, false);
	column->pack_start (_create_button, false, false, 6);

	Gtk::Alignment* center = manage (new Gtk::Alignment (0.5, 0.0, 0.0, 0.0));
	center->set_padding (rintf (8 * scale), rintf (4 * scale), 0, 0);
	center->add (*column);

	session_new_vbox.pack_start (*center, true, true);
	session_new_vbox.show_all ();
}

uint32_t
SessionDialog::session_sample_rate () const
{
	if (!_chosen_recent.empty ()) {
		return 0;
	}
	return const_cast<Gtk::Notebook&> (_tabs).get_current_page () == 0 ? _chosen_rate : 0;
}

void
SessionDialog::rate_clicked (uint32_t r)
{
	_chosen_rate = r;
	for (size_t i = 0; i < _rate_buttons.size (); ++i) {
		_rate_buttons[i]->set_active_state (_rates[i] == r ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	}
}

void
SessionDialog::depth_clicked (SampleFormat f)
{
	_chosen_format = f;
	for (size_t i = 0; i < _depth_buttons.size (); ++i) {
		_depth_buttons[i]->set_active_state (_formats[i] == f ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	}
}

/* One button per template ("Empty Template", the factory templates and
 * any the user saved). Clicking one selects it in the hidden list. */
void
SessionDialog::rebuild_template_buttons ()
{
	for (ArdourButton* b : _template_buttons) {
		_template_radio_box.remove (*b);
		delete b;
	}
	_template_buttons.clear ();

	const float scale = UIConfiguration::instance ().get_ui_scale ();

	for (Gtk::TreeModel::iterator i = template_model->children ().begin (); i != template_model->children ().end (); ++i) {
		std::string name = (*i)[session_template_columns.name];
		std::string path = (*i)[session_template_columns.path];
		std::string desc = (*i)[session_template_columns.description];

		if (path.empty ()) {
			name = _("Empty session");
		}

		ArdourButton* b = new ArdourButton (ArdourButton::led_default_elements);
		b->set_text (name);
		b->set_name ("tab button");
		b->set_alignment (0.5, 0.5);
		b->set_corner_radius (6);
		b->set_led_left (true);
		b->set_size_request (-1, rintf (34 * scale));
		if (!desc.empty ()) {
			set_tooltip (*b, desc);
		}
		b->signal_clicked.connect (sigc::bind (sigc::mem_fun (*this, &SessionDialog::template_button_clicked), path));
		_template_radio_box.pack_start (*b, false, false);
		b->show ();
		_template_buttons.push_back (b);
	}
	sync_template_buttons ();
}

void
SessionDialog::template_button_clicked (std::string path)
{
	for (Gtk::TreeModel::iterator i = template_model->children ().begin (); i != template_model->children ().end (); ++i) {
		if (std::string ((*i)[session_template_columns.path]) == path) {
			template_chooser.get_selection ()->select (i);
			break;
		}
	}
	sync_template_buttons ();
}

void
SessionDialog::sync_template_buttons ()
{
	std::string selected = session_template_name ();
	bool        any      = use_session_template ();
	size_t      n        = 0;

	for (Gtk::TreeModel::iterator i = template_model->children ().begin (); i != template_model->children ().end () && n < _template_buttons.size (); ++i, ++n) {
		std::string path = (*i)[session_template_columns.path];
		const bool  on   = any && path == selected;
		_template_buttons[n]->set_active_state (on ? Gtkmm2ext::ExplicitActive : Gtkmm2ext::Off);
	}
}

void
SessionDialog::setup_demo_sessions ()
{
	demo_model = ListStore::create (demo_columns);
	demo_display.set_model (demo_model);

	demo_display.append_column (_("Session Name"), demo_columns.name);
	demo_display.append_column (_("Download Size"), demo_columns.download_size);

	Gtk::CellRendererProgress* progress_renderer = manage (new Gtk::CellRendererProgress());
	progress_renderer->property_width() = std::max<int>(100, rintf(100. * UIConfiguration::instance().get_ui_scale()));
	Gtk::TreeViewColumn* tvc = manage (new Gtk::TreeViewColumn ("", *progress_renderer));
	tvc->add_attribute (*progress_renderer, "value", demo_columns.progress);
	tvc->add_attribute (*progress_renderer, "visible", demo_columns.downloading);
	demo_display.append_column (*tvc);

	demo_display.get_selection()->set_mode (Gtk::SELECTION_SINGLE);
	demo_display.get_selection()->signal_changed().connect (sigc::mem_fun (*this, &SessionDialog::demo_session_selected));
	demo_display.signal_row_activated().connect (sigc::mem_fun (*this, &SessionDialog::demo_row_activated));
	demo_display.signal_button_press_event().connect (sigc::mem_fun (*this, &SessionDialog::demo_button_press), false);

	demo_folder_chooser.set_title (_("Select folder for session"));
	Gtkmm2ext::add_volume_shortcuts (demo_folder_chooser);
	demo_folder_chooser.set_current_folder (poor_mans_glob (Config->get_default_session_parent_dir()));

	demo_author.set_alignment (Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER);
	demo_license.set_alignment (Gtk::ALIGN_LEFT, Gtk::ALIGN_CENTER);
	demo_license.signal_activate_link().connect ([](std::string const& url) { return PBD::open_uri (url); }, false);

	demo_description.set_wrap_mode (Gtk::WRAP_WORD);
	demo_description.set_editable (false);
	demo_description.set_name (X_("TextOnBackground"));
	demo_description.set_border_width (6);
	demo_description.set_size_request (-1, std::max<int>(80, rintf(80. * UIConfiguration::instance().get_ui_scale())));

	demo_name_entry.signal_changed().connect (sigc::mem_fun (*this, &SessionDialog::demo_name_changed));
	demo_name_entry.signal_activate().connect (sigc::mem_fun (*this, &SessionDialog::demo_name_activated));

	demo_name_entry.signal_key_press_event().connect (sigc::mem_fun (*this, &SessionDialog::name_edited), false);

	demo_scroller.add (demo_display);
	demo_scroller.set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	demo_scroller.set_shadow_type	(Gtk::SHADOW_IN);

	Label* author_label  = manage (new Label (_("Author:"), ALIGN_LEFT));
	Label* license_label = manage (new Label (_("License:"), ALIGN_LEFT));
	Label* folder_label  = manage (new Label (_("Extract demo session to:")));;
	Label* name_label    = manage (new Label (_("Session name:")));

	Table* license_tabel = manage (new Table);
	license_tabel->set_spacings (8);
	license_tabel->attach (*author_label,  0, 1, 0, 1, FILL,          SHRINK);
	license_tabel->attach (demo_author,    1, 2, 0, 1, EXPAND | FILL, SHRINK);
	license_tabel->attach (*license_label, 0, 1, 1, 2, FILL,          SHRINK);
	license_tabel->attach (demo_license,   1, 2, 1, 2, EXPAND | FILL, SHRINK);

	HBox* folder_hbox = manage (new HBox);
	folder_hbox->set_spacing (8);
	folder_hbox->pack_start (*folder_label, false, false);
	folder_hbox->pack_start (demo_folder_chooser, true, true);

	HBox* name_hbox = manage (new HBox);
	name_hbox->set_spacing (8);
	name_hbox->pack_start (*name_label, false, true);
	name_hbox->pack_start (demo_name_entry, true, true);

	Gtk::ScrolledWindow* desc_scroller = manage (new Gtk::ScrolledWindow());
	desc_scroller->set_policy (Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
	desc_scroller->add (demo_description);

	demo_desc_frame.set_name (X_("TextHighlightFrame"));
	demo_desc_frame.add (*desc_scroller);

	demo_vbox.pack_start (demo_scroller, true, true);
	demo_vbox.pack_start (demo_desc_frame, false, false);
	demo_vbox.pack_start (*license_tabel, false, false);
	demo_vbox.pack_start (*folder_hbox, false, false);
	demo_vbox.pack_start (*name_hbox, false, false);
	demo_vbox.set_spacing (8);
}

bool
SessionDialog::name_edited (GdkEventKey* ev)
{
	switch (ev->keyval) {
	case GDK_KP_Enter:
	case GDK_3270_Enter:
	case GDK_Return:
		break;
	default:
		switch (_tabs.get_current_page()) {
			case 0:
				new_name_was_edited = true;
				break;
			case 3:
				demo_name_was_edited = true;
				break;
		}
	}

	return false;
}


static bool is_invalid_session_char (char c)
{
	/* see also Session::session_name_is_legal */
	return iscntrl (c) || c == '/' || c == '\\' || c == ':' || c == ';';
}

void
SessionDialog::new_name_changed ()
{
	std::string new_name = new_name_entry.get_text();

	std::string const& illegal = Session::session_name_is_legal (new_name);
	if (!illegal.empty()) {
		ArdourMessageDialog msg (string_compose (_("To ensure compatibility with various systems\nsession names may not contain a '%1' character"), illegal));
		msg.run ();
		new_name.erase (remove_if (new_name.begin(), new_name.end(), is_invalid_session_char), new_name.end());
		new_name_entry.set_text (new_name);
	}

	if (!new_name_entry.get_text().empty()) {
		session_selected ();
		open_button->set_sensitive (true);
		_create_button.set_sensitive (true);
	} else {
		open_button->set_sensitive (false);
		_create_button.set_sensitive (false);
	}
}

void
SessionDialog::new_name_activated ()
{
	if (new_name_entry.get_text ().empty ()) {
		return;
	}
	response (RESPONSE_ACCEPT);
}

int
SessionDialog::redisplay_recent_sessions ()
{
	std::vector<std::string> session_directories;
	RecentSessionsSorter cmp;

	recent_session_display.set_model (Glib::RefPtr<TreeModel>(0));
	recent_session_model->clear ();

	ARDOUR::RecentSessions rs;
	ARDOUR::read_recent_sessions (rs);

	if (rs.empty()) {
		recent_session_display.set_model (recent_session_model);
		return 0;
	}

	// sort them alphabetically
	sort (rs.begin(), rs.end(), cmp);

	for (ARDOUR::RecentSessions::iterator i = rs.begin(); i != rs.end(); ++i) {
		session_directories.push_back ((*i).second);
	}

	int session_snapshot_count = 0;

	for (vector<std::string>::const_iterator i = session_directories.begin(); i != session_directories.end(); ++i) {

		string dirname = *i;

		/* remove any trailing / */

		if (dirname.empty()) {
			continue;
		}

		if (dirname[dirname.length()-1] == '/') {
			dirname = dirname.substr (0, dirname.length()-1);
		}

		/* check whether session still exists */
		if (!Glib::file_test(dirname.c_str(), Glib::FILE_TEST_EXISTS)) {
			/* session doesn't exist */
			continue;
		}

		/* now get available states for this session */
		vector<string> state_file_names = Session::possible_states (dirname);

		if (state_file_names.empty()) {
			/* no state file? */
			continue;
		}

		float sr;
		SampleFormat sf;
		std::string program_version;

		std::string state_file_basename;

		if (state_file_names.size() > 1) {
			state_file_basename = Session::get_snapshot_from_instant (dirname);
			std::string s = Glib::build_filename (dirname, state_file_basename + statefile_suffix);
			if (!Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
				state_file_basename = "";
			}
		}

		if (state_file_basename.empty()) {
			state_file_basename = state_file_names.front();
		}

		std::string s = Glib::build_filename (dirname, state_file_basename + statefile_suffix);

		int err = Session::get_info_from_path (s, sr, sf, program_version);
		if (err < 0) {
			// XML cannot be parsed, or unsuppored version
			continue;
		}

#ifdef LIVETRAX
		/* check 'modified-with' */
		if (program_version.empty()) {
			continue;
		}
		if (program_version.rfind (PROGRAM_NAME, 0) != 0) {
			continue;
		}
#endif

		GStatBuf gsb;
		g_stat (s.c_str(), &gsb);

		Gtk::TreeModel::Row row = *(recent_session_model->append());
		row[recent_session_columns.fullpath] = s;
		row[recent_session_columns.time_modified] = gsb.st_mtime;


		if (err == 0) {
			row[recent_session_columns.sample_rate] = rate_as_string (sr);
			switch (sf) {
			case FormatFloat:
				row[recent_session_columns.disk_format] = _("32-bit float");
				break;
			case FormatInt24:
				row[recent_session_columns.disk_format] = _("24-bit");
				break;
			case FormatInt16:
				row[recent_session_columns.disk_format] = _("16-bit");
				break;
			}
		} else {
			row[recent_session_columns.sample_rate] = "??";
			row[recent_session_columns.disk_format] = "--";
		}

		if (program_version.empty()) {
			row[recent_session_columns.tip] = Gtkmm2ext::markup_escape_text (dirname);
		} else {
			row[recent_session_columns.tip] = Gtkmm2ext::markup_escape_text (dirname + "\n" + string_compose (_("Last modified with: %1"), program_version));
			row[recent_session_columns.modified_with] = program_version;
		}

		++session_snapshot_count;

		if (state_file_names.size() > 1) {
			// multiple session files in the session directory - show the directory name.
			// if there's not a session file with the same name as the session directory,
			// opening the parent item will fail, but expanding it will show the session
			// files that actually exist, and the right one can then be opened.
			row[recent_session_columns.visible_name] = Glib::path_get_basename (dirname);
			int64_t most_recent = 0;

			// add the children
			int kidcount = 0;
			for (std::vector<std::string>::iterator i2 = state_file_names.begin(); i2 != state_file_names.end(); ++i2) {

				s = Glib::build_filename (dirname, *i2 + statefile_suffix);
				Gtk::TreeModel::Row child_row = *(recent_session_model->append (row.children()));

				child_row[recent_session_columns.visible_name] = *i2;
				child_row[recent_session_columns.fullpath] = s;
				child_row[recent_session_columns.tip] = Gtkmm2ext::markup_escape_text (dirname);
				g_stat (s.c_str(), &gsb);
				child_row[recent_session_columns.time_modified] = gsb.st_mtime;

				Glib::DateTime gdt(Glib::DateTime::create_now_local (gsb.st_mtime));
				child_row[recent_session_columns.time_formatted] = gdt.format ("%F %H:%M");

				if (gsb.st_mtime > most_recent) {
					most_recent = gsb.st_mtime;
				}

				if (++kidcount < 5) {
					// parse "modified with" for the first 5 snapshots
					if (Session::get_info_from_path (s, sr, sf, program_version) == 0) {
#if 0
						child_row[recent_session_columns.sample_rate] = rate_as_string (sr);
						switch (sf) {
						case FormatFloat:
							child_row[recent_session_columns.disk_format] = _("32-bit float");
							break;
						case FormatInt24:
							child_row[recent_session_columns.disk_format] = _("24-bit");
							break;
						case FormatInt16:
							child_row[recent_session_columns.disk_format] = _("16-bit");
							break;
						}
#else
						child_row[recent_session_columns.sample_rate] = "";
						child_row[recent_session_columns.disk_format] = "";
#endif
					} else {
						child_row[recent_session_columns.sample_rate] = "??";
						child_row[recent_session_columns.disk_format] = "--";
					}
					if (!program_version.empty()) {
						child_row[recent_session_columns.tip] = Gtkmm2ext::markup_escape_text (string_compose (_("Last modified with: %1"), program_version));
					}
				} else {
					child_row[recent_session_columns.sample_rate] = "";
					child_row[recent_session_columns.disk_format] = "";
				}

				++session_snapshot_count;
			}

			assert (most_recent >= row[recent_session_columns.time_modified]);
			row[recent_session_columns.time_modified] = most_recent;

		} else {
			// only a single session file in the directory - show its actual name.
			row[recent_session_columns.visible_name] = state_file_basename;
		}

		Glib::DateTime gdt(Glib::DateTime::create_now_local (row[recent_session_columns.time_modified]));
		row[recent_session_columns.time_formatted] = gdt.format ("%F %H:%M");
	}

	if (UIConfiguration::instance().get_use_tooltips()) {
		recent_session_display.set_tooltip_column(1); // recent_session_columns.tip
	}
	recent_session_display.set_model (recent_session_model);

	// custom sort
	Gtk::TreeView::Column* pColumn;
	if ((pColumn = recent_session_display.get_column (0))) { // name
		pColumn->set_sort_column (recent_session_columns.visible_name);
	}
	if ((pColumn = recent_session_display.get_column (3))) { // date
		pColumn->set_sort_column (recent_session_columns.time_modified); // unixtime
	}

	int32_t sort = UIConfiguration::instance().get_recent_session_sort();
	if (abs(sort) != 1 + recent_session_columns.visible_name.index () &&
	    abs(sort) != 1 + recent_session_columns.time_modified.index ()) {
		sort = 1 + recent_session_columns.visible_name.index();
	}
	recent_session_model->set_sort_column (abs (sort) -1, sort < 0 ? Gtk::SORT_DESCENDING : Gtk::SORT_ASCENDING);

	//auto-select the first item in the list
	Gtk::TreeModel::Row first = recent_session_model->children()[0];
	if(first) {
		recent_session_display.get_selection()->select(first);
	}

	return session_snapshot_count;
}

void
SessionDialog::recent_session_sort_changed ()
{
	int column;
	SortType order;
	if (recent_session_model->get_sort_column_id (column, order)) {
		int32_t sort = (column + 1) * (order == Gtk::SORT_DESCENDING ? -1 : 1);
		if (sort != UIConfiguration::instance().get_recent_session_sort()) {
			UIConfiguration::instance().set_recent_session_sort(sort);
		}
	}
}

void
SessionDialog::recent_session_row_selected ()
{
	if (recent_session_display.get_selection()->count_selected_rows() > 0) {
		open_button->set_sensitive (true);
		session_selected ();
	} else {
		open_button->set_sensitive (false);
	}
}

void
SessionDialog::template_row_selected ()
{
	if (template_chooser.get_selection()->count_selected_rows() > 0) {
		TreeIter iter = template_chooser.get_selection()->get_selected();

		if (iter) {
			string s = (*iter)[session_template_columns.description];
			template_desc.get_buffer()->set_text (s);
		}
	}
	sync_template_buttons ();
}

void
SessionDialog::recent_row_activated (const Gtk::TreePath&, Gtk::TreeViewColumn*)
{
	response (RESPONSE_ACCEPT);
}

bool
SessionDialog::recent_button_press (GdkEventButton* ev)
{
	if ((ev->type == GDK_BUTTON_PRESS) && (ev->button == 3)) {

		TreeModel::Path path;
		TreeViewColumn* column;
		int cellx, celly;
		if (recent_session_display.get_path_at_pos ((int)ev->x, (int)ev->y, path, column, cellx, celly)) {
			Glib::RefPtr<Gtk::TreeView::Selection> selection = recent_session_display.get_selection();
			if (selection) {
				selection->unselect_all();
				selection->select(path);
			}
		}

		if (recent_session_display.get_selection()->count_selected_rows() > 0) {
			recent_context_mennu (ev);
		}
	}
	return false;
}

void
SessionDialog::recent_context_mennu (GdkEventButton *ev)
{
	using namespace Gtk::Menu_Helpers;

	TreeIter iter = recent_session_display.get_selection()->get_selected();
	assert (iter);
	string s = (*iter)[recent_session_columns.fullpath];
	if (Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
		s = Glib::path_get_dirname (s);
	}
	if (!Glib::file_test (s, Glib::FILE_TEST_IS_DIR)) {
		return;
	}

	Gtk::TreeModel::Path tpath = recent_session_model->get_path(iter);
	const bool is_child = tpath.up () && tpath.up ();

	Gtk::Menu* m = ARDOUR_UI::instance()->shared_popup_menu ();
	MenuList& items = m->items ();
	items.push_back (MenuElem (s, sigc::bind (sigc::hide_return (sigc::ptr_fun (&PBD::open_folder)), s)));
	if (!is_child) {
		items.push_back (SeparatorElem());
		items.push_back (MenuElem (_("Remove session from recent list"), sigc::mem_fun (*this, &SessionDialog::recent_remove_selected)));
	}
	m->popup (ev->button, ev->time);
}

void
SessionDialog::recent_remove_selected ()
{
	TreeIter iter = recent_session_display.get_selection()->get_selected();
	assert (iter);
	string s = (*iter)[recent_session_columns.fullpath];
	if (Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
		s = Glib::path_get_dirname (s);
	}
	ARDOUR::remove_recent_sessions (s);
	redisplay_recent_sessions ();
}

void
SessionDialog::disable_plugins_clicked ()
{
	ARDOUR::Session::set_disable_all_loaded_plugins (_disable_plugins.get_active());
}

void
SessionDialog::existing_file_activated ()
{
	std::string s = existing_session_chooser.get_filename ();
	if (Glib::file_test (s, Glib::FILE_TEST_IS_REGULAR)) {
		response (RESPONSE_ACCEPT);
	}
}

void
SessionDialog::updates_button_clicked ()
{
	//now open a browser window so user can see more
	PBD::open_uri (Config->get_updates_url());
}

bool
SessionDialog::info_scroller_update()
{
	info_scroller_count++;

	char buf[512];
	snprintf (buf, std::min(info_scroller_count,sizeof(buf)-1), "%s", ARDOUR_UI::instance()->announce_string().c_str());
	buf[info_scroller_count] = 0;
	info_scroller_label.set_text (buf);
	info_scroller_label.show();

	if (info_scroller_count > ARDOUR_UI::instance()->announce_string().length()) {
		info_scroller_connection.disconnect();
	}

	return true;
}

bool
SessionDialog::on_delete_event (GdkEventAny* ev)
{
	if (download_active) {
		cancel_download = true;
		/* wait for demo_dl_timer_callback () */
		Glib::usleep (150000);
	}
	response (RESPONSE_CANCEL);
	return ArdourDialog::on_delete_event (ev);
}

void
SessionDialog::add_demo_session (ARDOUR::RemoteResourceInfo const& ld)
{
	Gtk::TreeModel::iterator i       = demo_model->append();
	(*i)[demo_columns.name]          = ld.name();
	(*i)[demo_columns.size]          = ld.size();
	(*i)[demo_columns.download_size] = ld.size();
	(*i)[demo_columns.url]           = ld.url();
	(*i)[demo_columns.file]          = ld.toplevel_dir();
	(*i)[demo_columns.downloading]   = false;
	(*i)[demo_columns.description]   = Glib::Markup::escape_text (ld.description());
	(*i)[demo_columns.license]       = ld.license();
	(*i)[demo_columns.author]        = ld.author();

	string demo_filename = ld.toplevel_dir();
	string demo_session_archive = Glib::build_filename (demo_session_dir (), demo_filename);
	if (Glib::file_test (demo_session_archive, Glib::FILE_TEST_EXISTS)) {
		(*i)[demo_columns.download_size] = "Downloaded";
	}
}

void
SessionDialog::query_demo_sessions ()
{
	if (Config->get_demo_session_index_url ().empty ()) {
		return;
	}
	if (!demo_model->children().empty()) {
		/* fill only once */
		return;
	}

	ARDOUR::LibraryFetcher lf (Config->get_demo_session_index_url (), demo_session_dir(), "Sessions");

	if (lf.get_descriptions () || 0 == lf.n_descriptions()) {
		ArdourMessageDialog msg (_("Failed to retrieve demo sessions list. Please check your internet connection."), false, MESSAGE_ERROR);
		msg.run ();
		return;
	}
	if (lf.n_descriptions()) {
		demo_model->clear ();
	}
	lf.foreach_description (std::bind (&SessionDialog::add_demo_session, this, _1));
}

void
SessionDialog::demo_session_selected ()
{
	if (demo_display.get_selection()->count_selected_rows() == 0) {
		demo_description.get_buffer()->set_text (string());
		demo_license.set_text (string());
		demo_author.set_text (string ());
		open_button->set_sensitive(false);  //do not allow to open a session from this page
		return;
	}

  Gtk::TreeModel::iterator row = demo_display.get_selection()->get_selected();
  string name = (*row)[demo_columns.name];
	string desc = (*row)[demo_columns.description];
	string lcns = (*row)[demo_columns.license];
	string athr = (*row)[demo_columns.author];
	demo_name_entry.set_text (name);
	demo_author.set_text (athr);
	demo_license.set_markup (lcns);
	demo_description.get_buffer()->set_text (desc);

	if (lcns.find ("<a href=") != lcns.npos) {
		demo_license.property_track_visited_links() = false;
	}

}

void
SessionDialog::demo_row_activated (const Gtk::TreePath& path, Gtk::TreeViewColumn* col)
{
	if (demo_display.get_selection()->count_selected_rows() != 1 || download_active) {
		return;
	}

	if (demo_name_entry.get_text().empty () || !demo_name_was_edited) {
		Gtk::TreeModel::iterator row = demo_display.get_selection()->get_selected();
		string name = (*row)[demo_columns.name];
		demo_name_entry.set_text (name);
	}

	open_button_pressed (NULL); // RESPONSE_ACCEPT
}

bool
SessionDialog::demo_button_press (GdkEventButton* ev)
{
	if (download_active) {
		return true;
	}
	if (demo_display.get_selection()->count_selected_rows() != 1) {
		return false;
	}
	if (!Gtkmm2ext::Keyboard::is_context_menu_event (ev)) {
		return false;
	}

	Gtk::TreeModel::iterator row = demo_display.get_selection()->get_selected();
	string file_size = (*row)[demo_columns.size];
	string demo_filename = (*row)[demo_columns.file];
	string demo_session_archive = Glib::build_filename (demo_session_dir (), demo_filename);

	bool exists = Glib::file_test (demo_session_archive, Glib::FILE_TEST_EXISTS);

	using namespace Gtk::Menu_Helpers;
	Gtk::Menu* m     = ARDOUR_UI_UTILS::shared_popup_menu ();
	MenuList&  items = m->items ();

	items.push_back (MenuElem (_("Delete Downloaded Session Archive"), [&, row, demo_session_archive, file_size](){ ::g_unlink (demo_session_archive.c_str()); (*row)[demo_columns.download_size] = file_size;}));
	items.back ().set_sensitive (exists);
	m->popup (ev->button, ev->time);

	return true;
}

void
SessionDialog::demo_name_changed ()
{
	std::string name = demo_name_entry.get_text();

	std::string const& illegal = Session::session_name_is_legal (name);
	if (!illegal.empty()) {
		ArdourMessageDialog msg (string_compose (_("To ensure compatibility with various systems\nsession names may not contain a '%1' character"), illegal));
		msg.run ();
		name.erase (remove_if (name.begin(), name.end(), is_invalid_session_char), name.end());
		new_name_entry.set_text (name);
	}

	if (demo_display.get_selection()->count_selected_rows() == 0 || demo_name_entry.get_text().empty()) {
		open_button->set_sensitive (false);
	} else {
		open_button->set_sensitive (true);
	}
}

void
SessionDialog::demo_name_activated ()
{
	if (open_button->get_sensitive () && !download_active) {
		open_button_pressed (NULL); // RESPONSE_ACCEPT
	}
}

void
SessionDialog::set_downloading (bool en)
{
	if (en) {
		download_active = true;
		open_button->set_sensitive (false);
		new_button.set_sensitive (false);
		recent_button.set_sensitive (false);
		existing_button.set_sensitive (false);
		prefs_button.set_sensitive (false);
	} else {
		download_active = false;
		open_button->set_sensitive (true);
		new_button.set_sensitive (true);
		recent_button.set_sensitive (true);
		existing_button.set_sensitive (true);
		prefs_button.set_sensitive (true);
	}
}

bool
SessionDialog::deploy_demo_session ()
{
	if (demo_display.get_selection()->count_selected_rows() != 1) {
		assert (0);
		return false;
	}

	/* test if session exists */
	if (Glib::file_test (session_folder (), Glib::FILE_TEST_EXISTS)) {
		if (ARDOUR_UI::instance()->ask_about_loading_existing_session (session_folder ())) {
			response (RESPONSE_ACCEPT);
		}
		return false;
	}

	/* Download session, unless already downloaded */
	Gtk::TreeModel::iterator row = demo_display.get_selection()->get_selected();
	string demo_filename = (*row)[demo_columns.file];
	string demo_session_archive = Glib::build_filename (demo_session_dir (), demo_filename);

	if (!Glib::file_test (demo_session_archive, Glib::FILE_TEST_EXISTS)) {
		std::string url = (*row)[demo_columns.url];
		PBD::Downloader* downloader;

		try {
			downloader = new PBD::Downloader (url, demo_session_dir ());
		} catch (...) {
			ArdourMessageDialog msg (_("Failed to initiate downloading."), false, MESSAGE_ERROR);
			msg.run ();
			return false;
		}

		(*row)[demo_columns.downloader] = downloader;
		(*row)[demo_columns.downloading] = true;

		cancel_download = false;
		downloader->start ();
		set_downloading (true);

		Glib::signal_timeout().connect (sigc::bind (sigc::mem_fun (*this, &SessionDialog::demo_dl_timer_callback), downloader, TreePath(row)), 40);

		/* wait for download to complete */
		while ((downloader = (*row)[demo_columns.downloader]) != NULL) {
			ARDOUR::GUIIdle ();
			// TODO only sleep if GUI did not process any events
			Glib::usleep (10000);
		}

		int status = (*row)[demo_columns.status];
		if (status < 0) {
			set_downloading (false);
			if (!cancel_download) {
				ArdourMessageDialog msg (_("Downloading demo session failed."), false, MESSAGE_ERROR);
				msg.run ();
			}
			return false;
		}
	}

	set_downloading (false);

	std::string error_msg = _("Failed to extract demo session archive.");
	switch (ARDOUR::inflate_demo_session (PBD::basename_nosuffix (demo_filename), session_folder ())) {
		case 0:
			return true; // OK -> close dialog
			break;
		case -1:
			error_msg = _("Demo Session not found.");
			break;
		case -2:
			error_msg = _("Invalid Session Name.");
			break;
		case -3:
			error_msg = _("Target folder for Demo Session exists.");
			break;
		case 1:
			/* invalid file, extract failed */
			::g_unlink (demo_session_archive.c_str());
			break;
		default:
			break;
	}

	ArdourMessageDialog msg (error_msg, false, MESSAGE_ERROR);
	msg.run ();

	return false; // Error
}

bool
SessionDialog::demo_dl_timer_callback (Downloader* dl, Gtk::TreePath treepath)
{
	Gtk::TreeModel::iterator row = demo_model->get_iter (treepath);

	if (dl->status() == 0) {
		(*row)[demo_columns.progress] = (int) round (dl->progress() * 100.0);
		if (cancel_download) {
			dl->cancel ();
		}
		return true; /* call again */
	}

	(*row)[demo_columns.progress] = 0.;
	(*row)[demo_columns.downloader] = 0;

	(*row)[demo_columns.status] = dl->status();
	(*row)[demo_columns.downloading] = false;
	delete dl;
	return false;
}

/* ---- simplified start window ---------------------------------------- */

void
SessionDialog::build_left_panel ()
{
	const float scale = UIConfiguration::instance ().get_ui_scale ();

	Gtk::Label* brand = manage (new Gtk::Label);
	brand->set_markup (string_compose ("<span size=\"x-large\" weight=\"bold\">%1</span>\n<span alpha=\"60%%\">%2</span>",
	                                   PROGRAM_NAME, string_compose (_("Version %1"), VERSIONSTRING)));
	brand->set_alignment (0.0, 0.5);

	new_button.set_size_request (-1, rintf (46 * scale));
	existing_button.set_size_request (-1, rintf (46 * scale));

	_recent_heading.set_markup (string_compose ("<span size=\"small\" weight=\"bold\" alpha=\"65%%\">%1</span>", _("RECENT")));
	_recent_heading.set_alignment (0.0, 1.0);
	_recent_heading.set_no_show_all (true);

	_recent_list_box.set_spacing (4);

	_left_panel.set_spacing (8);
	_left_panel.set_border_width (rintf (8 * scale));
	_left_panel.set_size_request (rintf (330 * scale), -1);
	_left_panel.pack_start (*brand, false, false, 0);
	_left_panel.pack_start (new_button, false, false);
	_left_panel.pack_start (existing_button, false, false);
	_left_panel.pack_start (_recent_heading, false, false, rintf (22 * scale));
	_left_panel.pack_start (_recent_list_box, true, true);
}

void
SessionDialog::fill_recent_list ()
{
	std::vector<Gtk::Widget*> old = _recent_list_box.get_children ();
	for (Gtk::Widget* w : old) {
		_recent_list_box.remove (*w);
		delete w;
	}

	struct Entry {
		int64_t     when;
		std::string path;
		std::string name;
		std::string tip;
	};
	std::vector<Entry> entries;

	for (Gtk::TreeModel::iterator i = recent_session_model->children ().begin (); i != recent_session_model->children ().end (); ++i) {
		Entry e;
		e.when = (*i)[recent_session_columns.time_modified];
		e.path = (*i)[recent_session_columns.fullpath];
		e.name = (*i)[recent_session_columns.visible_name];
		e.tip  = (*i)[recent_session_columns.tip];
		entries.push_back (e);
	}

	std::sort (entries.begin (), entries.end (), [] (Entry const& a, Entry const& b) { return a.when > b.when; });
	if (entries.size () > 5) {
		entries.resize (5);
	}

	size_t n = 0;
	for (Entry const& e : entries) {
		double seconds;
		int    tracks;
		read_session_summary (e.path, seconds, tracks);

		std::string details = friendly_date (e.when);
		if (seconds > 0) {
			details += " · " + format_length (seconds);
		}
		if (tracks >= 0) {
			details += " · " + string_compose (P_("%1 track", "%1 tracks", tracks), tracks);
		}

		/* tooltips arrive markup-escaped from the model; show them as plain text */
		std::string tip = e.tip;
		replace_all (tip, "&amp;", "&");
		replace_all (tip, "&lt;", "<");
		replace_all (tip, "&gt;", ">");

		RecentSessionRow* row = new RecentSessionRow (e.name, details, tip, recent_palette[n++ % (sizeof (recent_palette) / sizeof (recent_palette[0]))]);
		row->Activated.connect (sigc::bind<0> (sigc::mem_fun (*this, &SessionDialog::open_recent_path), e.path));
		_recent_list_box.pack_start (*row, false, false);
	}

	if (entries.empty ()) {
		_recent_heading.hide ();
	} else {
		_recent_heading.show ();
	}
}

/** Open one of the recent sessions straight away */
void
SessionDialog::open_recent_path (std::string const& fullpath, bool safe_mode)
{
	_chosen_recent = fullpath;
	if (safe_mode) {
		_disable_plugins.set_active (true);
	}
	response (RESPONSE_ACCEPT);
}

bool
SessionDialog::start_key_press (GdkEventKey* ev)
{
	if (!Gtkmm2ext::Keyboard::modifier_state_equals (ev->state, Gtkmm2ext::Keyboard::PrimaryModifier)) {
		return false;
	}
	switch (ev->keyval) {
		case GDK_n:
		case GDK_N:
			_tabs.set_current_page (0);
			return true;
		case GDK_o:
		case GDK_O:
			_tabs.set_current_page (2);
			return true;
	}
	return false;
}

/* The file browser opens a session on a single click. Folder changes
 * (and switching to the browser) can make GTK report a selection by
 * itself, so selections within a moment of those are ignored. */
void
SessionDialog::browser_folder_changed ()
{
	_ignore_browser_selection_until = g_get_monotonic_time () + 400000;
}
