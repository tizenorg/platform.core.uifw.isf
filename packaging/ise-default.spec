Name:       ise-default
Summary:    Tizen keyboard
Version:    0.7.9
Release:    1
Group:      TO BE / FILLED IN
License:    TO BE / FILLED IN
Source0:    ise-default-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  gettext-tools
BuildRequires:  cmake
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(isf)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(sensor)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(libscl-ui)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(libxml-2.0)



%description
Description: Tizen keyboard



%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install

%post

%postun

%files 
%defattr(-,root,root,-)
%{_libdir}/scim-1.0/1.4.0/Helper/ise-default.so
%{_libdir}/scim-1.0/1.4.0/SetupUI/ise-default-setup.so
%{_datadir}/isf/ise/ise-default/*
%{_datadir}/packages/*
%{_datadir}/locale/*

