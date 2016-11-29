Auroracoin
=====================

Copyright (c) 2016 Auroracoin Developers


Running
---------------------
The following are some helpful notes on how to run Auroracoin on your native platform.

### Unix

You need the Qt4 run-time libraries to run Auroracoin-Qt. On Debian or Ubuntu:

	sudo apt-get install libqtgui4

Unpack the files into a directory and run:

- bin/32/auroracoin-qt (GUI, 32-bit) or bin/32/auroracoind (headless, 32-bit)
- bin/64/auroracoin-qt (GUI, 64-bit) or bin/64/auroracoind (headless, 64-bit)

### Windows

Unpack the files into a directory, and then run auroracoin-qt.exe.

### OSX

Drag Auroracoin-Qt to your applications folder, and then run Auroracoin-Qt.

Building
---------------------
The following are developer notes on how to build Auroracoin on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [OSX Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-msw.md)

Development
---------------------
The Auroracoin repo's [root README](https://github.com/aurarad/Auroracoin/blob/master/README.md) contains relevant information on the development process and automated testing.

- [Coding Guidelines](coding.md)

### Resources
* Discuss on the [BitcoinTalk](https://bitcointalk.org/index.php?topic=1467050.0) thread.

### Miscellaneous
- [Files](files.md)

License
---------------------
Distributed under the [MIT/X11 software license](http://www.opensource.org/licenses/mit-license.php).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](http://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
