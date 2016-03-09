MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak *.edj
EXTRA_DIST		= wayland.version-script

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

noinst_HEADERS = isf_wsc_context.h \
					isf_wsc_control.h \
					isf_wsc_control_ui.h

#if ISF_BUILD_WSC_EFL
CONFIG_WSC_PANELAGENT_MODULE  = wayland.la
#endif

if SCIM_LD_VERSION_SCRIPT
LD_VERSION_SCRIPT_OPTION="-Wl,--version-script=$(srcdir)/wayland.version-script"
endif

moduledir		= @SCIM_MODULE_PATH@/$(SCIM_BINARY_VERSION)/PanelAgent
module_LTLIBRARIES	= $(CONFIG_WSC_PANELAGENT_MODULE)

wayland_la_SOURCES  = wayland_panel_agent_module.cpp \
						 isf_wsc_control.cpp \
						 isf_wsc_control_ui.cpp

wayland_la_CXXFLAGS = @DLOG_CFLAGS@ \
                       @ECORE_CFLAGS@ \
                       @ECORE_IMF_CFLAGS@ \
                       @EINA_CFLAGS@ \
                       @VCONF_CFLAGS@ \
                       @WAYLAND_CFLAGS@ \
                       @WL_TEXT_INPUT_CFLAGS@ \
                       @WL_INPUT_METHOD_CFLAGS@ \
                       @XKBCOMMON_CFLAGS@

wayland_la_LDFLAGS  = -avoid-version \
			-rpath $(moduledir) \
			-module \
			$(LD_VERSION_SCRIPT_OPTION) \
			@LIBTOOL_EXPORT_OPTIONS@ \
			@LTLIBINTL@ \
			@DLOG_LIBS@ \
			@ECORE_LIBS@ \
			@EINA_LIBS@ \
			@VCONF_LIBS@ \
			@WAYLAND_LIBS@ \
			@WL_INPUT_METHOD_LIBS@ \
			@XKBCOMMON_LIBS@

wayland_la_LIBADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la
