Name:       ise-default
Summary:    Tizen keyboard
Version:    1.2.49
Release:    1
Group:      Graphics & UI Framework/Input
License:    Apache-2.0
Source0:    ise-default-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  gettext-tools
BuildRequires:  edje-bin
BuildRequires:  cmake
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(isf)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(libscl-ui)
BuildRequires:  pkgconfig(libscl-core)
BuildRequires:  pkgconfig(ecore-imf)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(efl-extension)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  model-build-features


%description
Description: Tizen keyboard



%prep
%setup -q


%build
export CFLAGS+=" -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS+=" -DTIZEN_DEBUG_ENABLE"
export FFLAGS+=" -DTIZEN_DEBUG_ENABLE"

%if "%{profile}" == "wearable"
CFLAGS+=" -D_WEARABLE";
CXXFLAGS+=" -D_WEARABLE";
%if "%{model_build_feature_formfactor}" == "circle"
CFLAGS+=" -D_CIRCLE";
CXXFLAGS+=" -D_CIRCLE";
%endif
%endif

%if "%{profile}" == "mobile"
CFLAGS+=" -D_MOBILE";
CXXFLAGS+=" -D_MOBILE";
%endif

%if "%{profile}" == "tv"
CFLAGS+=" -D_TV";
CXXFLAGS+=" -D_TV";
%endif

rm -rf CMakeFiles
rm -rf CMakeCache.txt
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} \
        -DTARGET=%{?profile} \
        -DTZ_SYS_RO_APP=%TZ_SYS_RO_APP \
        -DTZ_SYS_RO_PACKAGES=%TZ_SYS_RO_PACKAGES

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}

%make_install
%find_lang %{name}

%files -f %{name}.lang
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{TZ_SYS_RO_APP}/*
%{TZ_SYS_RO_PACKAGES}/%{name}.xml
%license LICENSE

