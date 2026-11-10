/*
 * Copyright (C) 2005-2007 Doug McLain <doug@nostar.net>
 * Copyright (C) 2005-2017 Tim Mayberry <mojofunk@gmail.com>
 * Copyright (C) 2005-2019 Paul Davis <paul@linuxaudiosystems.com>
 * Copyright (C) 2005 Karsten Wiese <fzuuzf@googlemail.com>
 * Copyright (C) 2005 Taybin Rutkin <taybin@taybin.com>
 * Copyright (C) 2006-2015 David Robillard <d@drobilla.net>
 * Copyright (C) 2007-2012 Carl Hetherington <carl@carlh.net>
 * Copyright (C) 2008-2010 Sakari Bergen <sakari.bergen@beatwaves.net>
 * Copyright (C) 2012-2019 Robin Gareus <robin@gareus.org>
 * Copyright (C) 2013-2015 Colin Fletcher <colin.m.fletcher@googlemail.com>
 * Copyright (C) 2013-2016 John Emmas <john@creativepost.co.uk>
 * Copyright (C) 2013-2016 Nick Mainsbridge <mainsbridge@gmail.com>
 * Copyright (C) 2014-2018 Ben Loftis <ben@harrisonconsoles.com>
 * Copyright (C) 2015 André Nusser <andre.nusser@googlemail.com>
 * Copyright (C) 2016-2018 Len Ovens <len@ovenwerks.net>
 * Copyright (C) 2017 Johannes Mueller <github@johannes-mueller.org>
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
#include "gtk2ardour-version.h"
#endif

#ifndef PLATFORM_WINDOWS
#include <sys/resource.h>
#include <sys/wait.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#endif

#include <cstring>
#include <vector>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <glib.h>
#include "pbd/gstdio_compat.h"

#include <ytkmm/stock.h>
#include <ytkmm/alignment.h>
#include <ytkmm/clipboard.h>
#include <ytkmm/frame.h>
#include <ytkmm/table.h>
#include <ytkmm/textview.h>

#include "pbd/basename.h"
#include "pbd/file_utils.h"
#include "pbd/search_path.h"

#include "ardour/audioengine.h"
#include "ardour/filename_extensions.h"
#include "ardour/filesystem_paths.h"
#include "ardour/profile.h"
#include "ardour/recent_sessions.h"

#include "gtkmm2ext/application.h"

#include "ambiguous_file_dialog.h"
#include "ardour_dialog.h"
#include "gtkmm2ext/utils.h"
#include "ardour_message.h"
#include "ardour_ui.h"
#include "debug.h"
#include "engine_dialog.h"
#include "keyboard.h"
#include "missing_file_dialog.h"
#include "nsm.h"
#include "opts.h"
#include "utils.h"
#include "pingback.h"
#include "plugin_scan_dialog.h"
#include "public_editor.h"
#include "splash.h"
#include "ui_config.h"

#include "pbd/i18n.h"

using namespace Temporal;
using namespace ARDOUR;
using namespace PBD;
using namespace Gtk;
using namespace Gtkmm2ext;
using namespace std;

static bool
_hide_splash (gpointer arg)
{
	((ARDOUR_UI*)arg)->hide_splash();
	return false;
}

bool
ARDOUR_UI::first_idle ()
{
	if (_session) {
		_session->reset_xrun_count ();
		_session->allow_auto_play (true);
	}

	if (editor) {
		editor->first_idle();
	}

	/* in 1 second, hide the splash screen */
	Glib::signal_timeout().connect (sigc::bind (sigc::ptr_fun (_hide_splash), this), 1000);

	Keyboard::set_can_save_keybindings (true);
	return false;
}

void
ARDOUR_UI::setup_profile ()
{
	if (gdk_screen_width() < 1200 || getenv ("ARDOUR_NARROW_SCREEN")) {
		Profile->set_small_screen ();
	}

	if (g_getenv ("MIXBUS")) {
		Profile->set_mixbus ();
	}
}

int
ARDOUR_UI::missing_file (Session*s, std::string str, DataType type)
{
	MissingFileDialog dialog (_main_window, s, str, type);

	dialog.show ();
	dialog.present ();

	int result = dialog.run ();
	dialog.hide ();

	switch (result) {
	case RESPONSE_OK:
		break;
	default:
		return 1; // quit entire session load
	}

	result = dialog.get_action ();

	return result;
}

int
ARDOUR_UI::ambiguous_file (std::string file, std::vector<std::string> hits)
{
	AmbiguousFileDialog dialog (file, hits);

	dialog.show ();
	dialog.present ();

	dialog.run ();

	return dialog.get_which ();
}

