#sbs-git:framework/uifw/isf isf 2.3.5923 07f2b65224e6cef5cd6799065bb01fa656bc115e
%define _usrdir	/usr
%define _ugdir	%{_usrdir}/ug

Name:       isf
Summary:    Input Service Framework
Version:    2.3.5923
Release:    2
Group:      TO_BE/FILLED_IN
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Source1:    isf-panel.service
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(syspopup-caller)
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig
requires:       e17, net-config

%description
Input Service Framewok (ISF) is an input method (IM) platform, and it has been derived from SCIM.


%package devel
Summary:    ISF header files
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.



%prep
%setup -q

%build

./bootstrap
%configure --disable-static \
		--disable-tray-icon --disable-filter-sctc
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}%{_datadir}/license
install -m0644 %{_builddir}/%{buildsubdir}/COPYING %{buildroot}%{_datadir}/license/%{name}

install -d %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
install -m0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/user/
ln -sf ../isf-panel.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/isf-panel.service

# FIXME: remove initscripts after systemd is in
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s /etc/init.d/isf-panel-efl %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S47isf-panel-efl
ln -s /etc/init.d/isf-panel-efl %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S81isf-panel-efl

%post 
/sbin/ldconfig
mkdir -p /etc/scim/conf

/usr/bin/vconftool set -t bool file/private/isf/autocapital_allow 1 -g 6514 || :
/usr/bin/vconftool set -t bool file/private/isf/autoperiod_allow 0 -g 6514 || :

%postun -p /sbin/ldconfig


%files
%manifest isf.manifest
%defattr(-,root,root,-)
%attr(755,root,root) %{_sysconfdir}/init.d/isf-panel-efl
%{_sysconfdir}/rc.d/rc3.d/S47isf-panel-efl
%{_sysconfdir}/rc.d/rc4.d/S81isf-panel-efl
%{_libdir}/systemd/user/core-efl.target.wants/isf-panel.service
%{_libdir}/systemd/user/isf-panel.service
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/*.edj
%{_datadir}/scim/icons/*
%{_datadir}/locale/*
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
%{_ugdir}/res/locale/*
%{_ugdir}/lib/libug-keyboard-setting-wizard-efl.so
%{_ugdir}/lib/libug-isfsetting-efl.so
%{_datadir}/license/%{name}

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc
