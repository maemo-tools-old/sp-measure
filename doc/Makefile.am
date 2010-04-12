# man pages that go into section 3:
man3_MANS = *.3 man3/*.3

DISTCLEANFILES = Makefile.in

man3/*.3: all-local

all-local:
	doxygen doxygen.conf 

distclean-local:
	rm -rf man3