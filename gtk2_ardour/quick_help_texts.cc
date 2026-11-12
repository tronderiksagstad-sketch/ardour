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
	{ "add plugin...", N_("Add plugin"), N_("Opens the plugin browser for the selected track: search your plugins, click one to add it, Esc to close.") },
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

	/* ---- all the menus, and the Mixer, Recorder and Cue pages ---- */
	{ "snapshot (& switch to new version) ...", N_("Snapshot and switch"), N_("Saves the session as it is under a new name, and you carry on working in the new one.") },
	{ "save session copy as...", N_("Save a copy"), N_("Saves a complete copy of the session somewhere else. You stay in the current one.") },
	{ "add new track, bus, vca or foldback...", N_("Add track"), N_("Adds audio tracks, MIDI tracks or busses, with all the options.") },
	{ "import audio/midi files...", N_("Import"), N_("Brings audio or MIDI files into the session, as new tracks or onto existing ones.") },
	{ "open video...", N_("Open video"), N_("Adds a video so you can work on sound for picture.") },
	{ "remove video", N_("Remove video"), N_("Takes the video out of the session.") },
	{ "quick audio export...", N_("Quick export"), N_("Makes an audio file of the mix with the usual settings, in one step.") },
	{ "export surround master...", N_("Export surround"), N_("Exports the surround mix as a master file.") },
	{ "export to video file...", N_("Export video"), N_("Makes a new video file with your mix as its sound.") },
	{ "import pt session...", N_("Import Pro Tools session"), N_("Brings in the audio and regions of a Pro Tools session.") },
	{ "import mixer strips...", N_("Import mixer strips"), N_("Loads tracks with their plugins and settings from another session.") },
	{ "save mixer strips...", N_("Save mixer strips"), N_("Saves the selected tracks' plugins and settings to use in another session.") },
	{ "create session template...", N_("Make template"), N_("Saves this session's tracks and setup as a starting point for new sessions.") },
	{ "atmos surround master", N_("Surround master"), N_("Adds the bus used for surround (Atmos) mixing.") },
	{ "loudness assistant...", N_("Loudness"), N_("Measures how loud the mix is and can set it to a standard level, for streaming for example.") },
	{ "loudness assistant", N_("Loudness"), N_("Measures how loud the selected part is and can set it to a standard level.") },
	{ "use monitor section", N_("Monitor section"), N_("Adds listening controls (speaker volume, dim, mono) that do not affect what is exported.") },
	{ "dim", N_("Dim"), N_("Turns the speakers down a set amount, to talk for example.") },
	{ "mono", N_("Mono"), N_("Lets you hear the mix in mono, to check it still works on a single speaker.") },
	{ "edit metadata...", N_("Edit song info"), N_("Title, artist and other details written into exported files.") },
	{ "import metadata...", N_("Import song info"), N_("Copies title, artist and other details from another session.") },
	{ "bring all media into session folder", N_("Collect files"), N_("Copies every audio file the session uses into the session folder, so nothing is left elsewhere on the disk.") },
	{ "rebuild peak files", N_("Redraw waveforms"), N_("Makes the waveform pictures again. Use it if waveforms look wrong or are missing.") },
	{ "clean-up unused sources...", N_("Clean up files"), N_("Finds recordings that are no longer used anywhere and moves them to the wastebasket.") },
	{ "clean-up unused regions...", N_("Clean up regions"), N_("Removes regions that are no longer on any track from the list.") },
	{ "flush wastebasket", N_("Empty wastebasket"), N_("Deletes the cleaned-up files from the disk for good. This cannot be undone.") },
	{ "archive...", N_("Archive"), N_("Packs the whole session into one compressed file, for backup or sending.") },
	{ "quit", N_("Quit"), N_("Closes Ardour.") },
	{ "session properties", N_("Session settings"), N_("Settings that belong to this session only: timecode, file format, monitoring.") },
	{ "start/stop", N_("Play / stop"), N_("Starts playback, or stops it.") },
	{ "play selection", N_("Play selection"), N_("Plays only the selected time range or regions.") },
	{ "solo selection", N_("Solo selection"), N_("Plays the selection with only its tracks audible.") },
	{ "play w/preroll", N_("Play with pre-roll"), N_("Starts a little before the playhead so you can come in on time.") },
	{ "start/continue/stop", N_("Play / stop here"), N_("Like Play, but stopping leaves the playhead where it is.") },
	{ "play from edit point and return", N_("Play and return"), N_("Plays from the edit point and jumps back there when you stop.") },
	{ "start recording", N_("Start recording"), N_("Arms recording and starts playing in one step.") },
	{ "stop and forget capture", N_("Stop and discard"), N_("Stops and throws away what was just recorded.") },
	{ "enable record", N_("Record"), N_("Arms recording. Then press Play.") },
	{ "record w/preroll", N_("Record with pre-roll"), N_("Plays a little first, then starts recording at the playhead.") },
	{ "record w/count-in", N_("Record with count-in"), N_("Gives you a bar of clicks before recording starts.") },
	{ "use skip ranges", N_("Skip ranges"), N_("Playback jumps over ranges you have marked to be skipped.") },
	{ "forward", N_("Fast forward"), N_("Plays forward faster. Press again to go faster still.") },
	{ "rewind", N_("Rewind"), N_("Plays backwards. Press again to go faster.") },
	{ "transition to roll", N_("Play forward"), N_("Starts playing forward at normal speed.") },
	{ "transition to reverse", N_("Play backwards"), N_("Starts playing backwards at normal speed.") },
	{ "set loop from selection", N_("Loop the selection"), N_("Makes the selection the loop range.") },
	{ "set punch from selection", N_("Punch the selection"), N_("Makes the selection the punch range: recording will only happen inside it.") },
	{ "set session start/end from selection", N_("Set session length"), N_("Makes the selection the start and end of the session, which is what Export uses.") },
	{ "playhead to mouse", N_("Playhead to mouse"), N_("Moves the playhead to where the mouse pointer is.") },
	{ "playhead to active mark", N_("Playhead to marker"), N_("Moves the playhead to the selected marker.") },
	{ "center playhead", N_("Centre playhead"), N_("Scrolls so the playhead is in the middle of the window.") },
	{ "nudge playhead forward", N_("Playhead a step later"), N_("Moves the playhead a small step to the right.") },
	{ "nudge playhead backward", N_("Playhead a step earlier"), N_("Moves the playhead a small step to the left.") },
	{ "move to next transient", N_("Next hit"), N_("Moves the playhead to the next sudden sound (a drum hit, a note start).") },
	{ "move to previous transient", N_("Previous hit"), N_("Moves the playhead to the sudden sound before it.") },
	{ "playhead to next grid", N_("Next grid line"), N_("Moves the playhead to the next grid line.") },
	{ "playhead to previous grid", N_("Previous grid line"), N_("Moves the playhead to the grid line before it.") },
	{ "playhead to next region boundary", N_("Next region edge"), N_("Moves to the next place where a region starts or ends.") },
	{ "playhead to next region boundary (no track selection)", N_("Next region edge"), N_("Moves to the next place where a region starts or ends.") },
	{ "to next region boundary", N_("Next region edge"), N_("Moves to the next place where a region starts or ends.") },
	{ "playhead to previous region boundary", N_("Previous region edge"), N_("Moves to the place before, where a region starts or ends.") },
	{ "playhead to previous region boundary (no track selection)", N_("Previous region edge"), N_("Moves to the place before, where a region starts or ends.") },
	{ "to previous region boundary", N_("Previous region edge"), N_("Moves to the place before, where a region starts or ends.") },
	{ "playhead to next region sync", N_("Next sync point"), N_("Moves to the next region's sync point.") },
	{ "to next region sync", N_("Next sync point"), N_("Moves to the next region's sync point.") },
	{ "playhead to previous region sync", N_("Previous sync point"), N_("Moves to the sync point of the region before.") },
	{ "to previous region sync", N_("Previous sync point"), N_("Moves to the sync point of the region before.") },
	{ "jump to next mark", N_("Next marker"), N_("Moves the playhead to the next marker.") },
	{ "jump to previous mark", N_("Previous marker"), N_("Moves the playhead to the marker before it.") },
	{ "jump to loop start", N_("Loop start"), N_("Moves the playhead to where the loop begins.") },
	{ "jump to loop end", N_("Loop end"), N_("Moves the playhead to where the loop ends.") },
	{ "go to zero", N_("Go to zero"), N_("Moves the playhead to the very beginning of the timeline.") },
	{ "go to start", N_("Go to start"), N_("Moves the playhead to the start of the session.") },
	{ "go to end", N_("Go to end"), N_("Moves the playhead to the end of the session.") },
	{ "go to wall clock", N_("Go to time of day"), N_("Moves the playhead to the current time of day. For long recordings stamped with clock time.") },
	{ "add mark from playhead", N_("Add marker"), N_("Puts a marker where the playhead is.") },
	{ "remove mark at playhead", N_("Remove marker"), N_("Removes the marker at the playhead.") },
	{ "toggle mark at playhead", N_("Marker on/off"), N_("Adds a marker at the playhead, or removes the one that is there.") },
	{ "set session start from playhead", N_("Session starts here"), N_("Makes the playhead position the start of the session.") },
	{ "set session end from playhead", N_("Session ends here"), N_("Makes the playhead position the end of the session. Export stops here.") },
	{ "time master", N_("Time master"), N_("Makes Ardour the clock that other programs follow.") },
	{ "punch in/out", N_("Punch"), N_("Records only inside the punch range, so you can replace one phrase.") },
	{ "punch in", N_("Punch in"), N_("Recording starts by itself at the start of the punch range.") },
	{ "punch out", N_("Punch out"), N_("Recording stops by itself at the end of the punch range.") },
	{ "auto input", N_("Auto input"), N_("While stopped you hear the input; while playing you hear the recording. Best for most recording.") },
	{ "follow range", N_("Follow range"), N_("The playhead jumps to wherever you click or select with the Range tool.") },
	{ "auto play", N_("Auto play"), N_("Playback starts by itself when you move the playhead.") },
	{ "auto return", N_("Return on stop"), N_("The playhead jumps back to where you started each time you stop.") },
	{ "click", N_("Metronome"), N_("A click on every beat while you play or record.") },
	{ "use external positional sync source", N_("External sync"), N_("Ardour follows another machine's clock instead of its own.") },
	{ "panic (send midi all-notes-off)", N_("MIDI panic"), N_("Stops stuck notes: tells every instrument to go silent.") },
	{ "undo selection change", N_("Undo selection"), N_("Goes back to what was selected before.") },
	{ "redo selection change", N_("Redo selection"), N_("Goes forward again to the newer selection.") },
	{ "cut", N_("Cut"), N_("Removes what is selected and keeps it ready to paste.") },
	{ "copy", N_("Copy"), N_("Keeps a copy of what is selected, ready to paste.") },
	{ "paste", N_("Paste"), N_("Puts in what you last cut or copied, at the edit point.") },
	{ "delete", N_("Delete"), N_("Removes what is selected.") },
	{ "cut/paste range section to playhead", N_("Move section"), N_("Cuts the selected stretch of the whole song and inserts it at the playhead.") },
	{ "copy/paste range section to playhead", N_("Copy section"), N_("Copies the selected stretch of the whole song and inserts it at the playhead.") },
	{ "delete range section", N_("Delete section"), N_("Removes the selected stretch of the whole song and closes the gap.") },
	{ "select all", N_("Select all"), N_("Selects everything.") },
	{ "select all objects", N_("Select all"), N_("Selects every region.") },
	{ "select all tracks", N_("Select all tracks"), N_("Selects every track.") },
	{ "select all visible lanes", N_("Select visible lanes"), N_("Selects every track and automation lane you can see.") },
	{ "deselect all", N_("Select nothing"), N_("Clears the selection.") },
	{ "invert selection", N_("Invert selection"), N_("Selects everything that is not selected, and the other way round.") },
	{ "set range to loop range", N_("Select the loop"), N_("Makes the loop range the selection.") },
	{ "set range to punch range", N_("Select the punch range"), N_("Makes the punch range the selection.") },
	{ "set range to selected regions", N_("Select the regions' time"), N_("Selects the stretch of time the selected regions cover.") },
	{ "add range marker from selection", N_("Mark the selection"), N_("Saves the selected stretch of time as a named range.") },
	{ "select all after edit point", N_("Select everything after"), N_("Selects every region after the edit point.") },
	{ "select all before edit point", N_("Select everything before"), N_("Selects every region before the edit point.") },
	{ "select all overlapping edit range", N_("Select touching"), N_("Selects every region that touches the selected time.") },
	{ "select all inside edit range", N_("Select inside"), N_("Selects every region that lies completely inside the selected time.") },
	{ "select all in punch range", N_("Select in punch range"), N_("Selects every region inside the punch range.") },
	{ "select all in loop range", N_("Select in loop"), N_("Selects every region inside the loop.") },
	{ "move range start to previous region boundary", N_("Selection start earlier"), N_("Moves the start of the selection back to the previous region edge.") },
	{ "move range start to next region boundary", N_("Selection start later"), N_("Moves the start of the selection forward to the next region edge.") },
	{ "move range end to previous region boundary", N_("Selection end earlier"), N_("Moves the end of the selection back to the previous region edge.") },
	{ "move range end to next region boundary", N_("Selection end later"), N_("Moves the end of the selection forward to the next region edge.") },
	{ "start range", N_("Start selection"), N_("Starts a time selection at the playhead.") },
	{ "finish range", N_("Finish selection"), N_("Ends the time selection at the playhead.") },
	{ "select next track or bus", N_("Next track"), N_("Selects the track below.") },
	{ "select previous track or bus", N_("Previous track"), N_("Selects the track above.") },
	{ "crop", N_("Crop"), N_("Trims the regions so only the selected time is left.") },
	{ "separate under", N_("Cut underneath"), N_("Cuts the regions lying under the selected one at its edges.") },
	{ "separate using loop range", N_("Split at loop"), N_("Splits regions at the start and end of the loop.") },
	{ "separate using punch range", N_("Split at punch range"), N_("Splits regions at the start and end of the punch range.") },
	{ "consolidate range", N_("Join into one"), N_("Makes one new region from everything in the selected time.") },
	{ "consolidate range (with processing)", N_("Join into one, with effects"), N_("Makes one new region from the selected time, with the track's plugins applied.") },
	{ "combine", N_("Combine"), N_("Joins the selected regions into a single region. Uncombine takes it apart again.") },
	{ "uncombine", N_("Uncombine"), N_("Takes a combined region apart into its original regions.") },
	{ "align start", N_("Align starts"), N_("Moves the selected regions so they start at the edit point.") },
	{ "align start relative", N_("Align starts, keep spacing"), N_("Moves the selected regions to the edit point, keeping their distance to each other.") },
	{ "align end", N_("Align ends"), N_("Moves the selected regions so they end at the edit point.") },
	{ "align end relative", N_("Align ends, keep spacing"), N_("Moves the selected regions so the last ends at the edit point, keeping their spacing.") },
	{ "align sync", N_("Align sync points"), N_("Moves the selected regions so their sync points are at the edit point.") },
	{ "align sync relative", N_("Align sync, keep spacing"), N_("Moves the selected regions by their sync points, keeping their spacing.") },
	{ "fade range selection", N_("Fade the selection"), N_("Adds a fade in or out over the selected time.") },
	{ "set fade in length", N_("Fade in to here"), N_("Makes the region's fade in end at the edit point.") },
	{ "fade in", N_("Fade in on/off"), N_("Switches the region's fade in on or off.") },
	{ "set fade out length", N_("Fade out from here"), N_("Makes the region's fade out start at the edit point.") },
	{ "fade out", N_("Fade out on/off"), N_("Switches the region's fade out on or off.") },
	{ "fades", N_("Fades on/off"), N_("Switches the region's fade in and fade out on or off.") },
	{ "spectral analysis", N_("Spectrum"), N_("Shows which frequencies the selection contains.") },
	{ "spectral analysis...", N_("Spectrum"), N_("Shows which frequencies the region contains.") },
	{ "loudness analysis", N_("Loudness"), N_("Measures how loud the selection is.") },
	{ "loudness analysis...", N_("Loudness"), N_("Measures how loud the region is.") },
	{ "tag last capture", N_("Tag last take"), N_("Gives the last recording a label so you can find it again.") },
	{ "remove last capture", N_("Delete last take"), N_("Throws away the last recording, files and all.") },
	{ "change edit point", N_("Next edit point"), N_("Switches where cut, paste and split happen: mouse or playhead.") },
	{ "change edit point including marker", N_("Next edit point"), N_("Switches where cut, paste and split happen: mouse, playhead or marker.") },
	{ "next marker click mode", N_("Marker click mode"), N_("Switches what clicking a marker does.") },
	{ "marker click only selects", N_("Click selects marker"), N_("Clicking a marker only selects it.") },
	{ "locate to marker on click", N_("Click jumps to marker"), N_("Clicking a marker moves the playhead there.") },
	{ "locate to marker when transport is not rolling ", N_("Click jumps when stopped"), N_("Clicking a marker moves the playhead there, but only while stopped.") },
	{ "set tempo from region = bar", N_("Tempo from region"), N_("Sets the tempo so the selected region is exactly one bar long.") },
	{ "set tempo from edit range = bar", N_("Tempo from selection"), N_("Sets the tempo so the selected time is exactly one bar long.") },
	{ "smart mode", N_("Smart mode"), N_("Makes the Grab tool also select time ranges when you drag in the top half of a track.") },
	{ "show automation lane on touch", N_("Show lane on touch"), N_("Opens a setting's automation lane when you move its control.") },
	{ "insert time", N_("Insert time"), N_("Pushes everything later to make an empty gap of the length you choose.") },
	{ "remove time", N_("Remove time"), N_("Deletes a stretch of time and pulls everything after it earlier.") },
	{ "remove gaps", N_("Remove gaps"), N_("Closes the silences between regions on the track.") },
	{ "insert region from source list", N_("Insert from list"), N_("Puts the region selected in the list onto the track at the edit point.") },
	{ "play selected regions", N_("Play regions"), N_("Plays just the selected regions.") },
	{ "tag selected regions", N_("Tag regions"), N_("Gives the selected regions a label so you can find them again.") },
	{ "group selected regions", N_("Group regions"), N_("Ties the selected regions together so they move and are edited as one.") },
	{ "ungroup selected regions", N_("Ungroup regions"), N_("Lets grouped regions be moved separately again.") },
	{ "edit in separate window...", N_("Edit in own window"), N_("Opens the region's notes in a larger piano-roll window.") },
	{ "split at percussion onsets", N_("Split at hits"), N_("Cuts the region at every drum hit.") },
	{ "make mono regions", N_("Make mono regions"), N_("Makes a separate mono region from each channel.") },
	{ "close gaps", N_("Close gaps"), N_("Stretches region edges so there is no silence between them.") },
	{ "place transient", N_("Mark a hit"), N_("Marks a hit in the region at the edit point.") },
	{ "rhythm ferret...", N_("Rhythm Ferret"), N_("Finds the hits in a region and can split it there.") },
	{ "raise to top", N_("Bring to top"), N_("Puts the region above all others it overlaps, so it is the one you hear.") },
	{ "raise", N_("Raise"), N_("Moves the region one layer up among overlapping regions.") },
	{ "lower", N_("Lower"), N_("Moves the region one layer down among overlapping regions.") },
	{ "lower to bottom", N_("Send to bottom"), N_("Puts the region underneath all others it overlaps.") },
	{ "transpose...", N_("Transpose"), N_("Moves all the notes up or down by a number of semitones.") },
	{ "insert patch change...", N_("Change sound"), N_("Adds a point where the instrument switches to another preset.") },
	{ "legatize", N_("Join notes"), N_("Makes each note last until the next one starts.") },
	{ "remove overlap", N_("Remove overlaps"), N_("Shortens notes so none of the same pitch overlap.") },
	{ "transform...", N_("Transform"), N_("Changes many notes at once by rules you set, such as making every velocity louder.") },
	{ "unlink all selected regions", N_("Unlink copies"), N_("Makes the selected copies independent, so editing one no longer changes the others.") },
	{ "unlink from unselected", N_("Unlink from others"), N_("Makes the selected copies independent of the copies that are not selected.") },
	{ "deinterlace into layers", N_("Notes to layers"), N_("Splits the region's MIDI channels into separate layers.") },
	{ "list editor...", N_("Note list"), N_("Shows the region's notes as a list of numbers you can edit.") },
	{ "opaque", N_("Solid"), N_("When on, this region hides whatever lies under it. When off, you hear both.") },
	{ "boost gain", N_("Louder"), N_("Makes the region a little louder.") },
	{ "cut gain", N_("Quieter"), N_("Makes the region a little quieter.") },
	{ "reset gain", N_("Reset volume"), N_("Puts the region back to its original loudness.") },
	{ "reset envelope", N_("Reset volume curve"), N_("Removes the volume curve you drew on the region.") },
	{ "invert polarity", N_("Flip phase"), N_("Flips this region's waveform upside down.") },
	{ "envelope active", N_("Volume curve on/off"), N_("Switches the region's own volume curve on or off.") },
	{ "move to original position", N_("Back to where recorded"), N_("Moves the region to the time it was recorded at.") },
	{ "snap position to grid", N_("Snap to grid"), N_("Moves the region to the nearest grid line.") },
	{ "lock (toggle)", N_("Lock"), N_("Stops the region from being moved or changed by accident.") },
	{ "lock to video", N_("Lock to video"), N_("Keeps the region at the same video frame if the video is moved.") },
	{ "set sync position", N_("Set sync point"), N_("Marks a point inside the region (a downbeat, say) that snaps to the grid instead of its start.") },
	{ "remove sync", N_("Remove sync point"), N_("The region snaps by its start again.") },
	{ "nudge later", N_("Nudge later"), N_("Moves what is selected a small step to the right.") },
	{ "nudge earlier", N_("Nudge earlier"), N_("Moves what is selected a small step to the left.") },
	{ "nudge later by capture offset", N_("Nudge later by latency"), N_("Moves the region later by the recording delay of your sound card.") },
	{ "nudge earlier by capture offset", N_("Nudge earlier by latency"), N_("Moves the region earlier by the recording delay of your sound card.") },
	{ "sequence regions", N_("Line up end to end"), N_("Puts the selected regions one after the other with no gaps.") },
	{ "add region cue marker", N_("Add cue marker"), N_("Puts a marker inside the region at the edit point.") },
	{ "clear region cue markers", N_("Clear cue markers"), N_("Removes the markers inside the region.") },
	{ "convert region cue markers to cd markers", N_("Cues to CD markers"), N_("Turns the markers inside the region into CD track markers.") },
	{ "convert region cue markers to location markers", N_("Cues to markers"), N_("Turns the markers inside the region into ordinary markers on the ruler.") },
	{ "trim start at edit point", N_("Trim start"), N_("Makes the region start at the edit point.") },
	{ "trim end at edit point", N_("Trim end"), N_("Makes the region end at the edit point.") },
	{ "trim to loop", N_("Trim to loop"), N_("Trims the region to fit the loop range.") },
	{ "trim to punch", N_("Trim to punch range"), N_("Trims the region to fit the punch range.") },
	{ "trim to previous", N_("Extend to previous"), N_("Stretches the region's start back to the end of the region before it.") },
	{ "trim to next", N_("Extend to next"), N_("Stretches the region's end forward to the start of the next region.") },
	{ "set loop range", N_("Loop these regions"), N_("Sets the loop to cover the selected regions.") },
	{ "set punch", N_("Punch these regions"), N_("Sets the punch range to cover the selected regions.") },
	{ "add single range marker", N_("Mark as one range"), N_("Saves the time the selected regions cover as one named range.") },
	{ "add range marker per region", N_("Mark each region"), N_("Saves a named range for each selected region.") },
	{ "set range selection", N_("Select their time"), N_("Selects the stretch of time the selected regions cover.") },
	{ "multi-duplicate...", N_("Duplicate many"), N_("Makes several copies in a row.") },
	{ "fill track", N_("Fill track"), N_("Repeats the region to the end of the session.") },
	{ "export...", N_("Export region"), N_("Saves this region as an audio or MIDI file.") },
	{ "find & display track/bus...", N_("Find track"), N_("Type a name to jump to a track.") },
	{ "duplicate tracks/busses...", N_("Duplicate track"), N_("Makes a copy of the selected tracks, with or without their recordings.") },
	{ "toggle record enable", N_("Record-enable"), N_("Switches the selected tracks' record button on or off.") },
	{ "toggle solo", N_("Solo"), N_("Switches solo on or off for the selected tracks.") },
	{ "toggle mute", N_("Mute"), N_("Switches mute on or off for the selected tracks.") },
	{ "show playlist selector", N_("Choose take"), N_("Shows the track's playlists (takes) so you can switch between them.") },
	{ "new playlist for all tracks", N_("New take, all tracks"), N_("Gives every track a fresh empty playlist. The old takes are kept.") },
	{ "new playlist for rec-armed tracks", N_("New take, armed tracks"), N_("Gives every record-enabled track a fresh empty playlist. The old takes are kept.") },
	{ "new playlist for selected tracks", N_("New take, selected tracks"), N_("Gives the selected tracks a fresh empty playlist. The old takes are kept.") },
	{ "copy playlist for all tracks", N_("Copy take, all tracks"), N_("Makes a copy of every track's current take to work on, keeping the original.") },
	{ "copy playlist for rec-armed tracks", N_("Copy take, armed tracks"), N_("Makes a copy of the current take on record-enabled tracks, keeping the original.") },
	{ "copy playlist for selected tracks", N_("Copy take, selected tracks"), N_("Makes a copy of the current take on the selected tracks, keeping the original.") },
	{ "fit selection (vertical)", N_("Fit tracks"), N_("Sets track height so the selected tracks fill the window.") },
	{ "largest", N_("Largest"), N_("Makes the selected tracks as tall as they go.") },
	{ "larger", N_("Larger"), N_("Makes the selected tracks very tall.") },
	{ "large", N_("Large"), N_("Makes the selected tracks tall.") },
	{ "normal", N_("Normal"), N_("Sets the selected tracks to the usual height.") },
	{ "small", N_("Small"), N_("Makes the selected tracks short.") },
	{ "toggle active", N_("Active on/off"), N_("Switches the track off completely to save computer power, or on again.") },
	{ "remove selected track(s)", N_("Remove track"), N_("Deletes the selected tracks. The recorded files stay on disk.") },
	{ "stacked layer display", N_("Stacked"), N_("Shows overlapping regions in rows under each other.") },
	{ "overlaid layer display", N_("Overlaid"), N_("Shows overlapping regions on top of each other.") },
	{ "maximise editor space", N_("Full screen"), N_("Hides the window frame and toolbars to give the tracks more room.") },
	{ "maximise mixer space", N_("Full screen mixer"), N_("Hides the window frame to give the mixer more room.") },
	{ "focus on clock", N_("Type a time"), N_("Puts the cursor in the clock so you can type where to go.") },
	{ "zoom to extents", N_("Zoom to all"), N_("Zooms so every region is visible.") },
	{ "zoom to selection", N_("Zoom to selection"), N_("Zooms so the selection fills the window.") },
	{ "zoom to selection (horizontal)", N_("Zoom to selection width"), N_("Zooms in time only, so the selection fills the window's width.") },
	{ "toggle zoom state", N_("Previous zoom"), N_("Switches back to the zoom you had before.") },
	{ "expand track height", N_("Taller tracks"), N_("Makes the selected tracks taller.") },
	{ "shrink track height", N_("Shorter tracks"), N_("Makes the selected tracks shorter.") },
	{ "scroll tracks down", N_("Scroll down"), N_("Scrolls the tracks down.") },
	{ "scroll tracks up", N_("Scroll up"), N_("Scrolls the tracks up.") },
	{ "scroll forward", N_("Scroll later"), N_("Scrolls the timeline to the right.") },
	{ "scroll backward", N_("Scroll earlier"), N_("Scrolls the timeline to the left.") },
	{ "show editor mixer", N_("Inspector"), N_("Shows or hides the panel with the selected track's fader and plugins.") },
	{ "show editor list", N_("Lists"), N_("Shows or hides the side panel with lists of regions, tracks and markers.") },
	{ "show editor properties box", N_("Properties"), N_("Shows or hides the box at the bottom with details of what is selected.") },
	{ "show summary", N_("Overview"), N_("Shows or hides the miniature of the whole session at the bottom.") },
	{ "show vertical summary", N_("Vertical overview"), N_("Shows or hides the miniature of all tracks at the side.") },
	{ "show group tabs", N_("Group tabs"), N_("Shows or hides the coloured tabs for track groups.") },
	{ "show marker lines", N_("Marker lines"), N_("Shows or hides the vertical lines drawn down from each marker.") },
	{ "cue page: show sidebar list", N_("Cue page lists"), N_("Shows or hides the side panel on the Cue page.") },
	{ "cue page: show properties bottom", N_("Cue page properties"), N_("Shows or hides the clip settings at the bottom of the Cue page.") },
	{ "mixer: show sidebar list", N_("Mixer lists"), N_("Shows or hides the mixer's side panel with favourite plugins and groups.") },
	{ "mixer: show properties bottom", N_("Mixer properties"), N_("Shows or hides the box at the bottom of the mixer.") },
	{ "mixer: show mixbusses", N_("Show busses"), N_("Shows or hides the bus strips in the mixer.") },
	{ "mixer: show vcas", N_("Show VCAs"), N_("Shows or hides the VCA master faders in the mixer.") },
	{ "mixer: show monitor section", N_("Show monitor section"), N_("Shows or hides the listening controls in the mixer.") },
	{ "mixer: show foldback strip", N_("Show foldback"), N_("Shows or hides the strip for musicians' headphone mixes.") },
	{ "show editor", N_("Editor"), N_("Goes to the Editor: the timeline with your tracks and recordings.") },
	{ "show mixer", N_("Mixer"), N_("Goes to the Mixer: every track's fader, pan and plugins side by side.") },
	{ "show recorder", N_("Recorder"), N_("Goes to the Recorder: a simple page for arming tracks and watching input levels.") },
	{ "show cues", N_("Cues"), N_("Goes to the Cue page: a grid of clips and loops you start by clicking.") },
	{ "show", N_("Show"), N_("Brings this page to the front.") },
	{ "hide", N_("Hide"), N_("Hides this page.") },
	{ "attach", N_("Attach"), N_("Puts this page back as a tab in the main window.") },
	{ "detach", N_("Detach"), N_("Opens this page in its own window, for a second screen for example.") },
	{ "meterbridge", N_("Meterbridge"), N_("A window with big level meters for every track.") },
	{ "video monitor", N_("Video monitor"), N_("Shows the video in its own window.") },
	{ "templates", N_("Templates"), N_("Manage your saved session and track templates.") },
	{ "midi tracer", N_("MIDI tracer"), N_("Shows every MIDI message coming in or going out. Useful when a keyboard does nothing.") },
	{ "log", N_("Log"), N_("Shows messages, warnings and errors from Ardour.") },
	{ "website", N_("Website"), N_("Opens the Ardour website.") },
	{ "tutorial", N_("Tutorial"), N_("Opens the online beginner's tutorial.") },
	{ "reference", N_("Manual"), N_("Opens the online manual.") },
	{ "user forums", N_("Forum"), N_("Opens the Ardour user forum, where you can ask questions.") },
	{ "chat", N_("Chat"), N_("Opens the Ardour chat room.") },
	{ "how to report a bug", N_("How to report a bug"), N_("Explains how to describe a problem so the developers can fix it.") },
	{ "report a bug", N_("Report a bug"), N_("Opens the bug tracker.") },
	{ "development", N_("Development"), N_("Opens the page for people who want to work on Ardour's code.") },
	{ "about", N_("About"), N_("Shows the version of Ardour and who made it.") },
	{ "keyboard shortcuts", N_("Keyboard shortcuts"), N_("Lists every shortcut key and lets you change them.") },
	{ "plugin manager", N_("Plugin manager"), N_("Lists all installed plugins and lets you hide the ones you never use.") },
	{ "audio connections", N_("Audio connections"), N_("A grid showing which inputs and outputs are connected to which tracks.") },
	{ "midi connections", N_("MIDI connections"), N_("A grid showing which MIDI devices are connected to which tracks.") },
	{ "locations", N_("Markers and ranges"), N_("A window listing every marker and range, where you can rename and move them.") },
	{ "big clock", N_("Big clock"), N_("A large clock you can read from across the room.") },
	{ "tracks and busses", N_("Tracks and busses"), N_("A list of all tracks where you can show, hide and set many at once.") },
	{ "bundle manager", N_("Bundles"), N_("Name groups of inputs or outputs, so you can connect them in one go.") },
	{ "plugin dsp load", N_("Plugin load"), N_("Shows how much computer power each plugin uses.") },
	{ "scripting", N_("Scripting"), N_("A window for writing and running Lua scripts.") },
	{ "new plugin", N_("Add plugin"), N_("Adds an effect or instrument to this track.") },
	{ "new insert", N_("Add insert"), N_("Sends the signal out to outboard gear and back in at this point.") },
	{ "new external send ...", N_("Add external send"), N_("Sends a copy of the signal out of Ardour, to another program or output.") },
	{ "new aux send ...", N_("Add send"), N_("Sends a copy of the signal to a bus, for a shared reverb for example.") },
	{ "new foldback send ...", N_("Add headphone send"), N_("Sends a copy of the signal to a musician's headphone mix.") },
	{ "remove foldback send ...", N_("Remove headphone send"), N_("Removes this track from a headphone mix.") },
	{ "clear (all)", N_("Remove all plugins"), N_("Removes every plugin and send from this track.") },
	{ "clear (pre-fader)", N_("Remove pre-fader"), N_("Removes the plugins that come before the fader.") },
	{ "clear (post-fader)", N_("Remove post-fader"), N_("Removes the plugins that come after the fader.") },
	{ "activate all", N_("All on"), N_("Switches every plugin on this track on.") },
	{ "deactivate all", N_("All off"), N_("Switches every plugin on this track off (bypass).") },
	{ "a/b plugins", N_("Compare with/without"), N_("Switches all plugins off, and back on again, to compare.") },
	{ "edit...", N_("Open plugin"), N_("Opens the plugin's own window.") },
	{ "edit with generic controls...", N_("Open with plain controls"), N_("Opens the plugin with simple sliders instead of its own window.") },
	{ "inline controls", N_("Controls in the strip"), N_("Shows a few of the plugin's controls right in the mixer strip.") },
	{ "presets", N_("Presets"), N_("The plugin's saved settings.") },
	{ "rename", N_("Rename"), N_("Gives it a new name.") },
	{ "pin connections...", N_("Pin connections"), N_("Shows how the plugin's inputs and outputs are wired. Here you can feed two instruments from one track.") },
	{ "send options", N_("Send options"), N_("Settings for this send.") },
	{ "pre-fader", N_("Before the fader"), N_("The meter or send takes the signal before the fader, so the fader does not affect it.") },
	{ "post-fader", N_("After the fader"), N_("The meter or send takes the signal after the fader, so it follows the fader.") },
	{ "custom", N_("Custom"), N_("Lets you place it anywhere among the plugins.") },
	{ "audition", N_("Listen"), N_("Plays the selected item on its own.") },
	{ "import to source list...", N_("Import to list"), N_("Brings files into the session without putting them on a track yet.") },
	{ "remove unused", N_("Remove unused"), N_("Removes items from the list that are not used on any track.") },
	{ "add a plugin to the effect list", N_("Add"), N_("Adds the selected plugin to the list on the right.") },
	{ "remove a plugin from the effect list", N_("Remove"), N_("Takes the selected plugin off the list.") },
	{ "click to recall this mixer scene", N_("Mixer scene"), N_("A saved snapshot of all faders and settings. Click to go back to it.") },
	{ "right-click to add/remove/edit", N_("Plugins"), N_("The effects on this track, top to bottom. Double-click the empty space to add one, double-click a plugin to open it.") },
	{ "hide this vca strip", N_("Hide"), N_("Hides this VCA fader from the mixer.") },
	{ "hide foldback strip", N_("Hide"), N_("Hides the headphone-mix strip.") },
	{ "mute assigned channels", N_("Mute"), N_("Silences every track assigned to this VCA.") },
	{ "solo assigned channels", N_("Solo"), N_("Solos every track assigned to this VCA.") },
	{ "click to show assigned channels only", N_("Show its tracks"), N_("Shows only the tracks this VCA controls.") },
	{ "click to show slaves only", N_("Show its tracks"), N_("Shows only the tracks this VCA controls.") },
	{ "click to show normal mixer", N_("Show all"), N_("Shows every strip again.") },
	{ "reset peak", N_("Reset peak"), N_("Clears the remembered loudest level.") },
	{ "exclusive solo means that only 1 solo is active at a time", N_("Exclusive solo"), N_("Soloing one track un-solos the others.") },
	{ "if enabled, solo will override mute", N_("Solo beats mute"), N_("A soloed track is heard even if it is muted.") },
	{ "gain increase for soloed signals (0db is normal)", N_("Solo boost"), N_("Makes soloed tracks louder than normal.") },
	{ "gain reduction non-soloed signals", N_("Solo cut"), N_("How much quieter the other tracks get when something is soloed. Fully down silences them.") },
	{ "gain reduction to use when dimming monitor outputs", N_("Dim level"), N_("How much quieter the speakers get when Dim is on.") },
	{ "solo controls affect solo-in-place", N_("Solo in place"), N_("Solo silences the other tracks in the mix itself.") },
	{ "solo controls toggle after-fader-listen", N_("Listen after fader"), N_("Solo buttons let you hear a track on its own, after its fader, without changing the mix.") },
	{ "solo controls toggle pre-fader-listen", N_("Listen before fader"), N_("Solo buttons let you hear a track on its own, before its fader, without changing the mix.") },
	{ "allow one to add monitor effect processors", N_("Monitor plugins"), N_("Lets you put plugins on what you hear, room correction for example. They do not affect exports.") },
	{ "add a track for this input port", N_("Add track"), N_("Makes a new track that records from this input.") },
	{ "auto-input: when enabled, track input monitoring automatically follows transport state", N_("Auto input"), N_("While stopped you hear the input; while playing you hear the recording.") },
	{ "track input monitoring automatically follows transport state", N_("Auto input"), N_("While stopped you hear the input; while playing you hear the recording.") },
	{ "create a new playlist for all rec-armed tracks", N_("New take"), N_("Gives every record-enabled track a fresh empty take. The old ones are kept.") },
	{ "create a new playlist for all tracks and switch to it.", N_("New take, all tracks"), N_("Gives every track a fresh empty take. The old ones are kept.") },
	{ "delete the region and the audio files of the last recording take", N_("Delete last take"), N_("Throws away the last recording, files and all.") },
	{ "disable recording of all tracks", N_("Disarm all"), N_("Switches every track's record button off.") },
	{ "record enable all tracks", N_("Arm all"), N_("Switches every track's record button on.") },
	{ "duration: length of the most recent (or current) recording take", N_("Take length"), N_("How long the last (or current) recording is.") },
	{ "force all tracks to monitor disk playback, unless they are explicitly set to input", N_("Hear recordings"), N_("Every track plays what is recorded instead of its input.") },
	{ "force all tracks to monitor input, unless they are explicitly set to monitor disk", N_("Hear inputs"), N_("Every track lets you hear its input.") },
	{ "remaining time:  recording time available on the current disk with currently armed tracks", N_("Time left"), N_("How long you can still record before the disk is full.") },
	{ "reset peak-hold indicator of all input meters", N_("Reset peaks"), N_("Clears the remembered loudest level on every input meter.") },
	{ "solo/listen to this input", N_("Listen"), N_("Lets you hear this input on its own.") },
	{ "this port is not feeding any tracks", N_("Unused input"), N_("No track records from this input yet.") },
	{ "x-runs: soundcard buffer under- or over-run occurrences in the last recording take", N_("Dropouts"), N_("How many times the computer could not keep up during the last take. More than 0 can mean clicks: use a bigger buffer.") },
	{ "5 second history waveform", N_("Input waveform"), N_("The last five seconds of what came in on this input.") },
	{ "display most recently received midi messages", N_("MIDI monitor"), N_("Shows the last MIDI messages that came in.") },
	{ "highlight incoming midi data per midi channel", N_("MIDI channels"), N_("Lights up for each MIDI channel that receives data.") },
	{ "audition volume", N_("Preview volume"), N_("How loud clips are when you preview them.") },
	{ "audition selected clip", N_("Preview"), N_("Plays the selected clip so you can hear it before using it.") },
	{ "stop the audition", N_("Stop preview"), N_("Stops the clip you are previewing.") },
	{ "click to select a clip folder and edit your available clip folders", N_("Clip folder"), N_("Choose which folder of clips and loops to browse.") },
	{ "open clip library folder", N_("Open folder"), N_("Opens the folder with your clips in the file manager.") },
	{ "refresh clip list", N_("Refresh"), N_("Looks in the folder again for new clips.") },
	{ "select the synth used for auditioning", N_("Preview instrument"), N_("The instrument used to play MIDI clips when you preview them.") },
	{ "show the gui for the auditioner synth", N_("Preview instrument window"), N_("Opens the preview instrument's window.") },
	{ "adjust audio gain (or midi velocity) for this slot", N_("Clip volume"), N_("How loud this clip plays.") },
	{ "allow this slot to send midi patches to a synth", N_("Send presets"), N_("Lets this clip switch the instrument to another preset when it starts.") },
	{ "click to use the follow length instead of the clip length", N_("Follow length"), N_("Use the length set here, instead of the clip's own length, before the follow action happens.") },
	{ "when enabled, the follow length is used instead of clip length", N_("Follow length"), N_("Use the length set here, instead of the clip's own length, before the follow action happens.") },
	{ "clip will wait until the timeline passes this boundary before it triggers", N_("Start on"), N_("The clip waits for the next bar (or beat) before it starts, so it comes in on time.") },
	{ "double-click to rename this clip", N_("Clip name"), N_("Double-click to rename the clip.") },
	{ "if selected, this clip can 'take over' playback in the playback position of the prior clip", N_("Carry on from"), N_("The clip starts at the position the previous clip had reached, instead of from its beginning.") },
	{ "if selected, this slot will not respond to cue events (either starting or stopping)", N_("Ignore cues"), N_("This clip is not started or stopped when you launch a whole row.") },
	{ "load a new file into this slot", N_("Load"), N_("Puts another audio or MIDI file in this slot.") },
	{ "midi velocity sensitivity for clip's volume", N_("Velocity sense"), N_("How much the clip's volume depends on how hard you hit the pad or key that starts it.") },
	{ "number of times to repeat this clip's follow length before triggering the follow-action", N_("Repeats"), N_("How many times the clip plays before the follow action happens.") },
	{ "pick a color for this clip", N_("Clip colour"), N_("Choose the clip's colour.") },
	{ "probability for the left vs right follow-action to be activated", N_("Chance"), N_("How likely the left or the right follow action is chosen each time.") },
	{ "select an action that will occur when this clip ends", N_("Follow action"), N_("What happens when the clip ends: stop, play again, or go to another clip.") },
	{ "set the behavior of this clip's launch button", N_("Launch style"), N_("What a click does: play once, play while held, or switch on and off.") },
	{ "view and edit the programs (patches) that this midi clip will send to a synth", N_("Presets"), N_("Which instrument presets this clip selects when it starts.") },
	{ "<b>if enabled</b>, the clip will be stretched to match the current timeline tempo", N_("Stretch"), N_("When on, the clip is stretched to play at the session's tempo.") },
	{ "if enabled, the clip will be stretched to match the current timeline tempo", N_("Stretch"), N_("When on, the clip is stretched to play at the session's tempo.") },
	{ "click to double the tempo for the clip. this will result in it playing slower when stretched on the timeline", N_("Double tempo"), N_("Tells Ardour the clip's tempo is twice as fast. Use it if the clip plays at double speed.") },
	{ "click to halve the tempo for the clip. this will result in it playing faster when stretched on the timeline", N_("Halve tempo"), N_("Tells Ardour the clip's tempo is half as fast. Use it if the clip plays at half speed.") },
	{ "clip tempo, used for stretching", N_("Clip tempo"), N_("The tempo the clip was made at. Ardour uses it to stretch the clip to the session's tempo.") },
	{ "length of the clip, in beats. changing this will change the tempo", N_("Clip length"), N_("How many beats long the clip is.") },
	{ "select crisp for drum loops, smooth for sustained notes, or mixed if the clip has both", N_("Stretch type"), N_("Crisp suits drums, Smooth suits held notes, Mixed suits clips with both.") },

	{ "left-click to show the editor window", N_("Editor"), N_("The timeline with your tracks and recordings. Right-click to open it in its own window.") },
	{ "left-click to show the mixer window", N_("Mixer"), N_("Every track's fader, pan and plugins side by side. Right-click to open it in its own window.") },
	{ "left-click to show the recorder window", N_("Recorder"), N_("A simple page for arming tracks and watching input levels. Right-click to open it in its own window.") },
	{ "left-click to show the cues window", N_("Cues"), N_("A grid of clips and loops you start by clicking. Right-click to open it in its own window.") },
	{ "left-click to show the preferences window", N_("Preferences"), N_("All of Ardour's settings.") },

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
