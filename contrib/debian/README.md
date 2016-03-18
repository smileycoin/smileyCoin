
Debian
====================
This directory contains files used to package auroracoind/auroracoin-qt
for Debian-based Linux systems. If you compile auroracoind/auroracoin-qt yourself, there are some useful files here.

## auroracoin: URI support ##


auroracoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install auroracoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your auroracoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/auroracoin128.png` to `/usr/share/pixmaps`

auroracoin-qt.protocol (KDE)

