%define _optdir	/opt
%define _ugdir	%{_optdir}/ug

Name:       isf
Summary:    Input Service Framework
Version:    2.3.5205
Release:    1
Group:      TO_BE/FILLED_IN
License:    LGPL
Source0:    %{name}-%{version}.tar.gz
Source1:    isf-panel.service
Source1001: packaging/isf.manifest 
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ui-gadget)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig


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
cp %{SOURCE1001} .

./bootstrap
%configure --disable-static \
		--disable-tray-icon --disable-filter-sctc
make %{?_smp_mflags}

%install
%make_install

install -d %{buildroot}%{_libdir}/systemd/user
install -m0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/user/

# FIXME: remove initscripts after systemd is in
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc3.d
mkdir -p %{buildroot}%{_sysconfdir}/rc.d/rc4.d
ln -s ../init.d/isf-panel-efl %{buildroot}%{_sysconfdir}/rc.d/rc3.d/S42isf-panel-efl
ln -s ../init.d/isf-panel-efl %{buildroot}%{_sysconfdir}/rc.d/rc4.d/S81isf-panel-efl


%post 
/sbin/ldconfig

/usr/bin/vconftool set -t string db/isf/input_lang "Automatic" -g 6514
/usr/bin/vconftool set -t string db/setting/autocapital_allow 0 -g 6514

%postun -p /sbin/ldconfig


%files
%manifest isf.manifest
%attr(755,root,root) %{_sysconfdir}/init.d/isf-panel-efl
%{_sysconfdir}/rc.d/rc3.d/S42isf-panel-efl
%{_sysconfdir}/rc.d/rc4.d/S81isf-panel-efl
%{_libdir}/systemd/user/isf-panel.service
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isfsetting.edj
%{_datadir}/scim/isf_candidate_theme2.edj
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_datadir}/locale/*
%{_bindir}/isf-demo-efl
%{_bindir}/scim
%{_bindir}/isf-log
%{_bindir}/isf-panel-efl
%{_bindir}/isf-query-engines
%{_libdir}/ecore/immodules/libisf-imf-module.so
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/socket.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-1.0.so.*
%{_ugdir}/res/locale/*
%{_ugdir}/lib/libug-keyboard-setting-wizard-efl.so
%{_ugdir}/lib/libug-isfsetting-efl.so

%files devel
%manifest isf.manifest
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-1.0.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc
