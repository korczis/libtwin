dnl
dnl Twin - A Tiny Window System
dnl Copyright © 2003 Keith Packard, Noah Levitt
dnl All rights reserved.
dnl
dnl This Library is free software; you can redistribute it and/or
dnl modify it under the terms of the GNU Library General Public License as
dnl published by the Free Software Foundation; either version 2 of the
dnl License, or (at your option) any later version.
dnl
dnl This Library is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl Library General Public License for more details.
dnl
dnl You should have received a copy of the GNU Library General Public
dnl License along with the Gnome Library; see the file COPYING.LIB.  If not,
dnl write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
dnl Boston, MA 02111-1307, USA.
dnl
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])

AC_INIT(libtwin, 0.0.3, [keithp@keithp.com], libtwin)
AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE

# Library versioning info
LIB_VERSION_CURRENT=0
LIB_VERSION_REVISION=1
LIB_VERSION_AGE=0
LIB_VERSION="$LIB_VERSION_CURRENT:$LIB_VERSION_REVISION:$LIB_VERSION_AGE"
AC_SUBST(LIB_VERSION)

AM_CONFIG_HEADER(twin_def.h)

# Check for progs
AC_PROG_CC
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

WARN_CFLAGS=""

if test "x$GCC" = "xyes"; then
	WARN_CFLAGS="-Wall -Wpointer-arith -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wnested-externs"
fi
AC_SUBST(WARN_CFLAGS)

# Check for X
AC_ARG_ENABLE(x11,
	AC_HELP_STRING([--disable-x11],
		[Disable x11 support (default=enabled)]),
	twin_x11="$enableval", twin_x11="yes")

if test "x$twin_x11" = "xyes"
then
	PKG_CHECK_MODULES(X, x11, twin_x11="yes", twin_x11="no")
fi
AC_SUBST(X_CFLAGS)
AC_SUBST(X_LIBS)

# Check for linux framebuffer includes
AC_ARG_ENABLE(fbdev,
	AC_HELP_STRING([--disable-fbdev],
		[Disable framebuffer support (default=enabled)]),
	twin_fb="$enableval", twin_fb="yes")

if test "x$twin_fb" = "xyes"
then
	AC_CHECK_HEADER(linux/fb.h, twin_fb="yes", twin_fb="no")
fi

# linux mouse
AC_ARG_ENABLE(linux-mouse,
	AC_HELP_STRING([--disable-linux-mouse],
		[Disable linux mouse support (default=enabled)]),
	twin_mouse="$enableval", twin_mouse="yes")

# linux joystick
AC_ARG_ENABLE(linux-joystick,
	AC_HELP_STRING([--disable-linux-joystick],
		[Disable linux joystick support (default=enabled)]),
	twin_joystick="$enableval", twin_joystick="yes")

# zlib
AC_ARG_ENABLE(zlib,
	AC_HELP_STRING([--disable-zlib],
		[Disable zlib support (default=enabled)]),
	twin_zlib="$enableval", twin_zlib="yes")
AH_TEMPLATE(HAVE_ZLIB, [Define if the zlib compression library is available])

if test "x$twin_zlib" = "zlib"
then
        AC_CHECK_HEADER(zlib.h, twin_zlib="yes", twin_zlib="no")
fi

if test "x$twin_zlib" = "xyes"
then
	Z_LIBS=-lz
	AC_DEFINE([HAVE_ZLIB])
fi

# png support
AC_ARG_ENABLE(png,
	AC_HELP_STRING([--disable-png],
		[Disable png support (default=enabled)]),
	twin_png="$enableval", twin_png="yes")

if test "x$twin_png" = "xyes"
then
	PKG_CHECK_MODULES(PNG, libpng12, twin_png="yes", twin_png="no")
fi

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)

# jpeg support
AC_ARG_ENABLE(jpeg,
	AC_HELP_STRING([--disable-jpeg],
		[Disable jpeg support (default=enabled)]),
	twin_jpeg="$enableval", twin_jpeg="yes")

