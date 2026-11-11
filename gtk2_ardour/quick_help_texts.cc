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

/* The words Quick Help shows. Plain language, one or two short sentences.
 *
 * key: the first line of the widget's tooltip, or a menu item's label, in
 * lower case. Anything without an entry here still shows its tooltip.
 */

#include "editor_items.h"
#include "quick_help.h"

#include "pbd/i18n.h"

const QuickHelp::Text QuickHelp::widget_texts[] = {
	/* ---- transport ---- */
	{ "play from playhead", N_("Play"), N_("Plays from the playhead (the red line). Press again to stop.") },
	{ "stop playback", N_("Stop"), N_("Stops playback or recording.") },
	{ "toggle record", N_("Record"), N_("Arms recording. Then press Play: every track with its red button on records.") },
	{ "play range/selection", N_("Play selection"), N_("Plays only the time range or regions you have selected.") },
	{ "go to start of session", N_("Go to start"), N_("Moves the playhead back to the beginning of the session.") },
	{ "go to end of session", N_("Go to end"), N_("Moves the playhead to the end of the session.") },
	{ "play loop range", N_("Loop"), N_("Plays the loop range round and round. Drag in the ruler with the Range tool to set it.") },
	{ "midi panic", N_("MIDI panic"), N_("Stops stuck notes: tells every instrument to go silent.") },
	{ "enable/disable metronome", N_("Metronome"), N_("A click on every beat while you play or record. Right-click for its settings, scroll to change its volume.") },
	{ "return to last playback start when stopped", N_("Return on stop"), N_("When on, the playhead jumps back to where you started each time you stop.") },
	{ "playhead follows range tool clicks, and range selections", N_("Follow range"), N_("When on, the playhead jumps to wherever you click or select with the Range tool.") },
	{ "be sensible about external positional sync", N_("Sync"), N_("Follows another machine's clock. Leave it off unless you sync Ardour to other gear.") },
	{ "enable/disable audio click", N_("Metronome"), N_("A click on every beat while you play or record.") },
	{ "speed display (context-click for options)", N_("Speed"), N_("Shows how fast playback is running. 1.0 is normal speed.") },
	{ "shuttle speed control (context-click for options)", N_("Shuttle"), N_("Drag left or right to play slower, faster or backwards. Let go to return.") },
	{ "varispeed: change the default playback and recording speed", N_("Varispeed"), N_("Plays and records the whole session faster or slower, like a tape machine. Pitch changes too.") },
	{ "start recording at auto-punch start", N_("Punch in"), N_("Recording starts by itself when the playhead reaches the start of the punch range.") },
	{ "stop recording at auto-punch end", N_("Punch out"), N_("Recording stops by itself at the end of the punch range.") },
	{ "set the session punch-range from the selected time range, and enable punch in/out", N_("Punch"), N_("Records only inside the punch range, so you can replace one phrase without touching the rest.") },
	{ "layered", N_("Record mode"), N_("What happens when you record over something. Layered keeps the old take underneath; the others replace it.") },
	{ "when active, something is soloed.", N_("Solo light"), N_("Flashes when any track is soloed. Click it to switch all solos off.") },
	{ "when active, auditioning is taking place.", N_("Audition light"), N_("Lights while you are previewing a file. Click to stop the preview.") },
	{ "when lit, there is a ports feedback loop.", N_("Feedback light"), N_("Lights when an output is routed back into its own input. Check your connections.") },
	{ "master output volume", N_("Master volume"), N_("The overall loudness of everything you hear and export.") },
	{ "measure loudness of the session, normalize master output volume", N_("Loudness"), N_("Measures how loud the whole session is and can set the master volume to a standard level, for streaming for example.") },
	{ "show/hide monitoring section", N_("Monitor section"), N_("Shows the listening controls: speaker volume, dim, mono. They change what you hear, not what is exported.") },

	{ "primary clock", N_("Clock"), N_("Where the playhead is. Click a number and type to jump there. Right-click to show bars and beats, minutes and seconds, or timecode.") },
	{ "secondary clock", N_("Second clock"), N_("The playhead position again, in another format. Right-click to choose the format.") },
	{ "i/o latency", N_("Latency"), N_("The delay between playing a note and hearing it through Ardour. Lower it with a smaller buffer in Audio/MIDI Setup.") },
	{ "dsp:", N_("DSP load"), N_("How hard the computer is working on the audio. If it gets near 100% you will hear clicks: use a bigger buffer or fewer plugins.") },

	/* ---- status bar ---- */
	{ "double click to open session folder.", N_("Session folder"), N_("Where this session is saved. Double-click to open the folder.") },
	{ "double click to edit audio file format.", N_("File format"), N_("The kind of audio files this session records. Double-click to change it.") },
	{ "double click to change timecode settings.", N_("Timecode"), N_("The frame rate used when working to video. Double-click to change it.") },
	{ "double click to show audio/midi setup.", N_("Sample rate"), N_("The audio quality the session runs at. Double-click to open Audio/MIDI Setup.") },
	{ "unknown", N_("Disk space"), N_("How much more recording time fits on the disk.") },

	/* ---- editor toolbar ---- */
	{ "smart mode (add range functions to grab mode)", N_("Smart mode"), N_("Makes the Grab tool also select time ranges when you drag in the top half of a track.") },
	{ "grab mode (select/move objects)", N_("Grab tool"), N_("Select and move regions. Drag a region's edge to trim it.") },
	{ "range mode (select time ranges)", N_("Range tool"), N_("Drag across tracks to select a stretch of time, to cut, copy, loop or export it.") },
	{ "cut mode (split regions)", N_("Cut tool"), N_("Click on a region to split it in two at that point.") },
	{ "draw mode (draw and edit gain/notes/automation)", N_("Draw tool"), N_("Draw notes in MIDI regions, and volume or automation curves.") },
	{ "grid mode (edit tempo-map, drag/drop music-time grid)", N_("Grid tool"), N_("Drag bar lines to make the grid fit music that was played without a click.") },
	{ "stretch mode (time-stretch audio and midi regions, preserving pitch)", N_("Stretch tool"), N_("Drag a region's edge to make it longer or shorter without changing its pitch.") },
	{ "internal edit mode (edit notes and automation points)", N_("Edit inside"), N_("Work on the notes or points inside a region instead of the region itself.") },
	{ "audition mode (listen to regions)", N_("Listen tool"), N_("Click a region to hear it on its own.") },
	{ "snap mode", N_("Snap"), N_("When on, things you move jump to the nearest grid line. Right-click for snap settings.") },
	{ "grid mode", N_("Grid"), N_("How fine the grid lines are. Snap and Quantize use this.") },
	{ "edit point", N_("Edit point"), N_("Where cut, paste and split happen: at the mouse pointer, the playhead, or a marker.") },
	{ "edit mode", N_("Edit mode"), N_("What happens to later regions when you move or delete one. Slide leaves them where they are; Ripple closes the gap.") },
	{ "ripple mode", N_("Ripple"), N_("Which tracks are shifted when Ripple closes a gap.") },
	{ "nudge region/selection later", N_("Nudge later"), N_("Moves what is selected a small step to the right.") },
	{ "nudge region/selection earlier", N_("Nudge earlier"), N_("Moves what is selected a small step to the left.") },
	{ "nudge clock", N_("Nudge size"), N_("How far one nudge moves things.") },
	{ "zoom in", N_("Zoom in"), N_("Shows a shorter stretch of time, in more detail.") },
	{ "zoom out", N_("Zoom out"), N_("Shows a longer stretch of time.") },
	{ "zoom to session", N_("Zoom to fit"), N_("Shows the whole session from start to end.") },
	{ "zoom to time scale", N_("Zoom to length"), N_("Zooms so the window shows a set length of time.") },
	{ "zoom focus", N_("Zoom focus"), N_("What stays in place when you zoom: the playhead, the mouse or the edge of the window.") },
	{ "expand tracks", N_("Taller tracks"), N_("Makes the selected tracks (or all tracks) taller.") },
	{ "shrink tracks", N_("Shorter tracks"), N_("Makes the selected tracks (or all tracks) shorter.") },
	{ "number of visible tracks", N_("Tracks shown"), N_("Sets track height so this many tracks fit in the window.") },
	{ "note length to draw (auto uses the current grid setting)", N_("Note length"), N_("How long the notes you draw are.") },
	{ "note velocity to draw (auto uses the nearest note's velocity)", N_("Note velocity"), N_("How hard the notes you draw are played.") },
	{ "note channel to draw (auto uses the nearest note's channel)", N_("Note channel"), N_("Which MIDI channel the notes you draw use.") },
	{ "play notes when selected", N_("Hear notes"), N_("Plays a note when you click or move it.") },
	{ "switch between sustained and percussive mode", N_("Note shape"), N_("Shows notes as bars with a length, or as diamonds for drums where only the start matters.") },
	{ "select visible midi channel", N_("MIDI channel"), N_("Which MIDI channel's notes you see and edit.") },
	{ "show the mixer window", N_("Open mixer"), N_("Opens the full mixer with every track's fader.") },
	{ "groups: click to (de)activate", N_("Groups"), N_("Coloured tabs for tracks that are grouped, so they mute, solo or move together. Click to switch a group on or off.") },

	/* ---- track headers and mixer strips ---- */
	{ "record", N_("Record-enable"), N_("This track will record when you press Record and Play.") },
	{ "record (right-click for step edit)", N_("Record-enable"), N_("This track will record when you press Record and Play.") },
	{ "mute", N_("Mute"), N_("Silences this track.") },
	{ "mute this track", N_("Mute"), N_("Silences this track.") },
	{ "solo", N_("Solo"), N_("Plays only this track, and any others that are soloed.") },
	{ "mute other (non-soloed) tracks", N_("Solo"), N_("Plays only this track, and any others that are soloed.") },
	{ "after-fade listen (afl)", N_("Listen"), N_("Lets you hear this track on its own in the monitor speakers, after its fader.") },
	{ "pre-fade listen (pfl)", N_("Listen"), N_("Lets you hear this track on its own in the monitor speakers, before its fader.") },
	{ "invert phase (polarity)", N_("Phase"), N_("Flips the waveform upside down. Try it when two microphones on the same source sound thin together.") },
	{ "playlist", N_("Playlist"), N_("The takes on this track. Make a new playlist to try another version and switch back at any time.") },
	{ "automation", N_("Automation"), N_("Shows lanes under the track where you draw volume, pan or plugin changes over time.") },
	{ "midi controllers and automation", N_("Automation"), N_("Shows lanes under the track for volume, pan, plugin and MIDI controller changes over time.") },
	{ "group", N_("Group"), N_("Puts this track in a group, so grouped tracks mute, solo, record or are edited together.") },
	{ "mix group", N_("Group"), N_("Puts this track in a group, so grouped tracks mute, solo, record or are edited together.") },
	{ "track/bus name (double click to edit)", N_("Name"), N_("Double-click to rename.") },
	{ "clip indicator: lights red when this track clips. click to reset.", N_("Clip light"), N_("Turns red when the track is too loud and distorts. Turn the level down at the source or with the fader.") },
	{ "dbfs - digital peak hold. click to reset.", N_("Peak"), N_("The loudest level reached so far. Red means it clipped. Click to reset.") },
	{ "metering point", N_("Meter point"), N_("Where in the track the meter measures: at the input, before the fader or after it.") },
	{ "fader automation mode", N_("Fader automation"), N_("Play follows the volume curve you drew. Write and Touch record your fader moves while playing.") },
	{ "pan automation mode", N_("Pan automation"), N_("Play follows the pan curve you drew. Write and Touch record your pan moves while playing.") },
	{ "hide this mixer strip", N_("Hide strip"), N_("Hides this strip from the mixer. The track itself is not removed.") },
	{ "realtime analyzer", N_("Analyzer"), N_("Shows which frequencies this track contains, live.") },
	{ "enable/disable midi input", N_("MIDI input"), N_("Lets notes from your keyboard reach this track.") },
	{ "double-click to edit the route color.", N_("Track colour"), N_("Double-click to change the colour. Right-click for more track options.") },
	{ "monitor input", N_("Hear input"), N_("You hear what is coming in to the track (the microphone) instead of what is recorded.") },
	{ "monitor playback", N_("Hear recording"), N_("You hear what is recorded on the track instead of its input.") },
	{ "solo isolate", N_("Solo isolate"), N_("Keeps this track playing when others are soloed. Useful for reverb and effect busses.") },
	{ "lock solo status", N_("Solo lock"), N_("Keeps this track's solo as it is when you solo or un-solo others.") },
	{ "show/hide inspector", N_("Inspector"), N_("Shows or hides the panel with the selected track's fader and plugins.") },
	{ "comments", N_("Comments"), N_("Notes about this track. Click to write some.") },
	{ "trim", N_("Trim"), N_("Input gain: makes the signal louder or quieter before plugins and the fader.") },
	{ "midi input", N_("Input"), N_("Where this track gets its signal from. Click to choose.") },
	{ "disk", N_("Disk"), N_("You hear what is recorded on the track instead of its input.") },
	{ "in", N_("In"), N_("You hear what is coming in to the track instead of what is recorded.") },

	/* ---- start window ---- */
	{ "the audio interface is already running at this rate. change it in window > audio/midi setup.", N_("Sample rate"), N_("The sound card is already running at this rate. Change it in Window > Audio/MIDI Setup.") },

	/* ---- menu items ---- */
	{ "new...", N_("New session"), N_("Starts a new, empty session.") },
	{ "open...", N_("Open session"), N_("Opens a session you saved before.") },
	{ "recent...", N_("Recent sessions"), N_("Sessions you worked on lately.") },
	{ "close", N_("Close session"), N_("Closes this session and goes back to the start window.") },
	{ "save", N_("Save"), N_("Saves the session. Recorded audio is always written to disk as you record.") },
	{ "save as...", N_("Save as"), N_("Saves a copy of the session under a new name and carries on in the copy.") },
	{ "snapshot (& keep working on current version) ...", N_("Snapshot"), N_("Saves the session as it is now under a name, so you can come back to this version later.") },
	{ "import", N_("Import"), N_("Brings audio or MIDI files into the session.") },
	{ "export to audio file(s)...", N_("Export"), N_("Makes the finished audio file of your mix: WAV, FLAC, MP3 and others.") },
	{ "stem export...", N_("Stem export"), N_("Exports every track or bus as its own file, for mixing somewhere else.") },
	{ "add track, bus or vca...", N_("Add track"), N_("Adds audio tracks, MIDI tracks or busses, with all the options.") },
	{ "add mono track", N_("Add mono track"), N_("Adds one mono audio track at the bottom. Good for a voice, a guitar or one microphone.") },
	{ "add stereo track", N_("Add stereo track"), N_("Adds one stereo audio track at the bottom. Good for keyboards or a stereo microphone pair.") },
	{ "search...", N_("Search"), N_("Type to find any command, track, marker or plugin.") },
	{ "undo", N_("Undo"), N_("Takes back the last thing you did.") },
	{ "redo", N_("Redo"), N_("Does again what you just undid.") },
	{ "split/separate", N_("Split"), N_("Cuts the selected regions in two at the edit point.") },
	{ "duplicate", N_("Duplicate"), N_("Makes a copy right after the original.") },
	{ "quantize audio to grid", N_("Quantize audio"), N_("Tightens the timing: stretches the audio between hits so each hit lands on the grid. Drag the blue markers afterwards to adjust single hits.") },
	{ "audio quantize settings...", N_("Quantize settings"), N_("How strongly Quantize Audio pulls hits to the grid.") },
	{ "open in musescore", N_("Open in MuseScore"), N_("Shows this MIDI region as sheet music in MuseScore. Save there and the notes come back here.") },
	{ "quantize...", N_("Quantize notes"), N_("Moves MIDI notes onto the grid to tighten the timing.") },
	{ "normalize...", N_("Normalize"), N_("Makes the region as loud as it can be without distorting.") },
	{ "reverse", N_("Reverse"), N_("Makes the region play backwards.") },
	{ "loop", N_("Loop region"), N_("Sets the loop to this region and plays it round and round.") },
	{ "rename...", N_("Rename"), N_("Gives it a new name.") },
	{ "remove", N_("Remove"), N_("Takes the region off the track. The recorded file stays on disk.") },
	{ "mute", N_("Mute"), N_("Silences it without removing it.") },
	{ "lock", N_("Lock"), N_("Stops it from being moved or changed by accident.") },
	{ "strip silence...", N_("Strip silence"), N_("Cuts out the quiet parts of a region, leaving only the sound.") },
	{ "pitch shift...", N_("Pitch shift"), N_("Makes the region higher or lower without changing its length.") },
	{ "bounce (with processing)", N_("Bounce"), N_("Makes a new audio file of this region with the track's plugins applied.") },
	{ "bounce (without processing)", N_("Bounce"), N_("Makes a new audio file of this region as it is, without the track's plugins.") },
	{ "quick help", N_("Quick Help"), N_("Shows or hides this help strip.") },
	{ "show track level bars", N_("Level bars"), N_("Shows or hides the small level meter under each track name.") },
	{ "show vertical track meters", N_("Track meters"), N_("Shows or hides the tall meter at the right of each track header.") },
	{ "audio/midi setup", N_("Audio/MIDI Setup"), N_("Choose your sound card, sample rate and buffer size.") },
	{ "preferences", N_("Preferences"), N_("All of Ardour's settings.") },

	{ 0, 0, 0 }
};

