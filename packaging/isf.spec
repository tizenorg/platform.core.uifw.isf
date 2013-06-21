%define _usrdir /usr
%define _ugdir  %{_usrdir}/ug

Name:       isf
Summary:    Input Service Framework
Version:    2.4.6621
Release:    2
Group:      Graphics & UI Framework/Input
License:    LGPL-2.1
Source0:    %{name}-%{version}.tar.gz
Source1001: isf.manifest
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tts)
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig

%description
Input Service Framewok (ISF) is an input method (IM) platform,
and it has been derived from SCIM.

%package devel
Summary:    ISF header files
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.

%package -n ug-isfsetting-efl
Summary:    ISF setting ug
Requires:   %{name} = %{version}-%{release}

%description -n ug-isfsetting-efl
ISF setting UI Gadget

%prep
%setup -q
cp %{SOURCE1001} .

%build
CFLAGS+=" -fvisibility=hidden "; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ";export CXXFLAGS

%autogen
%configure --disable-static \
            --disable-tray-icon \
            --disable-filter-sctc
make %{?_smp_mflags}

%install

%make_install

mkdir -p %{buildroot}/etc/scim/conf
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/Helper
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine

%find_lang isfsetting-efl
%find_lang scim

cat scim.lang > isf.lang

%post
/sbin/ldconfig


/usr/bin/vconftool set -t bool file/private/isf/autocapital_allow 1 -g 6514 || :
/usr/bin/vconftool set -t bool file/private/isf/autoperiod_allow 0 -g 6514 || :
/usr/bin/vconftool set -t string db/isf/input_language "en_US" -g 5000 || :

%postun -p /sbin/ldconfig

%post -n ug-isfsetting-efl
mkdir -p /opt/ug/bin/
ln -sf /usr/bin/ug-client /opt/ug/bin/isfsetting-efl

%files -f isf.lang
%manifest %{name}.manifest
/etc/smack/accesses.d/%{name}.rule
%defattr(-,root,root,-)
%dir /etc/scim/conf
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/Helper
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine
%{_libdir}/systemd/user/core-efl.target.wants/scim.service
%{_libdir}/systemd/user/scim.service
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_bindir}/isf-demo-efl
%{_bindir}/scim
%{_bindir}/isf-log
%{_bindir}/isf-panel-efl
%{_bindir}/isf-query-engines
%{_libdir}/*/immodules/*.so
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/*.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-*.so*
%license COPYING

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc


%files -n ug-isfsetting-efl -f isfsetting-efl.lang
%manifest ug-isfsetting-efl.manifest
/etc/smack/accesses.d/ug.isfsetting-efl.include
/usr/share/packages/ug-isfsetting-efl.xml
%{_ugdir}/lib/libug-isfsetting-efl.so
%{_datadir}/scim/isfsetting.edj
