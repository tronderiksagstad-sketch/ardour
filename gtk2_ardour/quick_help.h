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

#include <string>

#include <ytkmm/box.h>
#include <ytkmm/eventbox.h>
#include <ytkmm/label.h>

/** Quick Help: a strip along the bottom of the main window that says, in a
 * sentence or two, what the thing under the mouse pointer is and does.
 *
 * It works everywhere without each window having to cooperate: a few times
 * a second it looks up the widget under the pointer and uses, in order,
 *  - our own plain-language text for it (quick_help_texts.cc), found by the
 *    widget's tooltip, a menu item's label, or the kind of canvas item;
 *  - otherwise the widget's tooltip as it is.
 */
class QuickHelp : public Gtk::EventBox
{
public:
	static QuickHelp& instance ();

	/** the editor tells us which kind of canvas item (ItemType) the pointer entered or left */
	static void set_canvas_item (int item_type, bool entered);

	/** give a widget its own help; title and text must be string literals (they are not copied) */
	static void set (Gtk::Widget&, const char* title, const char* text);

	/** called by the View > Quick Help toggle */
	static void toggled ();

	void sync_visibility ();

	struct Text {
		const char* key;   /* tooltip (first line) or menu label, lower case */
		const char* title;
		const char* text;
	};
	static const Text  widget_texts[];
	static const char* canvas_text (int item_type, const char*& title);

private:
	QuickHelp ();
	bool poll ();
	void show_help (std::string const& title, std::string const& text, std::string const& key);

	Gtk::HBox  _box;
	Gtk::Label _mark;
	Gtk::Label _title;
	Gtk::Label _text;
	Gtk::Label _key;

	std::string      _shown;
	sigc::connection _timer;
	int              _canvas_item;
	void*            _canvas_widget;
};
