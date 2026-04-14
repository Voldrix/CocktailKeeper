#!/bin/bash

# zlib
wget https://zlib.net/current/zlib.tar.gz
tar -xf zlib.tar.gz
rm zlib.tar.gz
cd zlib-*
#./configure
make -j4
make install
cd ..

# png
git clone --depth 1 git://git.code.sf.net/p/libpng/code libpng
cd libpng
./autogen.sh
./configure
make -j4
make install
cd ..

# jpeg
wget https://www.ijg.org/files/jpegsrc.v9f.tar.gz
tar -xf jpegsrc.v9f.tar.gz
rm jpegsrc.v9f.tar.gz
cd jpeg-9f/
./configure
make -j4
make install
cd ..

# webp
git clone --depth 1 https://chromium.googlesource.com/webm/libwebp
cd libwebp
./autogen.sh
./configure --enable-libwebpdecoder
make -j4
make install
cd ..

