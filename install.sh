#!/bin/sh
set -ex
# Install script for a system smileycoin daemon
# Copyright 2018 Jamie Lentin (jamie.lentin@shuttlethread.com)

# This script will install smileycoin globally, create a systemd service for
# it and start the daemon.
#
# It is tested on Debian, but should hopefully work on anything systemd-based.

PROJECT_PATH="${PROJECT_PATH-$(dirname "$(readlink -f "$0")")}"  # The full project path, e.g. /srv/tutor-web.beta
PROJECT_NAME="${PROJECT_NAME-$(basename ${PROJECT_PATH})}"  # The project directory name, e.g. tutor-web.beta
PROJECT_MODE="${PROJECT_MODE-development}"  # The project mode, development or production

SMLY_BIN="${SMLY_BIN-${PROJECT_PATH}/src/smileycoind}"

TARGETBIN="/usr/local/bin/smileycoind"
TARGETDATA="/var/local/smly"
TARGETCONF="${TARGETDATA}/smileycoin.conf"
TARGETUSER="smly"
TARGETGROUP="nogroup"

# ---------------------------

# The visible smileycoind should be a wrapper to set user/datadir
cat <<EOF > "${TARGETBIN}"
#!/bin/sh
sudo -u${TARGETUSER} "${SMLY_BIN}" -datadir="${TARGETDATA}" "\$@"
EOF
chown root:root "${TARGETBIN}"
chmod a+rwx "${TARGETBIN}"

grep -qE "^${TARGETUSER}:" /etc/passwd || adduser --system \
    --home "${TARGETDATA}" --no-create-home \
    --disabled-password \
    ${TARGETUSER}
mkdir -p "${TARGETDATA}"
chown -R ${TARGETUSER}:${TARGETGROUP} "${TARGETDATA}"

RPCPASS="$(xxd -ps -l 22 /dev/urandom)"
[ -e "${TARGETCONF}" ] || cat <<EOF > "${TARGETCONF}"
rpcuser=smileycoinrpc
rpcpassword=${RPCPASS}
EOF
cat <<EOF > /etc/systemd/system/smly.service
[Unit]
Description=SMLYcoin
After=network.target

[Service]
ExecStart=${SMLY_BIN} -datadir=${TARGETDATA} --server -printtoconsole -algo=skein
User=${TARGETUSER}
Group=${TARGETGROUP}

[Install]
WantedBy=multi-user.target
EOF
systemctl enable smly.service
systemctl start smly.service
