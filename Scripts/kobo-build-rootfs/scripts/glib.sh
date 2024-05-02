#!/bin/bash
set -e -u
ARCHIVE_URL=https://download.gnome.org/sources/glib/2.44/glib-2.44.1.tar.xz
ARCHIVE=glib-2.44.1.tar.xz
ARCHIVEDIR=glib-2.44.1
. $KOBO_SCRIPT_DIR/build-common.sh
pushd $ARCHIVEDIR

	PKG_CONFIG_PATH="${DEVICEROOT}/lib/pkgconfig" \
	CFLAGS="${CFLAGS}" \
	LDFLAGS="${LDFLAGS}" \
	./configure \
		--prefix=/${DEVICEROOT} \
		--host=${CROSSTARGET} \
		--disable-man \
		--disable-gtk-doc \
		--disable-silent-rules \
		--cache=glib.config.cache \
		--disable-static \
		--with-libiconv=gnu \
		--disable-compile-warnings \
		glib_cv_stack_grows=yes \
		ac_cv_func_posix_getpwuid_r=yes \
		ac_cv_func_posix_getgrgid_r=yes \
		glib_cv_uscore=yes


	$MAKE -j$MAKE_JOBS
	$MAKE install

#	cp glib/glibconfig.h /${DEVICEROOT}/include

popd

markbuilt
