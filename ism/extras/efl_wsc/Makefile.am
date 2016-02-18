MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak *.edj

INCLUDES = -I$(top_builddir) \
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

noinst_HEADERS = isf_wsc_context.h \
					isf_wsc_control.h \
					isf_wsc_control_ui.h

if ISF_BUILD_WSC_EFL
CONFIG_WSC_EFL  = isf-wsc-efl
endif

bin_PROGRAMS           = $(CONFIG_WSC_EFL)

isf_wsc_efl_SOURCES  = isf_wsc_efl.cpp \
						 isf_wsc_context.cpp \
						 isf_wsc_control.cpp \
						 isf_wsc_control_ui.cpp

isf_wsc_efl_CXXFLAGS = @ECORE_CFLAGS@ \
                       @ECORE_IMF_CFLAGS@ \
                       @EINA_CFLAGS@ \
                       @VCONF_CFLAGS@ \
                       @WAYLAND_CFLAGS@ \
                       @WL_TEXT_INPUT_CFLAGS@ \
                       @XKBCOMMON_CFLAGS@ \
                       @DLOG_CFLAGS@ \
                       -fPIE

isf_wsc_efl_LDFLAGS  = @ECORE_LIBS@ @LTLIBINTL@ -rpath $(libdir) \
                       @ECORE_IMF_LIBS@ \
                       @EINA_LIBS@ \
                       @VCONF_LIBS@ \
                       @WAYLAND_LIBS@ \
                       @WL_TEXT_INPUT_LIBS@ \
                       @XKBCOMMON_LIBS@ \
                       @DLOG_LIBS@ \
                       -pie

isf_wsc_efl_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
