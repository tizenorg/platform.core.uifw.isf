MAINTAINERCLEANFILES 	= Makefile.in

CLEANFILES  = *.bak

if SCIM_BUILD_CONFIG_SIMPLE
configdir   = $(sysconfdir)/scim
config_DATA = config global
config_source_data = $(srcdir)/config $(srcdir)/global
endif

EXTRA_DIST  = $(config_DATA)
