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

#include <cstring>
#include <map>
#include <sstream>

#include <glibmm/main.h>
#include <glibmm/markup.h>
#include <ytkmm/action.h>
#include <ytkmm/menuitem.h>
#include <ytkmm/toggleaction.h>

#include "gtkmm2ext/actions.h"
#include "gtkmm2ext/activatable.h"
#include "gtkmm2ext/bindings.h"
#include "gtkmm2ext/utils.h"

#include "quick_help.h"
#include "ui_config.h"

#include "pbd/compose.h"
#include "pbd/i18n.h"

using std::string;

QuickHelp&
QuickHelp::instance ()
{
	static QuickHelp* q = new QuickHelp;
	return *q;
}

QuickHelp::QuickHelp ()
	: _canvas_item (-1)
	, _canvas_widget (0)
{
	_mark.set_markup ("<span weight=\"bold\" foreground=\"#4c9aff\" size=\"large\">?</span>");
	_title.set_alignment (0.0, 0.5);
	_text.set_alignment (0.0, 0.5);
	_text.set_ellipsize (Pango::ELLIPSIZE_END);
	_key.set_alignment (1.0, 0.5);

	_box.set_spacing (12);
	_box.set_border_width (7);
	_box.pack_start (_mark, false, false);
	_box.pack_start (_title, false, false);
	_box.pack_start (_text, true, true);
	_box.pack_start (_key, false, false);
	add (_box);
	_box.show_all ();

	show_help ("", "", "");
	UIConfiguration::instance ().ParameterChanged.connect (sigc::hide (sigc::mem_fun (*this, &QuickHelp::sync_visibility)));
	sync_visibility ();
}

void
QuickHelp::sync_visibility ()
{
	const bool yn = UIConfiguration::instance ().get_show_quick_help ();
	if (yn) {
		show ();
		if (!_timer.connected ()) {
			_timer = Glib::signal_timeout ().connect (sigc::mem_fun (*this, &QuickHelp::poll), 150);
		}
	} else {
		hide ();
		_timer.disconnect ();
	}
	Glib::RefPtr<Gtk::ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleQuickHelp"), false);
	if (tact && tact->get_active () != yn) {
		tact->set_active (yn);
	}
}

void
QuickHelp::toggled ()
{
	Glib::RefPtr<Gtk::ToggleAction> tact = ActionManager::get_toggle_action (X_("Editor"), X_("ToggleQuickHelp"), false);
	if (tact && tact->get_active () != UIConfiguration::instance ().get_show_quick_help ()) {
		UIConfiguration::instance ().set_show_quick_help (tact->get_active ());
	}
}

void
QuickHelp::set_canvas_item (int item_type, bool entered)
{
	QuickHelp& q (instance ());
	if (!entered) {
		if (q._canvas_item == item_type) {
			q._canvas_item = -1;
		}
		return;
	}
	q._canvas_item = item_type;
	gpointer   ud = 0;
	GdkWindow* w  = gdk_window_at_pointer (0, 0);
	if (w) {
		gdk_window_get_user_data (w, &ud);
	}
	q._canvas_widget = ud;
}

void
QuickHelp::set (Gtk::Widget& w, const char* title, const char* text)
{
	g_object_set_data (G_OBJECT (w.gobj ()), "quick-help-title", (gpointer) title);
	g_object_set_data (G_OBJECT (w.gobj ()), "quick-help-text", (gpointer) text);
}

void
QuickHelp::show_help (string const& title, string const& text, string const& key)
{
	const string all = title + '\n' + text + '\n' + key;
	if (all == _shown) {
		return;
	}
	_shown = all;

	if (title.empty () && text.empty ()) {
		_title.set_markup (string_compose ("<span size=\"large\" weight=\"bold\">%1</span>", _("Quick Help")));
		_text.set_markup (string_compose ("<span size=\"large\">%1</span>", _("Point at anything to see what it does.  Hide this strip: View > Quick Help")));
		_key.set_text ("");
		return;
	}
	_title.set_markup (string_compose ("<span size=\"large\" weight=\"bold\">%1</span>", Gtkmm2ext::markup_escape_text (title)));
	_text.set_markup (string_compose ("<span size=\"large\">%1</span>", Gtkmm2ext::markup_escape_text (text)));
	_text.set_tooltip_text ("");
	if (key.empty ()) {
		_key.set_text ("");
	} else {
		_key.set_markup (string_compose ("<span size=\"large\" font_family=\"monospace\"> %1 </span>", Gtkmm2ext::markup_escape_text (key)));
	}
}

/* ---- helpers ---- */

static string
strip_markup (string const& s)
{
	string out;
	bool   tag = false;
	for (char c : s) {
		if (c == '<') {
			tag = true;
		} else if (c == '>' && tag) {
			tag = false;
		} else if (!tag) {
			out += c;
		}
	}
	static const char* ent[][2] = { { "&amp;", "&" }, { "&lt;", "<" }, { "&gt;", ">" }, { "&quot;", "\"" }, { "&apos;", "'" } };
	for (auto const& e : ent) {
		string::size_type p;
		while ((p = out.find (e[0])) != string::npos) {
			out.replace (p, strlen (e[0]), e[1]);
		}
	}
	return out;
}

