AM_CFLAGS = @WARN_CFLAGS@ @ALTIVEC_CFLAGS@

EXTRA_PROGRAMS =

bin_PROGRAMS =
lib_LTLIBRARIES = libtwin/libtwin.la
pkginclude_HEADERS = libtwin/twin.h

libtwin_libtwin_la_LDFLAGS = -version-info @LIB_VERSION@
libtwin_libtwin_la_SOURCES = \
	libtwin/twin_box.c \
	libtwin/twin_button.c \
	libtwin/twin_convolve.c \
	libtwin/twin_cursor.c \
	libtwin/twin_dispatch.c \
	libtwin/twin_draw.c \
	libtwin/twin_feature.c \
	libtwin/twin_hull.c \
	libtwin/twin_icon.c \
	libtwin/twin_file.c \
	libtwin/twin_fixed.c \
	libtwin/twin_font.c \
	libtwin/twin_font_default.c \
	libtwin/twin_geom.c \
	libtwin/twin_label.c \
	libtwin/twin_matrix.c \
	libtwin/twin_path.c \
	libtwin/twin_pattern.c \
	libtwin/twin_pixmap.c \
	libtwin/twin_poly.c \
	libtwin/twin_primitive.c \
	libtwin/twin_queue.c \
	libtwin/twin_screen.c \
	libtwin/twin_spline.c \
	libtwin/twin_timeout.c \
	libtwin/twin_toplevel.c \
	libtwin/twin_trig.c \
	libtwin/twin_widget.c \
	libtwin/twin_window.c \
	libtwin/twin_work.c \
	libtwin/twinint.h \
	twin_def.h

if TWIN_X11
pkginclude_HEADERS += libtwin/twin_x11.h
libtwin_libtwin_la_SOURCES += libtwin/twin_x11.c
endif

if TWIN_FB
pkginclude_HEADERS += libtwin/twin_fbdev.h
libtwin_libtwin_la_SOURCES += libtwin/twin_fbdev.c
endif

if TWIN_MOUSE
pkginclude_HEADERS += libtwin/twin_linux_mouse.h
libtwin_libtwin_la_SOURCES += libtwin/twin_linux_mouse.c
endif

if TWIN_JOYSTICK
pkginclude_HEADERS += libtwin/twin_linux_js.h
libtwin_libtwin_la_SOURCES += libtwin/twin_linux_js.c
endif

if TWIN_PNG
pkginclude_HEADERS += libtwin/twin_png.h
libtwin_libtwin_la_SOURCES += libtwin/twin_png.c
endif

if TWIN_JPEG
pkginclude_HEADERS += libtwin/twin_jpeg.h
libtwin_libtwin_la_SOURCES += libtwin/twin_jpeg.c
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libtwin.pc

# demo twin applications
EXTRA_PROGRAMS += twin_demos/xtwin twin_demos/ftwin
noinst_PROGRAMS =
noinst_LIBRARIES = twin_demos/libtwin_demos.a

if TWIN_X11
noinst_PROGRAMS += twin_demos/xtwin
endif

if TWIN_FB
noinst_PROGRAMS += twin_demos/ftwin
endif

twin_demos_xtwin_SOURCES = twin_demos/xtwin.c
twin_demos_xtwin_CFLAGS = @TWIN_DEP_CFLAGS@ -I$(top_srcdir)/twin_demos
twin_demos_xtwin_LDADD = twin_demos/libtwin_demos.a libtwin/libtwin.la \
	@TWIN_DEP_LDFLAGS@

twin_demos_ftwin_SOURCES = twin_demos/ftwin.c
twin_demos_ftwin_CFLAGS = @TWIN_DEP_CFLAGS@ -I$(top_srcdir)/twin_demos
twin_demos_ftwin_LDADD = twin_demos/libtwin_demos.a libtwin/libtwin.la \
	@TWIN_DEP_LDFLAGS@

twin_demos_libtwin_demos_a_CFLAGS = @WARN_CFLAGS@ -I$(top_srcdir)/twin_demos
twin_demos_libtwin_demos_a_SOURCES = \
	twin_demos/twin_calc.c twin_demos/twin_calc.h \
	twin_demos/twin_clock.c twin_demos/twin_clock.h \
	twin_demos/twin_demo.c twin_demos/twin_demo.h \
	twin_demos/twin_demoline.c twin_demos/twin_demoline.h \
	twin_demos/twin_demospline.c twin_demos/twin_demospline.h \
	twin_demos/twin_hello.c twin_demos/twin_hello.h \
	twin_demos/twin_text.c twin_demos/twin_text.h

# at the moment, the twin demos require twinint.h :(
twin_demos_xtwin_CFLAGS += -I$(top_srcdir)/libtwin
twin_demos_ftwin_CFLAGS += -I$(top_srcdir)/libtwin
twin_demos_libtwin_demos_a_CFLAGS += -I$(top_srcdir)/libtwin

# twin_ttf
EXTRA_PROGRAMS += twin_ttf/twin_ttf

if TWIN_TTF
bin_PROGRAMS += twin_ttf/twin_ttf
endif

twin_ttf_twin_ttf_SOURCES = \
	twin_ttf/twin_ttf.h \
	twin_ttf/twin_ttf.c

twin_ttf_twin_ttf_LDADD = @FREETYPE_LIBS@ -lm
twin_ttf_twin_ttf_CFLAGS = -g @WARN_CFLAGS@
twin_ttf_twin_ttf_CPPFLAGS = @FREETYPE_CFLAGS@ -I$(top_srcdir)/twin_ttf
