#!/bin/sh
# Builds and runs the tests for this fork's own pure-logic code:
# search matching, plugin kinds and ranking, the MIDI file writer/reader
# used for MuseScore, and the warp-marker rule (gtk2_ardour/logic_tests.cc).
#
# Takes a few seconds. Needs g++ and glib only, not an Ardour build.
# The tests run with the memory and undefined-behaviour checkers on.

set -e

cd "$(dirname "$0")/../gtk2_ardour"

out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

g++ -std=c++17 -g -O1 -Wall -Wextra -Wno-missing-field-initializers \
	-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer \
	-I. $(pkg-config --cflags glib-2.0) \
	logic_tests.cc text_match.cc plugin_kinds.cc simple_smf.cc warp_map.cc \
	$(pkg-config --libs glib-2.0) \
	-o "$out/logic_tests"

ASAN_OPTIONS=detect_leaks=1 "$out/logic_tests"
