# Ardour 9 vs Cubase features

Compared against Steinberg's Cubase 16 feature list (steinberg.net/cubase/features).
Checked against this source tree where unsure. Written 2026-11-10.

✅ = Ardour has it · 🟡 = partly / in another form · ❌ = missing

## Highlights

| Cubase | Ardour |
|---|---|
| Pattern Sequencer | ❌ No pattern/step sequencer. Has MIDI **step entry**, and the **Cue page** (clip launching), which Cubase lacks. |
| Modulators (LFOs etc. on any parameter) | ❌ Automation and Lua scripts only. |
| VariAudio (pitch/timing editing of vocals) | ❌ Nothing built in. |
| Chord Pads | ❌ |
| Control Room | ✅ **Monitor Section** (dim, mono, cut, solo modes, solo-isolate, cue). |
| Comping | 🟡 Takes kept as playlists, **layered recording**; no take lanes. |
| Audio Warp Quantize | 🟡 **Rhythm Ferret** finds hits and splits; no stretch-to-grid across tracks. |

## General
- ✅ 32-bit float engine, high sample rates, low latency, free routing.
- ✅ Searchable preferences.
- ✅ Linux, macOS (incl. Apple silicon), Windows.
- 🟡 Surround/immersive: surround master, **ACE Vapor** renderer, ADM export.
- ❌ Ableton Link (Ableton Push control surface is supported).

## Composing
- ✅ Arranger track: **Arrangement** sections.
- ❌ Chord Track / Chord Assistant, Note Expression (per-note control, MPE), Expression Maps.

## Recording
- ✅ Punch in/out, pre-roll, count-in, loop recording.
- 🟡 Comping (see above).
- ✅ Monitoring (Monitor Section).

## MIDI
- ✅ Piano roll, incl. separate **Pianoroll** window.
- 🟡 Drum editing: percussion note view and MIDNAM note names; no drum maps.
- 🟡 Logical Editor: **Transform** dialog and **Lua scripts**.
- ✅ Quantize, legato, velocity tools, step entry.
- ❌ MIDI 2.0. ❌ Per-note probability.

## Audio editing
- ✅ Time-stretch and pitch-shift per region (Rubber Band), region gain envelopes, fades, crossfades, polarity.
- 🟡 Audio quantize (Rhythm Ferret).
- ❌ VariAudio-style pitch editing. ❌ Spectral warping.

## Mixing
- ✅ Mixer, inserts, unlimited sends, side-chain, VCAs, automation, drag-and-drop plugins.
- ✅ Loudness analysis and loudness-normalised export.
- ✅ Stem export.
- 🟡 Advanced channel strip (Mixbus has one; Ardour uses plugins).

## Instruments, sounds, loops
- 🟡 Few instruments: **ACE Reasonable Synth**, **ACE Fluid Synth** (SoundFonts), plus free LV2/VST3 plugins.
- 🟡 **Library Downloader** and Freesound search; no large bundled library.
- ❌ Instrument stacks, Drum Machine, Omnivocal.

## Effects
- 🟡 About 10 ACE effects (EQ, compressor, expander, delay, reverb, filters…); free collections (x42, LSP, Calf) cover most of Cubase's list.
- 🟡 MIDI effects: arpeggiators (simple, Barlow, Raptor) and a few scripts. No Beat Designer or Chorder.

## Other
- ❌ AI stem separation. ❌ Score editor. ❌ Collaboration tools. ❌ Quick Help.
- ✅ Video timeline and video export. ✅ Lua scripting (Cubase has none).
- 🟡 Tempo mapping tools; no automatic tempo detection from audio.

## Ardour has, Cubase doesn't
Free and open source · native Linux · clip launching (Cue page) · Lua scripting · unlimited tracks and sends in every version.

## Gaps, by effort
- **Fits Ardour's design (days to weeks each):** take-lane comping, chord track + chord pads,
  pattern/step sequencer (could build on the Cue page), multi-track audio warp quantize, instrument stacks.
- **Very large / research-level:** VariAudio-style pitch editing, AI stem separation, score editor.

## Useful building blocks already in the source
- `libs/vamp-pyin` — pYIN pitch tracker (note-by-note pitch detection, as used by pitch editors).
- `libs/vamp-plugins` — onset detection (Rhythm Ferret uses it).
- Rubber Band time-stretch, including variable stretch (`setKeyFrameMap`, used in `libs/ardour/lua_api.cc`).
