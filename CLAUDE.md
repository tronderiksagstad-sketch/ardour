# Project notes for Claude

This is a personal fork of Ardour being redesigned for ease of use.
Working branch: `memlock-fix-and-theme`.

## Design philosophy (from the project owner — apply to every change)

**Simplify. Minimise the number of clicks needed.**

- Prefer one screen over several steps, and one click over two.
- Remove options that most people never need; tuck rare ones away rather than showing them by default.
- No welcome screens, splash steps or "getting started" pages that sit between the user and their work.
- The most likely action should already be selected or shown (e.g. the start window opens straight on the New session form).
- Lists of things the user picks from (recent sessions, files) open on a single click.

**Centre the main elements.**

- Lay windows and dialogs out symmetrically around the middle, like centred text in an editor.
- The main thing sits in the middle and is the largest; side sections are of equal width.
- A search bar is narrow and centred over the middle section, not stretched across the window.
- Within a section, stack things on top of each other and centre them (a title above its content, not beside it).
- Rows in a list the user scans stay left-aligned inside the row; the list itself is centred.

## Platforms

Ardour runs on Linux, Windows and macOS, and this fork must too.

- Write every addition to work on all three. Guard POSIX-only code (`sys/wait.h`, `apt`, `pkexec`, X11 screen grabs) with `PLATFORM_WINDOWS` / `__APPLE__`, and give each a sensible fallback.
- Write shortcuts as "Ctrl+Z" in text; show them through the system's own primary-key name (`Keyboard::primary_modifier_name ()`).
- The free-plugin shelf (`plugin_browser.cc`) shows each system its own plugins: Linux the ones with Linux builds (and the `apt` installer), Windows and macOS the longer list in `free_plugins_catalogue.cc`.
- Only Linux builds are tested during development. Windows and macOS builds are tried at the end.

## Workflow

- Mockups live on the "Ardour Redesign" design canvas (claude.ai artifact P4iXDk1Sf91eiqX8BgmCu7); agree the design there before changing code.
- Then implement in the C++ code, build with `./waf`, run under Xvfb with the Dummy backend to check on screen, and push to the working branch.
- After touching search matching, plugin kinds or ranking, the MuseScore MIDI code or warp markers, run `tools/logic-tests.sh` (a few seconds, no Ardour build needed). New pure logic goes in its own small file with tests in `gtk2_ardour/logic_tests.cc`.
- For a bug hunt, build a second copy with `./waf configure --address-sanitizer` in a separate git worktree and click through the features under it (`ASAN_OPTIONS=detect_leaks=0`).
- Shallow clones need a local annotated version tag (e.g. `git tag -a 9.8 -m local <base>`) for `./waf configure`; never push that tag.

## Notes

- `notes/cubase-comparison.md`: Ardour vs Cubase feature list, with the gaps worth filling.
