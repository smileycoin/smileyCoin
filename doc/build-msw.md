WINDOWS BUILD NOTES
===================

Follow this guide:

https://bitcointalk.org/index.php?topic=149479.0

### Notes on guide:
* Download and install 7zip

* **2.2** *Berkeley DB*: You will need to download version 5.3

* **2.4** *Miniupnpc*: Skip this

* **2.7** *Qt 5 libraries*: The files can be found at https://download.qt.io/archive/qt/5.3/5.3.2/submodules/

* **3** Download and unpack Smileycoin from github: https://github.com/tutor-web/smileyCoin

*From msys shell configure and make smileycoin:*

```
cd /c/smileyCoin

./autogen.sh

CPPFLAGS="-I/c/deps/db-5.3.21/build_unix \
-I/c/deps/openssl-1.0.1l/include \
-I/c/deps \
-I/c/deps/protobuf-2.6.1/src \
-I/c/deps/libpng-1.6.16 \
-I/c/deps/qrencode-3.4.4" \
LDFLAGS="-L/c/deps/db-5.3.21/build_unix \
-L/c/deps/openssl-1.0.1l \
-L/c/deps/protobuf-2.6.1/src/.libs \
-L/c/deps/libpng-1.6.16/.libs \
-L/c/deps/qrencode-3.4.4/.libs" \
BOOST_ROOT=/c/deps/boost_1_57_0 \
./configure \
--disable-tests \
--with-qt-incdir=/c/Qt/5.3.2/include \
--with-qt-libdir=/c/Qt/5.3.2/lib \
--with-qt-plugindir=/c/Qt/5.3.2/plugins \
--with-qt-bindir=/c/Qt/5.3.2/bin \
--with-protoc-bindir=/c/deps/protobuf-2.6.1/src

make

strip src/smileycoin-cli.exe
strip src/smileycoind.exe
strip src/qt/smileycoin-qt.exe 
```