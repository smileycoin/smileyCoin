#!/bin/sh
set -ex

# This script will install smileycoin globally, create a systemd service for
# it and start the daemon.
#
# It is tested on Debian, but should hopefully work on anything systemd-based.

TARGETBIN="/usr/local/bin/smileycoind"
TARGETDATA="/var/local/smly"
TARGETCONF="${TARGETDATA}/smileycoin.conf"
TARGETUSER="smly"
TARGETGROUP="nogroup"

# ---------------------------

cp src/smileycoind "${TARGETBIN}"
chown root:root "${TARGETBIN}"
adduser --system \
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
ExecStart=${TARGETBIN} -datadir=${TARGETDATA} --server -printtoconsole
User=${TARGETUSER}
Group=${TARGETGROUP}

[Install]
WantedBy=multi-user.target
EOF
systemctl enable smly.service
systemctl start smly.service
