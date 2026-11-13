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

# Debian package names. Names a system does not have are skipped.
#  synths:   Surge XT, Dexed (FM), Helm, ZynAddSubFX
#  effects:  Dragonfly Reverb, LSP Plugins, Calf, x42
PACKAGES="
surge-xt surge-xt-lv2 surge-xt-vst3 surge-xt-clap
dexed helm helm-synth
zynaddsubfx zynaddsubfx-lv2
dragonfly-reverb-lv2 dragonfly-reverb-vst3
lsp-plugins-lv2 lsp-plugins-vst3
calf-plugins
x42-plugins
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