const char*
QuickHelp::canvas_text (int item_type, const char*& title)
{
	switch ((ItemType) item_type) {
		case RegionItem:
		case WaveItem:
		case RegionViewName:
		case RegionViewNameHighlight:
			title = N_("Region");
			return N_("A piece of recorded audio or MIDI. Drag to move it, drag its edge to trim, right-click for more. Double-click a MIDI region to edit its notes.");
		case LeftFrameHandle:
		case RightFrameHandle:
			title = N_("Region edge");
			return N_("Drag to make the region shorter or longer. Nothing is deleted: drag back to bring it out again.");
		case StreamItem:
			title = N_("Track");
			return N_("Recordings appear here. Drag an audio file in from your file manager to import it.");
		case DropZoneItem:
			title = N_("Empty area");
			return N_("Drop audio files here to make new tracks.");
		case PlayheadCursorItem:
			title = N_("Playhead");
			return N_("Where playback is, or will start. Drag it, or click in the ruler.");
		case MarkerItem:
		case SceneMarkerItem:
			title = N_("Marker");
			return N_("A named place in the song. Drag to move, double-click to rename, right-click for more.");
		case MarkerBarItem:
			title = N_("Marker ruler");
			return N_("Click to move the playhead here. Right-click to add a marker.");
		case RangeMarkerBarItem:
			title = N_("Range ruler");
			return N_("Drag to mark a range: for looping, punch recording or exporting just that part.");
		case SectionMarkerBarItem:
			title = N_("Arrangement ruler");
			return N_("Drag to mark sections such as verse and chorus, so you can move or copy whole sections.");
		case VideoBarItem:
			title = N_("Video ruler");
			return N_("Shows the frames of an imported video.");
		case SelectionItem:
		case StartSelectionTrimItem:
		case EndSelectionTrimItem:
			title = N_("Time selection");
			return N_("The stretch of time you selected. Drag its edges to adjust, right-click to cut, loop or export it.");
		case SelectionMarkerItem:
			title = N_("Selection marker");
			return N_("The start or end of the selected time. Drag to adjust.");
		case ControlPointItem:
			title = N_("Point");
			return N_("One point on a volume or automation curve. Drag to move it; hold Shift and right-click to delete it.");
		case GainLineItem:
			title = N_("Region volume line");
			return N_("The region's own volume curve. With the Draw tool, click to add points and drag them to make parts louder or quieter.");
		case EditorAutomationLineItem:
			title = N_("Automation line");
			return N_("How a setting changes over time. Click to add a point, drag points or line segments to shape it.");
		case AutomationTrackItem:
			title = N_("Automation lane");
			return N_("Draw here with the Draw tool to change volume, pan or a plugin setting over time.");
		case MeterMarkerItem:
		case MeterBarItem:
			title = N_("Time signature");
			return N_("How many beats are in a bar, 4/4 for example. Double-click to change it; click in the ruler to add a change.");
		case TempoMarkerItem:
		case TempoBarItem:
		case TempoCurveItem:
			title = N_("Tempo");
			return N_("How fast the song is, in beats per minute. Double-click to change it; click in the ruler to add a tempo change.");
		case BBTMarkerItem:
			title = N_("Bar position marker");
			return N_("Pins a bar number to this point in time. Drag to move it.");
		case FadeInItem:
		case FadeInHandleItem:
		case FadeInTrimHandleItem:
			title = N_("Fade in");
			return N_("Drag to make the region start gradually instead of suddenly. Right-click to change its shape.");
		case FadeOutItem:
		case FadeOutHandleItem:
		case FadeOutTrimHandleItem:
			title = N_("Fade out");
			return N_("Drag to make the region die away gradually. Right-click to change its shape.");
		case StartCrossFadeItem:
		case EndCrossFadeItem:
		case CrossfadeViewItem:
			title = N_("Crossfade");
			return N_("Where two overlapping regions blend into each other. Drag to make the blend longer or shorter.");
		case NoteItem:
			title = N_("Note");
			return N_("Drag to move it in time or pitch, drag its end to change its length. Delete removes it.");
		case VelocityItem:
		case VelocityBaseItem:
			title = N_("Velocity");
			return N_("How hard each note is played. Drag a bar up or down.");
		case FeatureLineItem:
			title = N_("Hit line");
			return N_("Marks a hit found in the audio.");
		case TimecodeRulerItem:
		case MinsecRulerItem:
		case BBTRulerItem:
		case SamplesRulerItem:
			title = N_("Ruler");
			return N_("Click to move the playhead here. Right-click to choose which rulers are shown.");
		case GridZoneItem:
			title = N_("Grid");
			return N_("With the Grid tool, drag here to shift bar lines so they fit the music.");
		case ClipStartItem:
		case ClipEndItem:
			title = N_("Clip edge");
			return N_("Drag to set where the clip starts or ends.");
		default:
			break;
	}
	return 0;
}
