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

## Workflow

- Mockups live on the "Ardour Redesign" design canvas (claude.ai artifact P4iXDk1Sf91eiqX8BgmCu7); agree the design there before changing code.
- Then implement in the C++ code, build with `./waf`, run under Xvfb with the Dummy backend to check on screen, and push to the working branch.
- Shallow clones need a local annotated version tag (e.g. `git tag -a 9.8 -m local <base>`) for `./waf configure`; never push that tag.

## Notes

- `notes/cubase-comparison.md`: Ardour vs Cubase feature list, with the gaps worth filling.
