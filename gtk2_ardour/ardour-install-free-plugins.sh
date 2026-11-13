#!/bin/sh
# Install the free, open-source plugins this version recommends, from the
# system's own package list (so they are kept up to date, and their
# licences are looked after by the distribution).
#
# Meant to be run through pkexec by the plugin browser's "Get free plugins"
# button. Only packages that the system really offers are installed; the
# others are named at the end. Nothing is downloaded from anywhere else.

if [ "$(id -u)" -ne 0 ] || [ -z "$PKEXEC_UID" ]; then
	echo "This helper must be run through pkexec." >&2
	exit 2
fi

# Debian package names (all checked against Debian 13 "trixie"). Names a
# system does not have are skipped. Surge XT, Dexed and Helm are not in
# Debian 13 under any name; the plugin browser links to their own pages.
#  synths:   ZynAddSubFX, Yoshimi, amSynth, DrumGizmo (drums)
#  effects:  Dragonfly Reverb, LSP Plugins, Calf, x42, Zam Plugins, DISTRHO, MDA,
#            EQ10Q, CAPS, SWH, Invada, abGate, Guitarix, Rubber Band; organ: setBfree
PACKAGES="
surge-xt surge-xt-lv2 surge-xt-vst3 surge-xt-clap
zynaddsubfx zynaddsubfx-lv2 yoshimi amsynth drumgizmo
dragonfly-reverb-lv2 dragonfly-reverb-vst3 dragonfly-reverb-clap
lsp-plugins-lv2 lsp-plugins-vst3
calf-plugins
x42-plugins
zam-plugins
dpf-plugins-lv2 dpf-plugins-vst3 dpf-plugins-clap
mda-lv2 eq10q fomp caps abgate rubberband-lv2
setbfree guitarix-lv2 swh-lv2 invada-studio-plugins-lv2
avldrums.lv2 padthv1 samplv1 synthv1 drumkv1
lsp-plugins-clap
"

export DEBIAN_FRONTEND=noninteractive
apt-get update -qq >/dev/null 2>&1

HAVE=""
MISSING=""
for p in $PACKAGES; do
	if apt-cache policy "$p" 2>/dev/null | grep -q "Candidate: [^(]"; then
		HAVE="$HAVE $p"
	else
		MISSING="$MISSING $p"
	fi
done

if [ -z "$HAVE" ]; then
	echo "None of the plugin packages are in this system's package list."
	exit 4
fi

# One at a time, so that a package with a problem does not stop the others.
DONE=""
FAILED=""
for p in $HAVE; do
	if apt-get install -y --no-install-recommends "$p" >/dev/null 2>&1; then
		DONE="$DONE $p"
	else
		FAILED="$FAILED $p"
	fi
done

if [ -z "$DONE" ]; then
	echo "The package manager could not install them."
	exit 5
fi

echo "Installed:$DONE"
[ -n "$FAILED" ] && echo "Could not be installed:$FAILED"
[ -n "$MISSING" ] && echo "Not offered by this system:$MISSING"
exit 0
