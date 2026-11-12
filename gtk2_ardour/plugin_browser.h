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
#include <utility>
#include <vector>

#include <ytkmm/box.h>
#include <ytkmm/drawingarea.h>
#include <ytkmm/entry.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/label.h>
#include <ytkmm/menu.h>
#include <ytkmm/scrollbar.h>
#include <ytkmm/table.h>
#include <ytkmm/textview.h>
#include <ytkmm/window.h>

#include "pbd/signals.h"

#include "ardour/plugin.h"

#include "widgets/ardour_button.h"

namespace ARDOUR {
	class Route;
}

class PluginList;
class PluginPlate;
class TrackPicker;

/** The plugin browser: one window to find a plugin among hundreds and put it
 * on a track.
 *
 * Laid out around the middle: effects/instruments, a narrow search bar and the
 * track to add to along the top; the kinds on the left, the plugins in the
 * middle, the one you are pointing at on the right. A click adds a plugin to
 * the track and the window stays open for the next one. Esc closes it.
 */
class PluginBrowser : public Gtk::Window
{
public:
	/** Open the browser for `route` (or, with none, the track selected in the
	 * editor). `placement` is where in the track's plugin list an effect goes,
	 * -1 for before the fader. */
	static void show_for (std::shared_ptr<ARDOUR::Route> route, int placement = -1);

	/** Open the browser for the selected track. */
	static void show_for_selected ();

	/* ---- text and colours the browser knows ---- */
	struct Described { const char* name; const char* text; };
	struct Skin { const char* maker; uint32_t light; uint32_t dark; uint32_t ink; };
	static const Described descriptions[];
	static const Skin      skins[];

	/** what a thing looks like on its screen */
	enum Show { Curve, Meter, Tail, Echo, Clip, Wave, Keys, Pads, Sample };

	struct KindInfo {
		const char* id;
		const char* label;
		const char* one;   ///< "reverb": for "your go-to reverb"
		const char* brief; ///< what it is, in a few words
		const char* tip;   ///< what it is for, in a sentence
		uint32_t    glow;
		Show        show;
		bool        instrument;
		const char* words; ///< space separated: what people call it
	};

	/** One plugin as the browser sees it: all its formats are one entry. */
	struct Entry {
		std::string name;
		std::string lname;
		std::string maker;
		std::string lmaker;
		std::string tags; ///< lower case, space separated
		std::string builtin;
		std::string note;
		int         kind;
		bool        instrument;
		bool        favourite;
		bool        go_to;
		int64_t     last_used;
		uint64_t    uses;
		int64_t     first_seen;
		std::vector<ARDOUR::PluginInfoPtr> variants; ///< best format first
		size_t      variant;
		uint32_t    light, dark, ink;
		unsigned    seed;
		/* while searching */
		double                         score;
		std::vector<std::pair<int,int> > hits;

		std::string description () const;
		bool is_new () const;
	};

	struct Row {
		bool        header;
		std::string group;
		std::string note;
		int         entry;
		std::string badge;
		bool        quiet;
	};

	static const KindInfo* kinds ();
	static int             n_kinds ();

	~PluginBrowser ();

private:
	PluginBrowser ();

	/* the window's parts */
	Gtk::EventBox*         _top;
	ArdourWidgets::ArdourButton _tab_fx;
	ArdourWidgets::ArdourButton _tab_inst;
	Gtk::Entry             _search;
	ArdourWidgets::ArdourButton _chip;
	Gtk::Table             _tiles;
	ArdourWidgets::ArdourButton _tile_all;
	ArdourWidgets::ArdourButton _tile_fav;
	ArdourWidgets::ArdourButton _tile_new;
	std::vector<ArdourWidgets::ArdourButton*> _tile_kind;
	ArdourWidgets::ArdourButton _manage;
	PluginList*            _list;
	Gtk::VScrollbar*       _scroll;
	PluginPlate*           _plate;
	Gtk::Label             _name;
	Gtk::Label             _kind_maker;
	ArdourWidgets::ArdourButton _format;
	Gtk::Label             _desc;
	Gtk::VBox              _facts;
	Gtk::Label             _fact_goto;
	Gtk::Label             _fact_since;
	Gtk::Label             _fact_used;
	Gtk::EventBox          _notes_link;
	Gtk::Label             _notes_label;
	Gtk::VBox              _note_edit;
	Gtk::TextView          _note_text;
	Gtk::Label             _foot;
	Gtk::Label             _rack_title;
	Gtk::DrawingArea*      _rack;
	Gtk::Label             _rack_msg;
	Gtk::VBox*             _right;

	/* the data */
	std::vector<Entry>     _entries;
	std::vector<Row>       _rows;
	std::map<std::string, std::pair<std::string, int64_t> > _meta; ///< name -> (your note, first seen)
	std::weak_ptr<ARDOUR::Route> _origin;
	TrackPicker*           _picker;
	sigc::connection       _picker_open_timer;
	sigc::connection       _picker_close_timer;
	bool                   _loaded;

	/* where it is */
	std::weak_ptr<ARDOUR::Route> _target;
	int                    _placement;
	bool                   _instruments; ///< the Instruments tab
	int                    _filter;      ///< -1 all, -2 favourites, -3 new, else a kind index
	int                    _sel;         ///< index into _rows, or -1
	bool                   _editing;
	std::string            _tip;
	PBD::ScopedConnection  _route_connection;
	PBD::ScopedConnection  _manager_connection;
	sigc::connection       _idle;

	void load_entries ();
	void refresh_stats ();
	void load_meta ();
	void save_meta () const;
	void rebuild ();
	void rebuild_soon ();
	void search_changed ();
	void set_tab (bool instruments);
	void set_filter (int f);
	void update_tiles ();
	void set_target (std::shared_ptr<ARDOUR::Route>);
	void choose_target ();
	void close_picker ();
	void picker_entered ();
	void picker_left ();
	void picked (std::shared_ptr<ARDOUR::Route>);
	void update_chip ();
	void select (int row);
	void step (int delta);
	void show_selected ();
	void add_entry (int entry);
	void toggle_star (int entry);
	void cycle_format ();
	void start_edit ();
	void end_edit (bool save);
	void set_foot (std::string const&);
	void rack_changed ();
	bool rack_expose (GdkEventExpose*);
	void open_manager ();
	void list_scrolled ();
	std::string tile_tip (int filter) const;

	bool search_key (GdkEventKey*);
	bool on_key_press_event (GdkEventKey*);
	void on_show ();
	void on_hide ();

	friend class PluginList;
	friend class PluginPlate;
	friend class TrackPicker;
};
