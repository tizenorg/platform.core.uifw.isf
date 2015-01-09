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

noinst_HEADERS = isf_wsm_utility.h

if ISF_BUILD_WSM_EFL
CONFIG_WSM_EFL  = isf-wsm-efl
endif

bin_PROGRAMS           = $(CONFIG_WSM_EFL)

isf_wsm_efl_SOURCES  = isf_wsm_efl.cpp \
                         isf_wsm_utility.cpp \
                         isf_wsm_query_utility.cpp

isf_wsm_efl_CXXFLAGS = @EFL_CFLAGS@ @VCONF_CFLAGS@ @DLOG_CFLAGS@ @TTS_CFLAGS@ @FEEDBACK_CFLAGS@

isf_wsm_efl_LDFLAGS  = @EFL_LIBS@ @LTLIBINTL@ -rpath $(libdir) \
                         @VCONF_LIBS@ \
                         @WAYLAND_LIBS@ \
                         @DLOG_LIBS@ \
                         @TTS_LIBS@ \
                         @FEEDBACK_LIBS@

isf_wsm_efl_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la

