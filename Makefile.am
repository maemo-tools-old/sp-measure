include_HEADERS = src/sp_measure.h src/sp_measure_system.h src/sp_measure_process.h

SUBDIRS = src doc tests

DISTCLEANFILES = Makefile Makefile.in configure config.* autoscan.log aclocal.m4 config-*

distclean-local:
	rm -rf aux autom4te.cache
	
install-data-local:
	install -d $(DESTDIR)/$(docdir)/libsp-measure-dev/
	install -m 644 examples/res-monitor.c $(DESTDIR)/$(docdir)/libsp-measure-dev/
