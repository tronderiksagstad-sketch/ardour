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

/* The words the plugin browser shows for plugins it knows, and the colour of
 * each maker's front panels.
 *
 * To describe another plugin, add a line to `descriptions`: its name in lower
 * case, then one or two plain sentences. Anything without a line here shows
 * a sentence about its kind instead, and you can always write your own note in
 * the browser, which wins over all of this.
 */

#include "plugin_browser.h"

#include "pbd/i18n.h"

const PluginBrowser::Described PluginBrowser::descriptions[] = {
	/* ---- Ardour's own ---- */
	{ "ace eq", N_("Ardour's own EQ: four bands plus high and low shelves.") },
	{ "ace compressor", N_("Ardour's own compressor. Few knobs, easy to set.") },
	{ "ace expander", N_("Makes quiet parts quieter, to push noise down.") },
	{ "ace delay", N_("Ardour's own echo. Can follow the song's tempo.") },
	{ "ace reverb", N_("Ardour's own small reverb. Light on the computer.") },
	{ "ace high/low pass filter", N_("Cuts everything below or above a chosen frequency.") },
	{ "ace reasonable synth", N_("Ardour's own tiny synth. Always there to hear your notes.") },
	{ "ace fluid synth", N_("Plays SoundFont (.sf2) files: instant General MIDI sounds.") },
	{ "ace a/b switch", N_("Switches between two signals, to compare them.") },
	{ "ace mute", N_("Silences the track from a plugin, so the mute can be automated.") },
	{ "ace inline scope", N_("A small oscilloscope that shows the waveform.") },
	{ "ace inline spectrogram", N_("Shows which frequencies are in the sound, scrolling by.") },
	{ "ace amplifier", N_("A plain volume control.") },

	/* ---- x42 ---- */
	{ "x42-eq parametric equalizer", N_("Four-band EQ with a built-in spectrum display.") },
	{ "x42-comp dynamic compressor", N_("Gentle, musical compressor for groups and whole mixes.") },
	{ "x42-limiter peak limiter", N_("Clean limiter for the master. Set the ceiling and forget it.") },
	{ "x42 auto tune", N_("Pulls a sung note to the nearest note of the scale.") },
	{ "x42 stereo balance", N_("Balance and width for a stereo track.") },
	{ "x42 ebu r128 meter", N_("Measures loudness in LUFS, the way streaming services do.") },
	{ "setbfree tonewheel organ", N_("Tonewheel organ with drawbars and a rotary speaker.") },
	{ "avl drumkits black pearl", N_("A ready-to-play acoustic kit. No setup.") },

	/* ---- LSP ---- */
	{ "lsp parametric equalizer x16 stereo", N_("Sixteen bands, for detailed work.") },
	{ "lsp graphic equalizer x32 stereo", N_("32 fixed bands, like the sliders on a live desk.") },
	{ "lsp compressor stereo", N_("Detailed compressor with a graph of what it is doing.") },
	{ "lsp multiband compressor stereo x8", N_("Compresses up to eight frequency bands separately.") },
	{ "lsp impulse reverb stereo", N_("Plays recordings of real spaces (impulse responses).") },
	{ "lsp spectrum analyzer x1", N_("Shows which frequencies are in the sound.") },

	/* ---- Calf ---- */
	{ "calf equalizer 8 band", N_("Eight bands with a clear curve display.") },
	{ "calf vintage delay", N_("Tape-style echo with ping-pong.") },
	{ "calf multi chorus", N_("Thick chorus with several voices.") },

	/* ---- Dragonfly and friends ---- */
	{ "dragonfly hall reverb", N_("Concert-hall reverb. Lush on vocals, strings and pads.") },
	{ "dragonfly room reverb", N_("The sound of a real room, from a booth to a studio.") },
	{ "dragonfly plate reverb", N_("Bright, smooth plate reverb. A classic on vocals and snare.") },
	{ "dragonfly early reflections", N_("Only the first echoes of a room: space without a tail.") },
	{ "tdr nova", N_("Dynamic EQ: each band can also work as a compressor.") },
	{ "tdr kotelnikov", N_("Very clean compressor, made for the whole mix.") },
	{ "loudmax", N_("Two sliders: makes a mix loud without clipping.") },
	{ "tal-reverb-4", N_("Vintage 80s-style plate. Dark and diffuse.") },
	{ "tal-dub-3", N_("Dub echo that gets dirty when you push it.") },
	{ "tal-chorus-lx", N_("The chorus of the Juno-60 synth. Two buttons, instant 80s.") },
	{ "tal-noisemaker", N_("Simple, punchy analog-style synth.") },
	{ "neural amp modeler", N_("Plays captures of real guitar amps. Load one and play.") },
	{ "chowtape model", N_("The sound of a reel-to-reel tape machine: warmth, wobble, hiss.") },
	{ "wolf shaper", N_("Draw your own distortion curve.") },
	{ "graillon 2", N_("Vocal tuning and pitch effects, from subtle to robot.") },
	{ "rubber band pitch shifter", N_("Moves the pitch up or down without changing the speed.") },

	/* ---- instruments ---- */
	{ "surge xt", N_("Huge free synth with thousands of presets, from pads to basses.") },
	{ "vital", N_("Wavetable synth where you can see every movement of the sound.") },
	{ "helm", N_("Friendly synth, good for learning how synths work.") },
	{ "zynaddsubfx", N_("Deep synth with rich pads, organs and bells.") },
	{ "dexed", N_("FM synth in the style of the DX7: electric pianos and bells.") },
	{ "ob-xd", N_("Fat analog-style polysynth.") },
	{ "cardinal", N_("A modular synth: patch modules together with virtual cables.") },
	{ "pianoteq 8", N_("Modelled grand pianos that respond like the real thing.") },
	{ "mda epiano", N_("Classic electric piano, Rhodes style.") },
	{ "drumgizmo", N_("Recorded acoustic kits, each drum on its own channel.") },
	{ "geonkick", N_("Design your own kicks and percussion.") },
	{ "sitala", N_("Sixteen pads. Drag in your own samples.") },
	{ "sfizz", N_("Plays SFZ sample libraries.") },
	{ "decent sampler", N_("Plays sample libraries, many of them free.") },
	{ "u-he diva", N_("Analog synth sound at its most convincing. Heavy on the computer.") },

	{ 0, 0 }
};

