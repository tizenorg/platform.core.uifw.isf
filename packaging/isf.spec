Name:       isf
Summary:    Input Service Framework
Version:    2.4.7424
Release:    2
Group:      Graphics & UI Framework/Input
License:    LGPL-2.1
Source0:    %{name}-%{version}.tar.gz
%if "%{_repository}" != "wearable"
Source1:    scim.service
%endif
Source1001: isf.manifest
BuildRequires:  edje-bin
BuildRequires:  embryo-bin
BuildRequires:  gettext-tools
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(libprivilege-control)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
%if "%{_repository}" != "wearable"
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(minicontrol-provider)
%endif
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tts)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  efl-assist-devel
BuildRequires:  pkgconfig(ail)
BuildRequires:  pkgconfig(libtzplatform-config)
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig

%description
Input Service Framewok (ISF) is an input method (IM) platform, and it has been derived from SCIM.


%package devel
Summary:    ISF header files
Requires:   %{name} = %{version}-%{release}

%description devel
This package contains ISF header files for ISE development.

%prep
%setup -q
cp %{SOURCE1001} .

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if "%{_repository}" == "wearable"
CFLAGS+=" -D_WEARABLE";
CXXFLAGS+=" -D_WEARABLE";
%else
CFLAGS+=" -D_MOBILE";
CXXFLAGS+=" -D_MOBILE";
%endif

export GC_SECTIONS_FLAGS="-fdata-sections -ffunction-sections -Wl,--gc-sections"
CFLAGS+=" -fvisibility=hidden ${GC_SECTIONS_FLAGS} "; export CFLAGS
CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ${GC_SECTIONS_FLAGS} ";export CXXFLAGS

%autogen
%configure --disable-static \
		--disable-tray-icon \
		--disable-filter-sctc \
		--disable-frontend-x11
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/etc/scim/conf
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/Helper
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine

%if "%{_repository}" != "wearable"
install -d %{buildroot}%{_libdir}/systemd/system/graphical.target.wants
install -d %{buildroot}%{_libdir}/systemd/system
install -m0644 %{SOURCE1} %{buildroot}%{_libdir}/systemd/system/
ln -sf ../../system/scim.service %{buildroot}%{_libdir}/systemd/system/graphical.target.wants/scim.service
%endif

%find_lang scim

cat scim.lang > isf.lang
%post
/sbin/ldconfig


/usr/bin/vconftool set -t bool file/private/isf/autocapital_allow 1 -s system::vconf_inhouse -g 6514 || :
/usr/bin/vconftool set -t bool file/private/isf/autoperiod_allow 0 -s system::vconf_inhouse -g 6514 || :
/usr/bin/vconftool set -t string db/isf/input_language "en_US" -s system::vconf_misc -g 5000 || :
/usr/bin/vconftool set -t int memory/isf/input_panel_state 0 -s system::vconf_inhouse -i -g 5000 || :

%postun -p /sbin/ldconfig


%files -f isf.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/Helper
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine
%if "%{_repository}" == "wearable"
%dir /etc/scim/conf
%{_libdir}/systemd/user/core-efl.target.wants/scim.service
%{_libdir}/systemd/user/scim.service
%else
%{_libdir}/systemd/system/graphical.target.wants/scim.service
%{_libdir}/systemd/system/scim.service
%endif
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
