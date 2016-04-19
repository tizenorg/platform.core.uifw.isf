MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak *.edj

AM_CPPFLAGS = -I$(top_builddir) \
           -I$(top_builddir)/ism/src \
           -I$(top_srcdir) \
           -I$(top_srcdir)/ism/src \
           -I$(top_srcdir)/ism/intl \
           -I$(top_srcdir)/ism/data \
           -I$(top_srcdir)/ism/utils \
           -I$(includedir) \
           -DSCIM_DATADIR=\"@SCIM_DATADIR@\" \
           -DSCIM_LOCALEDIR=\"@SCIM_LOCALEDIR@\" \
           -DSCIM_SYSCONFDIR=\"@SCIM_SYSCONFDIR@\" \
           -DSCIM_LIBEXECDIR=\"@SCIM_LIBEXECDIR@\" \
           -DSCIM_ICONDIR=\"@SCIM_ICONDIR@\" \
           -DSCIM_MODULE_PATH=\"@SCIM_MODULE_PATH@\" \
           -DSCIM_TEMPDIR=\"@SCIM_TEMPDIR@\"

noinst_HEADERS = isf_panel_utility.h

if ISF_BUILD_PANEL_EFL
CONFIG_SCIM_PANEL_EFL  = isf-panel-efl
endif

bin_PROGRAMS           = $(CONFIG_SCIM_PANEL_EFL)

isf_panel_efl_SOURCES  = isf_panel_efl.cpp \
                         isf_panel_utility.cpp

isf_panel_efl_CXXFLAGS = @ECOREX_CFLAGS@ \
                         @ECORE_WL_CFLAGS@ \
                         @EDJE_CFLAGS@ \
                         @ELEMENTARY_CFLAGS@ \
                         @VCONF_CFLAGS@ \
                         @X_CFLAGS@ \
                         @DLOG_CFLAGS@ \
                         @NOTIFICATION_CFLAGS@ \
                         @TTS_CFLAGS@ \
                         @EDBUS_CFLAGS@ \
                         @BLUETOOTH_CFLAGS@ \
                         @FEEDBACK_CFLAGS@ \
                         @APP_CONTROL_CFLAGS@ \
                         @PKGMGR_INFO_CFLAGS@ \
                         -fPIE

isf_panel_efl_LDFLAGS  = @LTLIBINTL@ -rpath $(libdir) \
                         @ECOREX_LIBS@ \
                         @ECORE_WL_LIBS@ \
                         @EDJE_LIBS@ \
                         @ELEMENTARY_LIBS@ \
                         @VCONF_LIBS@ \
                         @X_LIBS@ \
                         @DLOG_LIBS@ \
                         @NOTIFICATION_LIBS@ \
                         @TTS_LIBS@ \
                         @EDBUS_LIBS@ \
                         @BLUETOOTH_LIBS@ \
                         @FEEDBACK_LIBS@ \
                         @APP_CONTROL_LIBS@ \
                         @PKGMGR_INFO_LIBS@ \
                         -pie

isf_panel_efl_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la \
                         $(top_builddir)/ism/src/libisf_pkg.la \
                         $(top_builddir)/ism/src/libprivilege_checker.la

