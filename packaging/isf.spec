%bcond_with wayland
Name:       isf
Summary:    Input Service Framework
Version:    3.0.93
Release:    1
Group:      Graphics & UI Framework/Input
License:    LGPL-2.1+
Source0:    %{name}-%{version}.tar.gz
BuildRequires:  edje-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(vconf)
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
BuildRequires:  pkgconfig(text-client)
BuildRequires:  pkgconfig(input-method-client)
%else
BuildRequires:  pkgconfig(ecore-x)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(tts)
%endif
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  efl-extension-devel
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(capi-appfw-app-control)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(cynara-client)
BuildRequires:  pkgconfig(cynara-creds-socket)
BuildRequires:  pkgconfig(cynara-session)
BuildRequires:  capi-appfw-package-manager-devel
Requires(postun): /sbin/ldconfig
%if "%{?profile}" == "mobile"
BuildRequires:  pkgconfig(notification)
Requires: org.tizen.isf-kbd-mode-changer
%endif

%define APP_PREFIX %{TZ_SYS_RO_APP}/org.tizen.isf-kbd-mode-changer/bin/

%description
Input Service Framewok (ISF) is an input method (IM) platform, and it has been derived from SCIM.


%package devel
Summary:    ISF header files
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.

%package -n org.tizen.isf-kbd-mode-changer
Summary: isf-kbd-mode-changer
Group: Application
Requires: %{name} = %{version}-%{release}

%description -n org.tizen.isf-kbd-mode-changer
isf-kbd-mode-changer

%prep
%setup -q

%build
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"

%if "%{profile}" == "wearable"
CFLAGS+=" -D_WEARABLE";
CXXFLAGS+=" -D_WEARABLE";
%endif

%if "%{profile}" == "mobile"
CFLAGS+=" -D_MOBILE";
CXXFLAGS+=" -D_MOBILE";
%endif

%if "%{profile}" == "tv"
CFLAGS+=" -D_TV";
CXXFLAGS+=" -D_TV";
%endif

%if %{with wayland}
CFLAGS+=" -DWAYLAND"
CXXFLAGS+=" -DWAYLAND"
%endif

export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"

CFLAGS+=" ${GC_SECTIONS_FLAGS} "; export CFLAGS

CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ${GC_SECTIONS_FLAGS} ";export CXXFLAGS

%autogen
%configure --disable-static \
		--disable-tray-icon \
		--disable-filter-sctc \
%if %{with wayland}
        --disable-efl-immodule \
%endif
		--disable-frontend-x11 \
		--disable-multiwindow-support \
		--disable-ime-embed-app \
		--with-ro-app-dir=%{TZ_SYS_RO_APP} \
		--with-ro-packages-dir=%{TZ_SYS_RO_PACKAGES}
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/%{TZ_SYS_ETC}/dump.d/module.d
cp -af ism/dump/isf_log_dump.sh %{buildroot}/%{TZ_SYS_ETC}/dump.d/module.d
mkdir -p %{buildroot}/etc/scim/conf
%find_lang scim

cat scim.lang > isf.lang
%post
ln -sf %{_libdir}/ecore_imf/modules/wayland/v-1.16/module.so %{_libdir}/ecore_imf/modules/wayland/v-1.16/libwltextinputmodule.so
/sbin/ldconfig


%postun -p /sbin/ldconfig


%files -f isf.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir /etc/scim/conf
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_bindir}/isf-demo-efl
%{_bindir}/isf-panel-efl
%{_bindir}/scim
%{_bindir}/isf-log
%{_libdir}/ecore_imf/modules/*/*/*.so
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/PanelAgent/*.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/*.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-*.so*
%license COPYING
%{TZ_SYS_ETC}/dump.d/module.d/*

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc

%post -n org.tizen.isf-kbd-mode-changer
mkdir -p %{TZ_SYS_RO_APP}/org.tizen.isf-kbd-mode-changer

%files -n org.tizen.isf-kbd-mode-changer
%manifest org.tizen.isf-kbd-mode-changer.manifest
%{TZ_SYS_RO_PACKAGES}/org.tizen.isf-kbd-mode-changer.xml
%{APP_PREFIX}/*
