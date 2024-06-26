#!/bin/bash
set -e -u

ARCHIVE_URL=https://github.com/labapart/gattlib/archive/refs/heads/master.zip
ARCHIVE=gattlib-master.zip
ARCHIVEDIR=gattlib-master
. $KOBO_SCRIPT_DIR/build-common.sh

pushd $ARCHIVEDIR

    SYSROOT=$(${CROSSTARGET}-gcc --print-sysroot) \
    CFLAGS=${CFLAGS} \
	CPPFLAGS=${CPPFLAGS} \
    CROSSTARGET=${CROSSTARGET} \
    DEVICEROOT=${DEVICEROOT} \
    PKG_CONFIG_PATH=:$DEVICEROOT/lib/pkgconfig \
    cmake \
        -DCMAKE_TOOLCHAIN_FILE=${KOBO_SCRIPT_DIR}/arm-kobo-linux-gnueabihf.cmake \
        -DGATTLIB_BUILD_DOCS=OFF \
        -DGATTLIB_BUILD_EXAMPLES=OFF \
        -DGATTLIB_PYTHON_INTERFACE=OFF \
        -DGATTLIB_SHARED_LIB=ON \
        -DBLUEZ_INCLUDE_DIRS=${DEVICEROOT} \
        -DCMAKE_INSTALL_PREFIX=${DEVICEROOT} \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo

	$MAKE -j$MAKE_JOBS
	$MAKE install

popd
markbuilt
