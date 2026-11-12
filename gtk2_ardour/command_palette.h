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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <ytkmm/entry.h>
#include <ytkmm/label.h>
#include <ytkmm/liststore.h>
#include <ytkmm/action.h>
#include <ytkmm/box.h>
#include <ytkmm/scrolledwindow.h>
#include <ytkmm/treeview.h>
#include <ytkmm/window.h>

#include "pbd/signals.h"

#include "temporal/timeline.h"

#include "ardour/plugin.h"

namespace ARDOUR {
	class Route;
}

/** The Ctrl+Space search: one box that finds and runs any menu command,
 * jumps to a track or marker, or adds a plugin to the selected track.
 */
class CommandPalette : public Gtk::Window
{
public:
	/** show the search box over the main window (or hide it if shown) */
	static void toggle ();

private:
	CommandPalette ();

	enum Kind {
		Header,
		Command,
		Track,
		Marker,
		Plugin
	};

	struct Item {
		Kind                         kind;
		std::string                  title;
		std::string                  where;
		std::string                  key;
		std::string                  id; ///< stable id, used for "recently used"
		Glib::RefPtr<Gtk::Action>    action;
		std::weak_ptr<ARDOUR::Route> route;
		Temporal::timepos_t          pos;
		ARDOUR::PluginInfoPtr        plugin;
		int                          score;
		bool                         available = true;
		std::vector<size_t>          hits;
	};

	struct Columns : public Gtk::TreeModel::ColumnRecord {
		Columns () { add (text); add (key); add (index); add (selectable); }
		Gtk::TreeModelColumn<std::string> text;
		Gtk::TreeModelColumn<std::string> key;
		Gtk::TreeModelColumn<int>         index;
		Gtk::TreeModelColumn<bool>        selectable;
	};

	Gtk::VBox                    _box;
	Gtk::Entry                   _entry;
	Gtk::ScrolledWindow          _scroller;
	Gtk::TreeView                _view;
	Glib::RefPtr<Gtk::ListStore> _model;
	Columns                      _columns;
	Gtk::Label                   _footer;

	std::vector<Item>        _items; ///< everything that can be found
	std::vector<Item>        _shown; ///< what the list shows now (incl. headers)
	std::vector<std::string> _recent;

	static std::map<std::string, std::string> _key_cache; ///< action path -> shortcut label
	PBD::ScopedConnection _bindings_connection;

	void collect ();
	void collect_commands ();
	void collect_tracks ();
	void collect_markers ();
	void collect_plugins ();
	void refilter ();
	void select_row (int);
	int  selected_row () const;
	void step (int delta);
	void run (size_t shown_index);
	void add_plugin (ARDOUR::PluginInfoPtr);
	void remember (std::string const& id);
	void load_recent ();
	void save_recent ();

	bool entry_key_press (GdkEventKey*);
	bool view_button_release (GdkEventButton*);
	bool on_focus_out_event (GdkEventFocus*);
	bool on_key_press_event (GdkEventKey*);
	void on_show ();

	std::string highlighted (Item const&) const;
};