static string
trim (string s)
{
	while (!s.empty () && g_ascii_isspace (s[s.size () - 1])) {
		s.erase (s.size () - 1);
	}
	while (!s.empty () && g_ascii_isspace (s[0])) {
		s.erase (0, 1);
	}
	return s;
}

static string
lower (string const& s)
{
	char*  l = g_utf8_strdown (s.c_str (), -1);
	string r (l);
	g_free (l);
	return r;
}

static QuickHelp::Text const*
find_text (string const& key, bool prefix_ok = true)
{
	static std::map<string, QuickHelp::Text const*> index;
	if (index.empty ()) {
		for (QuickHelp::Text const* t = QuickHelp::widget_texts; t->key; ++t) {
			index[t->key] = t;
		}
	}
	const string k = lower (key);
	std::map<string, QuickHelp::Text const*>::const_iterator i = index.find (k);
	if (i != index.end ()) {
		return i->second;
	}
	if (!prefix_ok) {
		return 0;
	}
	/* long tooltips: match on how they begin */
	for (i = index.begin (); i != index.end (); ++i) {
		if (i->first.size () >= 9 && k.compare (0, i->first.size (), i->first) == 0) {
			return i->second;
		}
	}
	return 0;
}

static string
key_for_action (Glib::RefPtr<Gtk::Action> const& act)
{
	if (!act) {
		return string ();
	}
	for (Gtkmm2ext::Bindings* b : Gtkmm2ext::Bindings::bindings) {
		Gtkmm2ext::Bindings::Operation op;
		string k = b->get_binding_for_action (act, op).display_label ();
		if (!k.empty ()) {
			string out;
			for (char c : k) {
				if (c == '<') {
					continue;
				}
				out += (c == '>') ? '+' : c;
			}
			return out;
		}
	}
	return string ();
}

bool
QuickHelp::poll ()
{
	gpointer   ud = 0;
	GdkWindow* w  = gdk_window_at_pointer (0, 0);
	if (w) {
		gdk_window_get_user_data (w, &ud);
	}
	if (!ud || !GTK_IS_WIDGET (ud)) {
		return true; /* outside our windows: leave the last text up */
	}

	for (GtkWidget* p = GTK_WIDGET (ud); p; p = gtk_widget_get_parent (p)) {
		if (p == GTK_WIDGET (gobj ())) {
			return true; /* the strip itself: keep what it says */
		}

		if (const char* own = (const char*) g_object_get_data (G_OBJECT (p), "quick-help-title")) {
			show_help (_(own), _((const char*) g_object_get_data (G_OBJECT (p), "quick-help-text")), "");
			return true;
		}

		/* menu items: the label is the key */
		if (GTK_IS_MENU_ITEM (p)) {
			Gtk::MenuItem* mi = Glib::wrap (GTK_MENU_ITEM (p));
			Gtk::Label*    l  = mi ? dynamic_cast<Gtk::Label*> (mi->get_child ()) : 0;
			if (!l) {
				break;
			}
			const string label = trim (l->get_text ());
			string       text, key;
			GtkAction*   ga = gtk_activatable_get_related_action (GTK_ACTIVATABLE (p));
			if (ga) {
				Glib::RefPtr<Gtk::Action> act = Glib::wrap (ga, true);
				key  = key_for_action (act);
				text = strip_markup (act->property_tooltip ().get_value ());
			}
			if (Text const* t = find_text (label, false)) {
				show_help (_(t->title), _(t->text), key);
			} else {
				show_help (label, text == label ? string () : text, key);
			}
			return true;
		}

		gchar* tip = gtk_widget_get_tooltip_markup (p);
		if (!tip || !*tip) {
			g_free (tip);
			continue;
		}
		const string all = strip_markup (tip);
		g_free (tip);

		/* first line is what it is; the rest explains; "Shortcut: x" is the key */
		string first, rest, key;
		std::stringstream ss (all);
		string            line;
		const string      sc = trim (_("\n\nShortcut: "));
		while (std::getline (ss, line)) {
			line = trim (line);
			if (line.empty ()) {
				continue;
			}
			if (line.compare (0, sc.size (), sc) == 0) {
				key = trim (line.substr (sc.size ()));
			} else if (first.empty ()) {
				first = line;
			} else {
				rest += (rest.empty () ? "" : "  ") + line;
			}
		}
		if (key.empty ()) {
			Gtk::Widget* wp = Glib::wrap (p);
			if (Gtkmm2ext::Activatable* a = dynamic_cast<Gtkmm2ext::Activatable*> (wp)) {
				key = key_for_action (a->get_related_action ());
			}
		}
		for (char& c : key) {
			if (c == '-' && &c != &key[key.size () - 1]) {
				c = '+';
			}
		}
		if (Text const* t = find_text (first)) {
			show_help (_(t->title), _(t->text), key);
		} else {
			show_help (first, rest, key);
		}
		return true;
	}

	if (_canvas_item >= 0 && ud == _canvas_widget) {
		const char* title = 0;
		const char* text  = canvas_text (_canvas_item, title);
		if (text) {
			show_help (_(title), _(text), "");
			return true;
		}
	}

	show_help ("", "", "");
	return true;
}