if test "x$twin_jpeg" = "xyes"
then
        AC_CHECK_HEADER(jpeglib.h, twin_jpeg="yes", twin_jpeg="no")
fi

if test "x$twin_jpeg" = "xyes"
then
	JPEG_LIBS=-ljpeg
fi

# Check for freetype
AC_ARG_ENABLE(twin-ttf,
	AC_HELP_STRING([--disable-twin-ttf],
		[Don't build twin ttf font converter (default=enabled)]),
	twin_ttf="$enableval", twin_ttf="yes")

if test "x$twin_ttf" = "xyes"
then
	PKG_CHECK_MODULES(FREETYPE, freetype2, twin_ttf="yes", twin_ttf="no")
fi
AC_SUBST(FREETYPE_LIBS)
AC_SUBST(FREETYPE_CFLAGS)

# Altivec support
AC_ARG_ENABLE(altivec,
	AC_HELP_STRING([--enable-altivec],
		[Enable altivec support (default=detect)]),
	twin_altivec="$enableval")

if test x$twin_altivec = x
then
	AC_MSG_CHECKING([for altivec support])
	CFLAGS_save="$CFLAGS"
	CFLAGS="$CFLAGS -maltivec"
	AC_TRY_COMPILE([#include <altivec.h>],
		[vector unsigned int v = vec_splat_u32(0)],
		twin_altivec=yes,
		twin_altivec=no)
	CFLAGS="$CFLAGS_save"
	AC_MSG_RESULT($twin_altivec)
fi

ALTIVEC_CFLAGS=""
AH_TEMPLATE(HAVE_ALTIVEC,
	[Define if the C compiler supports altivec extensions])

if test x$twin_altivec = xyes
then
	AC_DEFINE([HAVE_ALTIVEC])
	ALTIVEC_CFLAGS="-maltivec"
fi
AC_SUBST(ALTIVEC_CFLAGS)


# TWIN_DEP_*FLAGS define all flags required by dependencies of libtwin
TWIN_DEP_LDFLAGS="$X_LIBS $PNG_LIBS $JPEG_LIBS $Z_LIBS -lm"
TWIN_DEPCFLAGS="$X_CFLAGS $PNG_CFLAGS"
AC_SUBST(TWIN_DEP_CFLAGS)
AC_SUBST(TWIN_DEP_LDFLAGS)

AM_CONDITIONAL(TWIN_X11, test x$twin_x11 = xyes)
AM_CONDITIONAL(TWIN_FB, test x$twin_fb = xyes)
AM_CONDITIONAL(TWIN_ZLIB, test x$twin_zlib = xyes)
AM_CONDITIONAL(TWIN_PNG, test x$twin_png = xyes)
AM_CONDITIONAL(TWIN_JPEG, test x$twin_jpeg = xyes)
AM_CONDITIONAL(TWIN_TTF, test x$twin_ttf = xyes)
AM_CONDITIONAL(TWIN_MOUSE, test x$twin_mouse = xyes)
AM_CONDITIONAL(TWIN_JOYSTICK, test x$twin_joystick = xyes)

AC_MSG_NOTICE([x11 support:    $twin_x11])
AC_MSG_NOTICE([fbdev support:  $twin_fb])
AC_MSG_NOTICE([zlib support:   $twin_zlib])
AC_MSG_NOTICE([png support:    $twin_png])
AC_MSG_NOTICE([jpeg support:   $twin_jpeg])
AC_MSG_NOTICE([twin_ttf tool:  $twin_ttf])
AC_MSG_NOTICE([linux mouse:    $twin_mouse])
AC_MSG_NOTICE([linux joystick: $twin_joystick])
AC_MSG_NOTICE([altivec:        $twin_altivec])

AC_OUTPUT([Makefile
           libtwin.pc])
