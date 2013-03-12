MAINTAINERCLEANFILES = Makefile.in
CLEANFILES           = *.bak

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

noinst_HEADERS = scim_setup_module.h \
                 isf_ise_setup_win.h \
                 isf_help_win.h \
                 isf_setup_win.h \
                 isf_lang_win.h \
                 isf_setup_utility.h 

if SCIM_BUILD_PANEL_GTK
CONFIG_SCIM_PANEL_GTK   = scim-panel-gtk
endif

bin_PROGRAMS            = $(CONFIG_SCIM_PANEL_GTK)

scim_panel_gtk_SOURCES  = scim_panel_gtk.cpp \
                          isf_ise_setup_win.cpp \
                          scim_setup_module.cpp \
                          isf_help_win.cpp \
                          isf_setup_win.cpp \
                          isf_lang_win.cpp \
                          isf_setup_utility.cpp 

scim_panel_gtk_LDFLAGS  = @LTLIBINTL@ -rpath $(libdir)

if ISF_BUILD_WITH_GCONF
scim_panel_gtk_CXXFLAGS = @GTK2_CFLAGS@ @GTHREAD2_CFLAGS@ @GCONF_CFLAGS@
scim_panel_gtk_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la \
                          $(top_builddir)/ism/utils/libscim-gtkutils@SCIM_EPOCH@.la \
                          -lgthread-2.0 -lphonestatus -lnoti \
                          @GTK2_LIBS@ @GTHREAD2_LIBS@ @GCONF_LIBS@ @X_LIBS@
else
scim_panel_gtk_CXXFLAGS = @GTK2_CFLAGS@ @GTHREAD2_CFLAGS@
scim_panel_gtk_LDADD    = $(top_builddir)/ism/src/libscim@SCIM_EPOCH@.la \
                          $(top_builddir)/ism/utils/libscim-gtkutils@SCIM_EPOCH@.la \
                          -lgthread-2.0 @GTK2_LIBS@ @GTHREAD2_LIBS@ @X_LIBS@
endif

