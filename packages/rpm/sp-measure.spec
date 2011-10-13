%define _defaultdocdir %{_datadir}/doc

Name: libsp-measure	
Version: 1.2.1	
Release: 1%{?dist}
Summary: API for measuring system/process resource usage
Group: Development/Libraries
License: LGPLv2+	
URL: http://www.gitorious.org/+maemo-tools-developers/maemo-tools/sp-measure	
Source: sp-measure_%{version}.tar.gz
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires: automake, doxygen, libtool

%description
API for taking system/process resource usage snaphots and calculating difference between two snapshots of the same type.

%prep
%setup -q -n sp-measure

%build
autoreconf -fvi
%configure 
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}
rm %{buildroot}/usr/lib/*a

%clean
rm -rf %{buildroot}

%files
%defattr(644,root,root,-)
%{_libdir}/libspmeasure.so.*
%doc COPYING README

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%package devel
Summary: Development files for libsmeasure library
Group: Development/Libraries

%description devel 
libspmeasure headeres and static libraries.

%files devel
%defattr(644,root,root,-)
%{_libdir}/libspmeasure.so
%{_includedir}/*.h
%{_defaultdocdir}/%{name}-dev/res-monitor.c
%{_mandir}/man3/*



