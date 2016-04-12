MAINTAINERCLEANFILES = Makefile.in
EXTRA_DIST     =

AM_CPPFLAGS       = -I$(top_builddir) \
                 -I$(top_builddir)/ism/src \
                 -I$(top_srcdir) \
                 -I$(top_srcdir)/ism/src \
                 -I$(top_srcdir)/ism/intl \
                 -I$(top_srcdir)/ism/utils \
                 -I$(includedir) \
                 -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
                 -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
                 -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
                 -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
                 -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
                 -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
                 -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\" \
                 -DSCIM_KEYBOARD_ICON_FILE=\"@SCIM_ICONDIR@/keyboard.png\" \
                 @ISF_BUILDING_DLL@

if ISF_BUILD_WAYLAND_IMMODULE
CONFIG_WAYLAND_IMMODULE = module.la
endif

noinst_HEADERS     = wayland_imcontext.h

moduledir          = @WAYLAND_IM_MODULEDIR@
module_LTLIBRARIES = $(CONFIG_WAYLAND_IMMODULE)

module_la_SOURCES  = wayland_module.c \
                     wayland_imcontext.c

module_la_CFLAGS = @ECORE_IMF_CFLAGS@ \
                   @ECORE_EVAS_CFLAGS@ \
                   @ECORE_WL_CFLAGS@ \
                   @DLOG_CFLAGS@

module_la_LDFLAGS  = -rpath $(moduledir) \
                     -avoid-version \
                     -module

module_la_LIBADD   = $(LD_VERSION_SCRIPT_OPTION) \
                     @ECORE_IMF_LIBS@ \
                     @ECORE_EVAS_LIBS@ \
                     @ECORE_WL_LIBS@ \
                     @DLOG_LIBS@
