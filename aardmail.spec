Name: aardmail
Version: 0.2
Release: 1
Summary: Mail related tools
Group: System/Base
License: GPLv2
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
BuildRequires: pkgconfig(openssl)
BuildRequires: doxygen

%description
%{summary}.

%files
%defattr(-,root,root,-)
%{_bindir}/*
%doc %{_datadir}/man/man1/*


%package devel
Summary: Development files for %{name}
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
%{summary}.

%files devel
%defattr(-,root,root,-)
%{_libdir}/*.a
%doc %{_datadir}/man/man3/*

%prep
%setup -q


%build
make dep
make

%install
make DESTDIR=%{buildroot} install