/* maker (lower case, any part of it) -> light, dark and lettering colour of the front panel */
const PluginBrowser::Skin PluginBrowser::skins[] = {
	{ "ardour", 0x3a5a7c, 0x1f344c, 0xdbe7f5 },
	{ "x42", 0x33363b, 0x17191c, 0xf2b04d },
	{ "robin gareus", 0x33363b, 0x17191c, 0xf2b04d },
	{ "lsp", 0x3f5059, 0x222e34, 0xaee6ff },
	{ "linux studio", 0x3f5059, 0x222e34, 0xaee6ff },
	{ "calf", 0x5a3535, 0x2e1a1a, 0xf1d9b5 },
	{ "willis", 0x35553f, 0x1a2d20, 0xcdf3d6 },
	{ "tokyo dawn", 0x5b6068, 0x2f3237, 0xffffff },
	{ "tal ", 0xd0c9b8, 0xa39c8a, 0x26241f },
	{ "togu", 0xd0c9b8, 0xa39c8a, 0x26241f },
	{ "zl audio", 0x30365a, 0x181c33, 0xc2cbff },
	{ "mundt", 0x633266, 0x36183a, 0xf5d6ff },
	{ "airwindows", 0xd9dbde, 0xa9acb1, 0x22252a },
	{ "atkinson", 0x26282c, 0x0d0e10, 0xe6e8eb },
	{ "guitarix", 0x65532e, 0x372c11, 0xffe2a8 },
	{ "chowdhury", 0x764228, 0x402213, 0xffd9b8 },
	{ "auburn", 0x75402d, 0x3d1e13, 0xffd0b0 },
	{ "surge", 0xe39034, 0xab6315, 0x20140a },
	{ "vital", 0x2b2748, 0x141126, 0xcdb3ff },
	{ "suburban", 0x63503a, 0x372a1c, 0xa8f3d6 },
	{ "modartt", 0x222326, 0x08080a, 0xf4e9d0 },
	{ "u-he", 0x44484f, 0x202226, 0xf2f2f2 },
	{ "drumgizmo", 0x5a3030, 0x2c1616, 0xffd6d6 },
	{ "decomposer", 0xe4dfd3, 0xb8b2a4, 0x24221e },
	{ "decent", 0xe9e4da, 0xbdb7ab, 0x2b2925 },
	{ "discodsp", 0x303236, 0x131416, 0xffcc66 },
	{ 0, 0, 0, 0 }
};
