MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak *.edj
EXTRA_DIST		= ecoresocket.version-script

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

noinst_HEADERS =


CONFIG_ECORESOCKET_PANELAGENT_MODULE  = ecoresocket.la


if SCIM_LD_VERSION_SCRIPT
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/ecoresocket.version-script"
endif

moduledir		= @SCIM_MODULE_PATH@/$(SCIM_BINARY_VERSION)/PanelAgent
module_LTLIBRARIES	= $(CONFIG_ECORESOCKET_PANELAGENT_MODULE)

ecoresocket_la_SOURCES  = ecore_socket_panel_agent_module.cpp

ecoresocket_la_CXXFLAGS = @DLOG_CFLAGS@ @ECORE_CFLAGS@

ecoresocket_la_LDFLAGS  = -avoid-version \
		     	  -rpath $(moduledir) \
			  -module \
			  $(LD_VERSION_SCRIPT_OPTION) \
			  @LIBTOOL_EXPORT_OPTIONS@ \
			  @LTLIBINTL@ \
			  @DLOG_LIBS@ \
			  @ECORE_LIBS@

ecoresocket_la_LIBADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