void
ARDOUR_UI::session_format_mismatch (std::string xml_path, std::string backup_path)
{
	const char* start_big = "<span size=\"x-large\" weight=\"bold\">";
	const char* end_big = "</span>";
	const char* start_mono = "<tt>";
	const char* end_mono = "</tt>";

	ArdourMessageDialog msg (string_compose (_("%4This is a session from an older version of %3%5\n\n"
	                                           "%3 has copied the old session file\n\n%6%1%7\n\nto\n\n%6%2%7\n\n"
	                                           "From now on, use the backup copy with older versions of %3"),
	                                         xml_path, backup_path, PROGRAM_NAME,
	                                         start_big, end_big,
	                                         start_mono, end_mono), true);

	msg.run ();
}


int
ARDOUR_UI::sr_mismatch_dialog (samplecnt_t desired, samplecnt_t actual)
{
	HBox* hbox = new HBox();
	Image* image = new Image (Stock::DIALOG_WARNING, ICON_SIZE_DIALOG);
	ArdourDialog dialog (_("Sample Rate Mismatch"), true);
	Label message (string_compose (_("\
This session was created with a sample rate of %1 Hz, but \
%2 is currently running at %3 Hz. If you load this session, \
audio will be resampled, which reduces quality.\n"), desired, PROGRAM_NAME, actual));
	message.set_line_wrap ();

	image->set_alignment(ALIGN_CENTER, ALIGN_START);
	hbox->pack_start (*image, PACK_EXPAND_WIDGET, 12);
	hbox->pack_end (message, PACK_EXPAND_PADDING, 12);
	dialog.get_vbox()->pack_start(*hbox, PACK_EXPAND_PADDING, 6);

	if (ARDOUR::AudioEngine::instance ()->setup_required ()) {
		dialog.add_button (_("Reconfigure Engine"), RESPONSE_YES);
	}
	dialog.add_button (_("Do not load session"), RESPONSE_REJECT);
	dialog.add_button (_("Load session anyway"), RESPONSE_ACCEPT);
	dialog.set_default_response (RESPONSE_ACCEPT);
	dialog.set_position (WIN_POS_CENTER);
	message.show();
	image->show();
	hbox->show();

	switch (dialog.run()) {
	case RESPONSE_YES:
		ARDOUR::AudioEngine::instance ()->stop ();
		(dynamic_cast<EngineControl*> (audio_midi_setup.get (true)))->run ();
		(dynamic_cast<EngineControl*> (audio_midi_setup.get (true)))->hide ();
		return AudioEngine::instance()->running () ? -1 : 1;
	case RESPONSE_ACCEPT:
		return 0;
	default:
		break;
	}

	return 1;
}

void
ARDOUR_UI::sr_mismatch_message (samplecnt_t desired, samplecnt_t actual)
{
	ArdourMessageDialog msg (string_compose (_("\
This session was created with a sample rate of %1 Hz, but \
%2 is currently running at %3 Hz.\n\
Audio is resampled for both playback and recording to match the sampling \
rate, which reduces quality. Reconfigure the Audio Engine in \
Menu > Window > Audio/Midi Setup"),
				desired, PROGRAM_NAME, actual),
			true,
			Gtk::MESSAGE_WARNING);
	msg.run ();
}


