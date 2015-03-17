%bcond_with wayland
Name:       isf
Summary:    Input Service Framework
Version:    2.4.8513
Release:    1
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
BuildRequires:  pkgconfig(vconf)
%if %{with wayland}
BuildRequires:  pkgconfig(ecore-wayland)
BuildRequires:  pkgconfig(xkbcommon) >= 0.3.0
%else
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(x11)
%endif
%if "%{profile}" == "mobile"
BuildRequires:  pkgconfig(minicontrol-provider)
%endif
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(notification)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(tts)
BuildRequires:  pkgconfig(edbus)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(feedback)
BuildRequires:  efl-extension-devel
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(pkgmgr-info)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  capi-appfw-package-manager-devel
Requires(post): /sbin/ldconfig /usr/bin/vconftool
Requires(postun): /sbin/ldconfig

%define _optexecdir /opt/usr/devel/usr/bin/

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

CFLAGS+=" -fvisibility=hidden ${GC_SECTIONS_FLAGS} "; export CFLAGS

CXXFLAGS+=" -fvisibility=hidden -fvisibility-inlines-hidden ${GC_SECTIONS_FLAGS} ";export CXXFLAGS

%autogen
%configure --disable-static \
		--disable-tray-icon \
		--disable-filter-sctc \
%if %{with wayland}
        --disable-panel-efl \
        --disable-efl-immodule \
%else
        --disable-wsm-efl \
        --disable-wsc-efl \
%endif
		--disable-frontend-x11 \
		--disable-multiwindow-support
make %{?_smp_mflags}

%install
rm -rf %{buildroot}

%make_install
mkdir -p %{buildroot}/opt/etc/dump.d/module.d
cp -af ism/dump/isf_log_dump.sh %{buildroot}/opt/etc/dump.d/module.d
mkdir -p %{buildroot}/opt/usr/dbspace
if [ ! -s %{buildroot}/opt/usr/dbspace/.ime_info.db ]; then
echo "The database file for ime will be created."
sqlite3 %{buildroot}/opt/usr/dbspace/.ime_info.db <<EOF
CREATE TABLE ime_info (appid TEXT PRIMARY KEY NOT NULL, label TEXT, languages TEXT, iconpath TEXT, pkgid TEXT, pkgtype TEXT, exec TEXT, mname TEXT, mpath TEXT, mode INTEGER, options INTEGER, enabled INTEGER, preinstalled INTEGER);
EOF
fi

mkdir -p %{buildroot}/etc/scim/conf
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/Helper
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
mkdir -p %{buildroot}/opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine

%find_lang scim

cat scim.lang > isf.lang
%post
/sbin/ldconfig


/usr/bin/vconftool set -t bool file/private/isf/autocapital_allow 1 -s User || :
/usr/bin/vconftool set -t bool file/private/isf/autoperiod_allow 0 -s User || :
/usr/bin/vconftool set -t string db/isf/input_language "en_US" -s User || :
/usr/bin/vconftool set -t string db/isf/csc_initial_uuid "" -s User || :
/usr/bin/vconftool set -t string db/isf/input_keyboard_uuid "isf-default" -s User || :
/usr/bin/vconftool set -t int memory/isf/input_panel_state 0 -s User -i || :

%postun -p /sbin/ldconfig


%files -f isf.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/Helper
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/SetupUI
%dir /opt/apps/scim/lib/scim-1.0/1.4.0/IMEngine
%dir /etc/scim/conf
%{_prefix}/lib/systemd/user/default.target.wants/scim.service
%{_prefix}/lib/systemd/user/scim.service
%attr(755,root,root) %{_sysconfdir}/profile.d/isf.sh
%{_sysconfdir}/scim/global
%{_sysconfdir}/scim/config
%{_datadir}/scim/isf_candidate_theme1.edj
%{_datadir}/scim/icons/*
%{_optexecdir}/isf-demo-efl
%if %{with wayland}
%{_bindir}/isf-wsm-efl
%{_bindir}/isf-wsc-efl
%else
%{_bindir}/isf-panel-efl
%{_libdir}/ecore_imf/modules/*/*/*.so
%endif
%{_bindir}/scim
%{_bindir}/isf-log
%{_libdir}/scim-1.0/1.4.0/IMEngine/socket.so
%{_libdir}/scim-1.0/1.4.0/Config/simple.so
%{_libdir}/scim-1.0/1.4.0/Config/socket.so
%{_libdir}/scim-1.0/1.4.0/FrontEnd/*.so
%{_libdir}/scim-1.0/scim-launcher
%{_libdir}/scim-1.0/scim-helper-launcher
%{_libdir}/libscim-*.so*
%license COPYING
/opt/etc/dump.d/module.d/*
%attr(660,root,app) /opt/usr/dbspace/.ime_info.db*

%files devel
%defattr(-,root,root,-)
%{_includedir}/scim-1.0/*
%{_libdir}/libscim-*.so
%{_libdir}/pkgconfig/isf.pc
%{_libdir}/pkgconfig/scim.pc
