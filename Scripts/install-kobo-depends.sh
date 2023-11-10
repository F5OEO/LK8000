#!/bin/sh
# 
# File:   install-kobo-depends.sh
# Author: Bruno de Lacheisserie
#
# Created on Jan 1, 2015, 6:39:30 PM
#

set -ex


[ -z "$TC"] && TC=arm-kobo-linux-gnueabihf
[ -z "$BUILD_DIR" ] && BUILD_DIR=$HOME/tmp
[ -z "$TARGET_DIR" ] && TARGET_DIR=/opt/kobo-rootfs

if [ command -v ${TC}-gcc >/dev/null ]; then
    echo "error : ${TC} toolchain not available"
    exit 1
fi

NB_CORES=$(grep -c '^processor' /proc/cpuinfo)
export MAKEFLAGS="-j$((NB_CORES+1)) -l${NB_CORES}"

[ ! -d ${BUILD_DIR} ] && mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}


BUILD_FLAGS="-O3 -march=armv7-a -mfpu=neon -ftree-vectorize -mvectorize-with-neon-quad"


# install zlib ( 1.2.13 - 2022-10-13 )
[ ! -f zlib-1.2.13.tar.gz ] && wget https://zlib.net/fossils/zlib-1.2.13.tar.gz
[ -d zlib-1.2.13 ] && rm -rf zlib-1.2.13
tar -xvzf zlib-1.2.13.tar.gz
cd zlib-1.2.13
CC=$TC-gcc CFLAGS=$BUILD_FLAGS \
./configure --prefix=$TARGET_DIR
make all && make install
cd ..

# install boostlib ( 1.82.0 - 2023-04-14 )
[ ! -f boost_1_82_0.tar.gz ] && wget https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.gz
[ -d boost_1_82_0 ] && rm -rf boost_1_82_0
tar xzf boost_1_82_0.tar.gz
cd boost_1_82_0
./bootstrap.sh
echo "using gcc : arm : $TC-g++ : cxxflags=$BUILD_FLAGS ;" > user-config.jam
./b2 toolset=gcc-arm \
           -q -d1 \
           variant=release \
           link=shared \
           runtime-link=shared \
           --prefix=$TARGET_DIR \
           --address-model=32 \
           --with-headers \
           -sZLIB_SOURCE="$BUILD_DIR/zlib-1.2.11" \
           -sZLIB_INCLUDE="$TARGET_DIR\include" \
           -sZLIB_LIBPATH="$TARGET_DIR\lib" \
           --user-config=user-config.jam \
           install
cd ..

# install libpng ( 1.6.40 - 2023-06-21 )
[ ! -f libpng-1.6.40.tar.gz ] && wget http://sourceforge.net/projects/libpng/files/libpng16/1.6.40/libpng-1.6.40.tar.gz
[ -d libpng-1.6.40 ] && rm -rf libpng-1.6.40
[ -d libpng-build ] && rm -rf libpng-build
tar xzf libpng-1.6.40.tar.gz
mkdir libpng-build
cd libpng-build
../libpng-1.6.40/configure \
    --host=$TC \
    CC=$TC-gcc \
    AR=$TC-ar \
    STRIP=$TC-strip \
    RANLIB=$TC-ranlib \
    CPPFLAGS="-funwind-tables $BUILD_FLAGS -I$TARGET_DIR/include" \
    LDFLAGS="-L$TARGET_DIR/lib" \
    --prefix=$TARGET_DIR \
    --enable-arm-neon
make && make install
cd ..

# install freetype2 ( 2.13.1 - 2020-10-19 )
[ ! -f freetype-2.13.1.tar.gz ] && wget https://download.savannah.gnu.org/releases/freetype/freetype-2.13.1.tar.gz
[ -d freetype-2.13.1 ] && rm -rf freetype-2.13.1
[ -d freetype-build ] && rm -rf freetype-build
tar xzf freetype-2.13.1.tar.gz
mkdir freetype-build
cd freetype-build
CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
../freetype-2.13.1/configure \
    --host=$TC \
    --target=$TC \
    --prefix=$TARGET_DIR \
    --without-harfbuzz \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..

# install Geographiclib ( 1.52 -  2021-06-21 )
[ ! -f GeographicLib-1.52.tar.gz ] && wget https://netcologne.dl.sourceforge.net/project/geographiclib/distrib/GeographicLib-1.52.tar.gz
[ -d GeographicLib-1.52 ] && rm -rf GeographicLib-1.52
[ -d GeographicLib-build ] && rm -rf GeographicLib-build
tar xzf GeographicLib-1.52.tar.gz
mkdir GeographicLib-build
cd GeographicLib-build
CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
../GeographicLib-1.52/configure \
    --host=$TC \
    --prefix=$TARGET_DIR \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..

# install bluez-libs ( 3.36 )
[ ! -f bluez-libs-3.36.tar.gz ] && wget http://bluez.sf.net/download/bluez-libs-3.36.tar.gz
[ -d bluez-libs-3.36 ] && rm -rf bluez-libs-3.36
[ -d bluez-libs-build ] && rm -rf bluez-libs-build
tar xzf bluez-libs-3.36.tar.gz
mkdir bluez-libs-build
cd bluez-libs-build
CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
../bluez-libs-3.36/configure \
    --host=$TC \
    --prefix=$TARGET_DIR \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..

# install readline ( 7.0 2016 )
[ ! -f readline-7.0.tar.gz ] && wget https://ftp.gnu.org/gnu/readline/readline-7.0.tar.gz
[ -d readline-7.0 ] && rm -rf readline-7.0
[ -d readline-build ] && rm -rf readline-build
tar xzf readline-7.0.tar.gz
mkdir readline-build
cd readline-build
CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
../readline-7.0/configure \
    --host=$TC \
    --prefix=$TARGET_DIR \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..

# install alsa-lib ( 1.2.10 )
[ ! -f alsa-lib-1.2.10.tar.bz2 ] && wget http://www.alsa-project.org/files/pub/lib/alsa-lib-1.2.10.tar.bz2
[ -d alsa-lib-1.2.10 ] && rm -rf alsa-lib-1.2.10
[ -d alsa-build ] && rm -rf alsa-build
tar xf alsa-lib-1.2.10.tar.bz2
mkdir alsa-build
cd alsa-build
CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
../alsa-lib-1.2.10/configure \
    --host=$TC \
    --prefix=$TARGET_DIR \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..

# install libsndfile ( 1.2.2 )
[ ! -f 1.2.2.tar.gz ] && wget https://github.com/libsndfile/libsndfile/archive/refs/tags/1.2.2.tar.gz
[ -d libsndfile-1.2.2 ] && rm -rf libsndfile-1.2.2

tar xzf 1.2.2.tar.gz
cd libsndfile-1.2.2
autoreconf -vif

CFLAGS=$BUILD_FLAGS \
LDFLAGS="-L$TARGET_DIR/lib"  \
./configure \
    --host=$TC \
    --prefix=$TARGET_DIR \
    CPPFLAGS="$BUILD_FLAGS -I$TARGET_DIR/include" \
    LDFLAGS="-L$TARGET_DIR/lib" \
    PKG_CONFIG_LIBDIR=$TARGET_DIR/lib/pkgconfig
make && make install
cd ..