XMLNode*
ARDOUR_UI::preferences_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("Preferences"));
	} else {
		node = Config->instant_xml(X_("Preferences"));
	}

	if (!node) {
		node = new XMLNode (X_("Preferences"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::mixer_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("Mixer"));
	} else {
		node = Config->instant_xml(X_("Mixer"));
	}

	if (!node) {
		node = new XMLNode (X_("Mixer"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::main_window_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("Main"));
	} else {
		node = Config->instant_xml(X_("Main"));
	}

	if (!node) {
		if (getenv("ARDOUR_INSTANT_XML_PATH")) {
			node = Config->instant_xml(getenv("ARDOUR_INSTANT_XML_PATH"));
		}
	}

	if (!node) {
		node = new XMLNode (X_("Main"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::editor_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("Editor"));
	} else {
		node = Config->instant_xml(X_("Editor"));
	}

	if (!node) {
		if (getenv("ARDOUR_INSTANT_XML_PATH")) {
			node = Config->instant_xml(getenv("ARDOUR_INSTANT_XML_PATH"));
		}
	}

	if (!node) {
		node = new XMLNode (X_("Editor"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::recorder_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("Recorder"));
	} else {
		node = Config->instant_xml(X_("Recorder"));
	}

	if (!node) {
		node = new XMLNode (X_("Recorder"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::trigger_page_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("TriggerPage"));
	} else {
		node = Config->instant_xml(X_("TriggerPage"));
	}

	if (!node) {
		node = new XMLNode (X_("TriggerPage"));
	}

	return node;
}

XMLNode*
ARDOUR_UI::clock_mode_settings () const
{
	XMLNode* node = 0;

	if (_session) {
		node = _session->instant_xml(X_("ClockModes"));
	}
	if (!node) {
		node = Config->instant_xml(X_("ClockModes"));
	}
	if (!node) {
		node = new XMLNode (X_("ClockModes"));
	}
	return node;
}

XMLNode*
ARDOUR_UI::keyboard_settings () const
{
	XMLNode* node = 0;

	node = Config->extra_xml(X_("Keyboard"));

	if (!node) {
		node = new XMLNode (X_("Keyboard"));
	}

	return node;
}

void
ARDOUR_UI::hide_splash ()
{
	Splash::drop ();
}

void
ARDOUR_UI::check_announcements ()
{
#ifdef PHONE_HOME
	string _annc_filename;

#ifdef __APPLE__
	_annc_filename = PROGRAM_NAME "_announcements_osx_";
#elif defined PLATFORM_WINDOWS
	_annc_filename = PROGRAM_NAME "_announcements_windows_";
#else
	_annc_filename = PROGRAM_NAME "_announcements_linux_";
#endif
	_annc_filename.append (VERSIONSTRING);

	_announce_string = "";

	std::string path = Glib::build_filename (user_config_directory(), _annc_filename);
	FILE* fin = g_fopen (path.c_str(), "rb");
	if (fin) {
		while (!feof (fin)) {
			char tmp[1024];
			size_t len;
			if ((len = fread (tmp, sizeof(char), 1024, fin)) == 0 || ferror (fin)) {
				break;
			}
			_announce_string.append (tmp, len);
		}
		fclose (fin);
	}

	pingback (VERSIONSTRING, path);
#endif
}

int
ARDOUR_UI::nsm_init ()
{
	const char *nsm_url;

	if ((nsm_url = g_getenv ("NSM_URL")) == 0) {
		return 0;
	}

	nsm = new NSM_Client;

	if (nsm->init (nsm_url)) {
		delete nsm;
		nsm = 0;
		error << _("NSM: initialization failed") << endmsg;
		return -1;
	}

	/* the ardour executable may have different names:
	 *
	 * waf's obj.target for distro versions: eg ardour4, ardourvst4
	 * Ardour4, Mixbus3 for bundled versions + full path on OSX & windows
	 * argv[0] does not apply since we need the wrapper-script (not the binary itself)
	 *
	 * The wrapper startup script should set the environment variable 'ARDOUR_SELF'
	 */
	const char *process_name = g_getenv ("ARDOUR_SELF");
	nsm->announce (PROGRAM_NAME, ":dirty:switch:", process_name ? process_name : "ardour7");

	unsigned int i = 0;
	// wait for announce reply from nsm server
	for ( i = 0; i < 5000; ++i) {
		nsm->check ();

		Glib::usleep (i);
		if (nsm->is_active()) {
			break;
		}
	}
	if (i == 5000) {
		error << _("NSM server did not announce itself. Continuing without NSM.") << endmsg;
		delete nsm;
		nsm = 0;
		return 0;
	}

	/* wait for open command from nsm server */
	for (i = 0; i < 5000; ++i) {
		nsm->check ();
		Glib::usleep (1000);
		if (nsm->client_id ()) {
			break;
		}
	}

	if (i == 5000) {
		error << _("NSM: no client ID provided") << endmsg;
		delete nsm;
		nsm = 0;
		return -1;
	}

	if (_session && nsm) {
		_session->set_nsm_state( nsm->is_active() );
	} else {
		error << _("NSM: no session created") << endmsg;
		delete nsm;
		nsm = 0;
		return -1;
	}

	// nsm requires these actions disabled
	vector<string> action_names;
	action_names.push_back("SaveAs");
	action_names.push_back("Rename");
	action_names.push_back("New");
	action_names.push_back("Open");
	action_names.push_back("Recent");
	action_names.push_back("Close");

	for (vector<string>::const_iterator n = action_names.begin(); n != action_names.end(); ++n) {
		Glib::RefPtr<Action> act = ActionManager::get_action (X_("Main"), (*n).c_str());
		if (act) {
			act->set_sensitive (false);
		}
	}

	return 0;
}

void
ARDOUR_UI::sfsm_response (StartupFSM::Result r)
{
	DEBUG_TRACE (DEBUG::GuiStartup, string_compose (X_("startup FSM response %1\n"), r));

	switch (r) {
	case StartupFSM::ExitProgram: {
		/* failure mode */
		std::stringstream str;;
		dump_errors (str, 10);
		std::string msg (string_compose (_("<span font_size=\"large\" font_weight=\"bold\">Something went seriously wrong. %1 cannot continue.</span>\n\n"
		                                   "Here are a few hints at what might be wrong:\n\n%2"),
		                                 PROGRAM_NAME,
		                                 str.str()));
		ArdourMessageDialog d (msg, true);
		d.set_title (string_compose (_("%1: Unrecoverable Error"), PROGRAM_NAME));
		d.run();

		queue_finish ();
	}
		break;

	case StartupFSM::QuitProgram:
		/* user explicitly requested quit */
		queue_finish ();
		break;

	case StartupFSM::LoadedSession:
		startup_done ();
		break;

	case StartupFSM::LoadSession:

		if (load_session_from_startup_fsm () == 0) {
			startup_done ();
		} else {
			DEBUG_TRACE (DEBUG::GuiStartup, "FSM reset\n");
			startup_fsm->reset ();
		}

		break;
	}
}

int
ARDOUR_UI::starting ()
{
	Application* app = Application::instance();

	app->ShouldLoad.connect (sigc::mem_fun (*this, &ARDOUR_UI::load_from_application_api));

	if (ARDOUR_COMMAND_LINE::check_announcements
#ifndef MIXBUS
	    && UIConfiguration::instance().get_check_announcements ()
#endif
	   ) {
		check_announcements ();
	}

	app->ready ();

	/* we need to create this early because it may need to set the
	 *  audio backend end up.
	 */

	EngineControl* amd;
	Timing amd_init;
	info << _("Detecting Audio/MIDI Devices") << endmsg;
	BootMessage (_("Detecting Audio/MIDI Devices..."));

	amd_init.start();
	try {
		amd = dynamic_cast<EngineControl*> (audio_midi_setup.get (true));
	} catch (...) {
		std::cerr << "audio-midi engine setup failed."<< std::endl;
		return -1;
	}
	amd_init.update();
	info << string_compose (_("Audio/MIDI Devices detection complete. Elapsed time: %1 ms"), amd_init.elapsed() / 1000) << endmsg;
	BootMessage (_("Continuing Startup..."));

	if (nsm_init ()) {
		return -1;
	} else {

		if (nsm) {
			ARDOUR_UI::hide_splash ();
			return 0;
		}

		startup_fsm = new StartupFSM (*amd);
		startup_fsm->signal_response().connect (sigc::mem_fun (*this, &ARDOUR_UI::sfsm_response));


		if (!startup_fsm) {
			DEBUG_TRACE (DEBUG::GuiStartup, "Starting: SFSM was driven by flush-pending\n");
			return 0;
		}

		/* Note: entire startup process could happen in this one call
		 * if:
		 *
		 * 1) not a new user
		 * 2) session name provided on command line (and valid)
		 * 3) no audio/MIDI setup required
		 */

		if (!startup_fsm->complete()) {
			startup_fsm->start ();
		} else {
			DEBUG_TRACE (DEBUG::GuiStartup, "Starting: SFSM already completed (by load_from_application_api)\n");
		}

		if (startup_fsm->complete()) {
			delete startup_fsm;
			startup_fsm = 0;
		} else {
			DEBUG_TRACE (DEBUG::GuiStartup, "Starting: SFSM incomplete; not deleted.\n");
		}
	}

	return 0;
}

int
ARDOUR_UI::load_session_from_startup_fsm ()
{
	const string     session_path          = startup_fsm->session_path;
	const string     session_name          = startup_fsm->session_name;
	const string     session_template      = startup_fsm->session_template;
	const bool       session_is_new        = startup_fsm->session_is_new;
	const bool       session_was_not_named = (!startup_fsm->session_name_edited && ARDOUR_COMMAND_LINE::session_name.empty());
	const TimeDomain session_domain        = startup_fsm->session_domain;
	const BusProfile bus_profile           = startup_fsm->bus_profile;

	if (session_is_new) {

		if (build_session (session_path, session_name, session_template, bus_profile, true, session_was_not_named, session_domain)) {
			return -1;
		}
		if (_session) {
			/* bit depth chosen in the new-session form */
			_session->config.set_native_file_data_format (startup_fsm->session_sample_format);
		}
		return 0;
	}

	int ret = load_session (session_path, session_name, session_template);

	return ret;
}

void
ARDOUR_UI::startup_done ()
{
	/* ShouldQuit is a desktop environment mechanism that tells the
	   application it should exit for reasons external to the application
	   itself.

	   During startup, startupFSM handles ShouldQuit. But it is done now,
	   and we have to take over responsibility.
	*/
	Application::instance()->ShouldQuit.connect (sigc::mem_fun (*this, &ARDOUR_UI::queue_finish));
	/* Same story applies for ShouldLoad ... startupFSM will handle it
	   normally, but if it doesn't we (ARDOUR_UI) need to take responsibility
	   for it.
	*/
	Application::instance()->ShouldLoad.connect (sigc::mem_fun (*this, &ARDOUR_UI::load_from_application_api));

	use_config ();

	WM::Manager::instance().show_visible ();

	/* We have to do this here since goto_editor_window() ends up calling show_all() on the
	 * editor window, and we may want stuff to be hidden.
	 */
	_status_bar_visibility.update ();

	BootMessage (string_compose (_("%1 is ready for use"), PROGRAM_NAME));

	startup_fsm->set_complete ();
}

void
ARDOUR_UI::use_config ()
{
	XMLNode* node = Config->extra_xml (X_("TransportControllables"));
	if (node) {
		set_transport_controllable_state (*node);
	}
}

#if !defined(__APPLE__) && !defined(PLATFORM_WINDOWS) && !defined(__FreeBSD__) && !defined(__NetBSD__)

/* true if the user is listed as a member of `group` in the group database */
static bool
user_listed_in_group (const char* group)
{
	struct group*  gr = getgrnam (group);
	struct passwd* pw = getpwuid (getuid ());
	if (!gr || !pw) {
		return false;
	}
	if (pw->pw_gid == gr->gr_gid) {
		return true;
	}
	for (char** m = gr->gr_mem; m && *m; ++m) {
		if (strcmp (*m, pw->pw_name) == 0) {
			return true;
		}
	}
	return false;
}

/* true if this running process already has `group` (i.e. the user
 * has logged in again since being added to it)
 */
static bool
process_has_group (const char* group)
{
	struct group* gr = getgrnam (group);
	if (!gr) {
		return false;
	}
	if (getegid () == gr->gr_gid) {
		return true;
	}
	int n = getgroups (0, NULL);
	if (n <= 0) {
		return false;
	}
	std::vector<gid_t> gids (n);
	n = getgroups (n, &gids[0]);
	for (int i = 0; i < n; ++i) {
		if (gids[i] == gr->gr_gid) {
			return true;
		}
	}
	return false;
}

/* A command or file text the user copies into a terminal: shown in a
 * monospace box they can also select from, with a "Copy" button.
 */
static Gtk::Widget*
copyable_text (std::string const& text)
{
	Gtk::TextView* tv = manage (new Gtk::TextView);
	tv->get_buffer ()->set_text (text);
	tv->set_editable (false);
	tv->set_cursor_visible (false);
	tv->set_left_margin (8);
	tv->set_right_margin (8);
	tv->set_pixels_above_lines (4);
	tv->set_pixels_below_lines (4);
	tv->set_name (X_("TextOnBackground"));

	Gtk::Frame* frame = manage (new Gtk::Frame);
	frame->set_shadow_type (Gtk::SHADOW_IN);
	frame->add (*tv);

	Gtk::Button* copy = manage (new Gtk::Button (_("Copy")));
	copy->signal_clicked ().connect ([text, copy] () {
		Gtk::Clipboard::get ()->set_text (text);
		Gtk::Clipboard::get (GDK_SELECTION_PRIMARY)->set_text (text);
		copy->set_label (_("Copied"));
	});

	Gtk::HBox* row = manage (new Gtk::HBox (false, 8));
	row->pack_start (*frame, true, true);
	Gtk::Alignment* a = manage (new Gtk::Alignment (0.5, 0.0, 0.0, 0.0));
	a->add (*copy);
	row->pack_start (*a, false, false);
	return row;
}

/* Numbered, copy-and-paste steps to raise the limit by hand, using nano. */
static Gtk::Widget*
memory_limit_steps ()
{
	struct Step {
		std::string text;
		std::string copy; // empty: nothing to copy
	};

	std::vector<Step> steps = {
		{ _("Open a terminal. On most Linux systems: press Ctrl+Alt+T."), "" },
		{ _("Add yourself to the \"audio\" group. Copy this, paste it into the terminal with Ctrl+Shift+V and press Enter. "
		    "Type your password when asked (nothing shows while you type) and press Enter."),
		  "sudo usermod -aG audio $USER" },
		{ _("Open the settings file in the nano text editor:"),
		  "sudo nano /etc/security/limits.d/95-audio.conf" },
		{ _("Copy these two lines and paste them into nano with Ctrl+Shift+V:"),
		  "@audio   -  rtprio     95\n@audio   -  memlock    unlimited" },
		{ _("Save and close nano: press Ctrl+O, then Enter, then Ctrl+X."), "" },
		{ _("Log out and log back in, or restart the computer."), "" },
		{ _("Optional: check that it worked. This should print \"unlimited\":"), "ulimit -l" },
	};

	Gtk::Table* t = manage (new Gtk::Table (steps.size () * 2, 2));
	t->set_col_spacings (12);
	t->set_row_spacings (6);

	int row = 0;
	int n   = 1;
	for (Step const& st : steps) {
		Gtk::Label* num = manage (new Gtk::Label);
		num->set_markup (string_compose ("<b>%1</b>", n++));
		num->set_alignment (1.0, 0.0);

		Gtk::Label* txt = manage (new Gtk::Label (st.text));
		txt->set_alignment (0.0, 0.0);
		txt->set_line_wrap (true);
		txt->set_size_request (560, -1);

		t->attach (*num, 0, 1, row, row + 1, Gtk::FILL, Gtk::FILL);
		t->attach (*txt, 1, 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL);
		++row;
		if (!st.copy.empty ()) {
			t->attach (*copyable_text (st.copy), 1, 2, row, row + 1, Gtk::FILL | Gtk::EXPAND, Gtk::FILL, 0, 2);
			++row;
		}
	}
	return t;
}

static void
show_memory_limit_steps (std::string const& intro)
{
	ArdourDialog d (_("Memory limit: fix it yourself"), true, false);
	Gtk::VBox* v = d.get_vbox ();
	v->set_spacing (14);
	v->set_border_width (18);

	Gtk::Label* head = manage (new Gtk::Label);
	head->set_markup (string_compose ("<span size=\"large\" weight=\"bold\">%1</span>", Gtkmm2ext::markup_escape_text (intro)));
	head->set_alignment (0.0, 0.5);
	head->set_line_wrap (true);
	v->pack_start (*head, false, false);
	v->pack_start (*memory_limit_steps (), false, false);

	d.add_button (_("Done"), RESPONSE_OK);
	d.set_default_response (RESPONSE_OK);
	v->show_all ();
	ARDOUR_UI_UTILS::enlarge_fonts (d, 1.3);
	d.run ();
}

/* Explain the locked-memory limit in plain words and offer to fix it.
 * The fix runs the `ardour-fix-memlock` helper as root via pkexec;
 * the steps to do it by hand are always one click away.
 */
static void
memory_limit_fix_dialog (rlim_t limit, int64_t ram)
{
	/* locate pkexec and our helper */
	std::string helper;
	PBD::Searchpath sp (ARDOUR::ardour_dll_directory ());
	bool have_helper = PBD::find_file (sp, X_("ardour-fix-memlock"), helper);

	gchar* pkexec_c = g_find_program_in_path ("pkexec");
	std::string pkexec = pkexec_c ? pkexec_c : "";
	g_free (pkexec_c);

	bool const can_fix   = have_helper && !pkexec.empty ();
	bool const relog     = user_listed_in_group ("audio") && !process_has_group ("audio");

	const std::string heading = string_compose (_("%1 can't reserve enough memory for glitch-free audio"), PROGRAM_NAME);
	std::string text = string_compose (
		_("Your system lets %1 lock %2 MB of memory, but your computer has %3 GB. "
		  "Locked memory can't be moved to the swap file, which prevents clicks and "
		  "dropouts while recording and playing back."),
		PROGRAM_NAME,
		(long long) (limit / (1024 * 1024)),
		ram > 0 ? string_compose ("%1", (int) ((ram + (1LL << 29)) >> 30)) : std::string ("?"));

	if (relog) {
		text += "\n\n" + std::string (_("You have been added to the \"audio\" group, but you haven't logged out "
		                                "since then. Log out and log back in, and this message should go away."));
	} else if (can_fix) {
		text += "\n\n" + string_compose (
			_("%1 can fix this for you. You'll be asked for your password, "
			  "and then you need to log out and log back in once."), PROGRAM_NAME);
	} else {
		text += "\n\n" + std::string (_("It takes a few minutes to fix by hand. Follow the steps below."));
	}

	ArdourDialog msg (_("Memory limit"), true, false);
	Gtk::VBox* v = msg.get_vbox ();
	v->set_spacing (14);
	v->set_border_width (18);

	Gtk::Label* head = manage (new Gtk::Label);
	head->set_markup (string_compose ("<span size=\"x-large\" weight=\"bold\">%1</span>", Gtkmm2ext::markup_escape_text (heading)));
	head->set_alignment (0.0, 0.5);
	head->set_line_wrap (true);
	Gtk::Label* body = manage (new Gtk::Label (text));
	body->set_alignment (0.0, 0.0);
	body->set_line_wrap (true);
	body->set_size_request (600, -1);
	v->pack_start (*head, false, false);
	v->pack_start (*body, false, false);

	if (!can_fix && !relog) {
		v->pack_start (*memory_limit_steps (), false, false);
	}

	CheckButton* cb = manage (new CheckButton (_("Do not show this window again")));
	v->pack_start (*cb, false, false, 6);

	const int RESPONSE_STEPS = 1;
	msg.add_button (_("Not now"), RESPONSE_CANCEL);
	if (can_fix && !relog) {
		msg.add_button (_("Show me how to do it myself"), RESPONSE_STEPS);
		msg.add_button (_("Fix this for me"), RESPONSE_ACCEPT);
		msg.set_default_response (RESPONSE_ACCEPT);
	} else {
		msg.set_default_response (RESPONSE_CANCEL);
	}

	v->show_all ();
	ARDOUR_UI_UTILS::enlarge_fonts (msg, 1.3);

	int const response = msg.run ();
	msg.hide ();

	if (cb->get_active ()) {
		XMLNode node (X_("no-memory-warning"));
		Config->add_instant_xml (node);
	}

	if (response == RESPONSE_STEPS) {
		show_memory_limit_steps (_("Raise the memory limit by hand"));
		return;
	}

	if (response != RESPONSE_ACCEPT || !can_fix) {
		return;
	}

	gchar* argv[] = { const_cast<gchar*> (pkexec.c_str ()), const_cast<gchar*> (helper.c_str ()), NULL };
	gint     status = -1;
	GError*  err    = NULL;
	bool     ok     = g_spawn_sync (NULL, argv, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &status, &err);

	if (err) {
		g_error_free (err);
		ok = false;
	}

	if (ok && WIFEXITED (status) && WEXITSTATUS (status) == 0) {
		ArdourMessageDialog done (
			string_compose (_("Done. Log out and log back in (or restart your computer), "
			                  "then start %1 again."), PROGRAM_NAME),
			false, MESSAGE_INFO, BUTTONS_OK, true);
		done.set_title (_("Memory limit"));
		ARDOUR_UI_UTILS::enlarge_fonts (done, 1.3);
		done.run ();
	} else if (ok && WIFEXITED (status) && (WEXITSTATUS (status) == 126 || WEXITSTATUS (status) == 127)) {
		/* 126: password dialog was cancelled, 127: not authorized. Nothing changed. */
		show_memory_limit_steps (_("Nothing was changed. You can do it yourself instead:"));
	} else {
		show_memory_limit_steps (_("The automatic fix didn't work. You can do it yourself instead:"));
	}
}

#endif

void
ARDOUR_UI::check_memory_locking ()
{
#if defined(__APPLE__) || defined(PLATFORM_WINDOWS)
	/* OS X doesn't support mlockall(2), and so testing for memory locking capability there is pointless */
	return;
#else // !__APPLE__

	XMLNode* memory_warning_node = Config->instant_xml (X_("no-memory-warning"));

	if (memory_warning_node == 0) {

		struct rlimit limits;
		int64_t ram;
		long pages, page_size;
#ifdef __FreeBSD__
		size_t pages_len=sizeof(pages);
		if ((page_size = getpagesize()) < 0 ||
				sysctlbyname("hw.availpages", &pages, &pages_len, NULL, 0))
#else
		if ((page_size = sysconf (_SC_PAGESIZE)) < 0 ||(pages = sysconf (_SC_PHYS_PAGES)) < 0)
#endif
		{
			ram = 0;
		} else {
			ram = (int64_t) pages * (int64_t) page_size;
		}

		if (getrlimit (RLIMIT_MEMLOCK, &limits)) {
			return;
		}

		if (limits.rlim_cur != RLIM_INFINITY) {

			if (ram == 0 || ((double) limits.rlim_cur / ram) < 0.75) {
#if defined(__FreeBSD__) || defined(__NetBSD__)
				ArdourMessageDialog msg (
					string_compose (
						_("WARNING: Your system has a limit for maximum amount of locked memory. "
						  "This might cause %1 to run out of memory before your system "
						  "runs out of memory. \n\n"
						  "You can view the memory limit with 'ulimit -l', "
						  "and it is normally controlled by %2"),
						PROGRAM_NAME, X_("/etc/login.conf")).c_str());

				msg.set_default_response (RESPONSE_OK);

				VBox* vbox = msg.get_vbox();
				HBox hbox;
				CheckButton cb (_("Do not show this window again"));
				hbox.pack_start (cb, true, false);
				vbox->pack_start (hbox);
				cb.show();
				vbox->show();
				hbox.show ();

				msg.run ();

				if (cb.get_active()) {
					XMLNode node (X_("no-memory-warning"));
					Config->add_instant_xml (node);
				}
#else
				memory_limit_fix_dialog (limits.rlim_cur, ram);
#endif
			}
		}
	}
#endif // !__APPLE__
}

void
ARDOUR_UI::load_from_application_api (const std::string& path)
{
	DEBUG_TRACE (DEBUG::GuiStartup, string_compose (X_("load_from_application_api %1\n"), path));
	/* OS X El Capitan (and probably later) now somehow passes the command
	   line arguments to an app via the openFile delegate protocol. Ardour
	   already does its own command line processing, and having both
	   pathways active causes crashes. So, if the command line was already
	   set, do nothing here. NSM also uses this code path.
	*/

	if (!ARDOUR_COMMAND_LINE::session_name.empty()) {
		return;
	}


	/* Cancel SessionDialog if it's visible to make OSX delegates work.
	 *
	 * ARDOUR_UI::starting connects app->ShouldLoad signal and then shows a SessionDialog
	 * race-condition:
	 *  - ShouldLoad does not arrive in time, ARDOUR_COMMAND_LINE::session_name is empty:
	 *    -> startupFSM starts a SessionDialog.
	 *  - ShouldLoad signal arrives, this function is called and sets ARDOUR_COMMAND_LINE::session_name
	 *    -> SessionDialog is not displayed
	 */

	if (startup_fsm) {
		/* this will result in the StartupFSM signalling us to load a
		 * session, which if successful will then destroy the
		 * startupFSM and we'll move right along.
		 */

		startup_fsm->handle_path (path);
		return;
	}

	if (nsm) {
		unload_session(false, true);

		if (!AudioEngine::instance()->set_backend(
#if ! (defined(__APPLE__) || defined(PLATFORM_WINDOWS))
					"JACK/Pipewire",
#else
					"JACK",
#endif
					ARDOUR_COMMAND_LINE::backend_client_name, "")) {
			error << _("NSM: The JACK backend is mandatory and can not be loaded.") << endmsg;
			return;
		}

		if (!AudioEngine::instance()->running()) {
			/* this auto-starts jackd with recent settings
			 * TODO: if Glib::file_test (path, Glib::FILE_TEST_IS_DIR)
			 *  query sample-rate of the session and use it.
			 */
			if (Glib::file_test (path, Glib::FILE_TEST_IS_DIR)) {
				float sr;
				SampleFormat sf;
				string pv;
				if (0 == Session::get_info_from_path (Glib::build_filename (path, basename_nosuffix (path) + statefile_suffix), sr, sf, pv)) {
					ARDOUR::AudioEngine::instance ()->set_sample_rate (sr);
				}
			}
			if (AudioEngine::instance()->start()) {
				error << string_compose (_("NSM: %1 cannot connect to the JACK server. Please start jackd first."), PROGRAM_NAME) << endmsg;
				return;
			}
		}

		if (!nsm_first_session_opened) {
			PluginScanDialog psd (true, false);
			psd.start ();

			post_engine ();
			nsm_first_session_opened = true;
		}
	}

	/* the mechanisms that can result is this being called are only
	 * possible for existing sessions.
	 */

	if (!Glib::file_test (path, Glib::FILE_TEST_EXISTS)) {
		if (nsm) {
			BusProfile bus_profile;
			bus_profile.master_out_channels = 2;
			build_session (path, basename_nosuffix (path), "", bus_profile, true, false, AudioTime);
		}
		return;
	}

	ARDOUR_COMMAND_LINE::session_name = path;

	int rv;

	if (Glib::file_test (path, Glib::FILE_TEST_IS_DIR)) {
		/* /path/to/foo => /path/to/foo, foo */
		rv = load_session (path, basename_nosuffix (path));
	} else {
		/* /path/to/foo/foo.ardour => /path/to/foo, foo */
		rv = load_session (Glib::path_get_dirname (path), basename_nosuffix (path));
	}

	// there was no startupFSM, load_session fails, and there is no existing session ....

	if (rv && !_session) {

		ARDOUR_COMMAND_LINE::session_name = string();

		/* do this again */

		EngineControl* amd;

		try {
			amd = dynamic_cast<EngineControl*> (audio_midi_setup.get (true));
		} catch (...) {
			std::cerr << "audio-midi engine setup failed."<< std::endl;
			return;
		}

		startup_fsm = new StartupFSM (*amd);
		startup_fsm->signal_response().connect (sigc::mem_fun (*this, &ARDOUR_UI::sfsm_response));

		/* Note: entire startup process could happen in this one call
		 * if:
		 *
		 * 1) not a new user
		 * 2) session name provided on command line (and valid)
		 * 3) no audio/MIDI setup required
		 */

		Splash::instance()->exists(); // create splash
		startup_fsm->start ();
	}
}
