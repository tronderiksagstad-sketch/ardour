#!/bin/sh
# Grant the "audio" group permission to lock memory and use realtime
# scheduling, and add the calling user to that group.
#
# This script is meant to be run through pkexec by the GUI's
# memory-limit dialog. It takes no arguments: the user to add is
# taken from PKEXEC_UID, which pkexec sets and the caller cannot fake.

set -e

if [ "$(id -u)" -ne 0 ] || [ -z "$PKEXEC_UID" ]; then
	echo "This helper must be run through pkexec." >&2
	exit 2
fi

USER_NAME=$(getent passwd "$PKEXEC_UID" | cut -d: -f1)
if [ -z "$USER_NAME" ]; then
	echo "Could not find the user with id $PKEXEC_UID." >&2
	exit 3
fi

if ! getent group audio >/dev/null; then
	groupadd --system audio
fi

LIMITS_FILE=/etc/security/limits.d/95-audio.conf
if [ ! -f "$LIMITS_FILE" ] || ! grep -q "memlock" "$LIMITS_FILE"; then
	mkdir -p /etc/security/limits.d
	cat > "$LIMITS_FILE" <<EOF
# Added by the audio workstation's memory-limit fix.
# Lets members of the "audio" group lock memory and use realtime
# priority, which prevents dropouts during recording and playback.
@audio   -  rtprio     95
@audio   -  memlock    unlimited
EOF
	chmod 644 "$LIMITS_FILE"
fi

usermod -aG audio "$USER_NAME"

exit 0
