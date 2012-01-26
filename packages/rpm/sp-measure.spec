Name: libsp-measure
Version: 1.3.3
Release: 1%{?dist}
Summary: API for measuring system/process resource usage
Group: Development/Libraries
License: LGPLv2+
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/sp-measure
Source: sp-measure_%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-build
BuildRequires: automake, doxygen, libtool

%description
API for taking system/process resource usage snaphots and calculating difference between two snapshots of the same type.

%prep
%setup -q -n sp-measure

%build
autoreconf -fvi

%configure --docdir=%{_defaultdocdir}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm %{buildroot}/usr/lib/*a

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root,-)
%{_libdir}/libspmeasure.so.*
%doc COPYING README

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%package devel
Summary: Development files for libsmeasure library
Group: Development/Libraries
Requires: %{name} = %{version}

%description devel 
Libmeasure development files (headers and static libraries).

%files devel
%defattr(-,root,root,-)
%{_libdir}/libspmeasure.so
%{_includedir}/*.h
%{_defaultdocdir}/%{name}-dev/res-monitor.c
%{_mandir}/man3/*


%changelog
* Fri Nov 11 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.3
  * Fix file descriptor leak in get_process_name().

* Fri Sep 09 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.2
  * Fix CPU frequency calculation.

* Fri Aug 05 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3.1
  * Fix crashes when /proc/ reading fails.

* Tue May 10 2011 Eero Tamminen <eero.tamminen@nokia.com> 1.3
  * Add support for Cgroups memory usage monitoring

* Tue Oct 12 2010 Eero Tamminen <eero.tamminen@nokia.com> 1.2.1
  * Packaging fixes.

* Fri Sep 24 2010 Eero Tamminen <eero.tamminen@nokia.com> 1.2
  * Fix sp_measure_process.h, so that it can be included in C++ code.

* Tue May 11 2010 Eero Tamminen <eero.tamminen@nokia.com> 1.1.1
  * use LGPL as license.  

* Fri Apr 30 2010 Eero Tamminen <eero.tamminen@nokia.com> 1.1
  * API change, allow retrieving of part of the data to fail and retrieving
    of rest of data to continue. 
  * System memory usage should take swap into account. 

* Mon Apr 12 2010 Eero Tamminen <eero.tamminen@nokia.com> 1.0
  * Initial Release. 
