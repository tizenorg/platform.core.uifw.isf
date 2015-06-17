%bcond_with wayland

Name:       ise-default
Summary:    Tizen keyboard
Version:    1.1.0
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



%description
Description: Tizen keyboard



%prep
%setup -q


%build
rm -rf CMakeFiles
rm -rf CMakeCache.txt

export CFLAGS+=" -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS+=" -DTIZEN_DEBUG_ENABLE"
export FFLAGS+=" -DTIZEN_DEBUG_ENABLE"

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
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DLIB_INSTALL_DIR:PATH=%{_libdir} -Dwith_wayland=TRUE
%else
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DLIB_INSTALL_DIR:PATH=%{_libdir}
%endif

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
cp LICENSE %{buildroot}/usr/share/license/%{name}

%make_install

%post

%postun

%files
%defattr(-,root,root,-)
%{_libdir}/scim-1.0/1.4.0/Helper/ise-default.so
%{_datadir}/isf/ise/ise-default/*
%{_datadir}/packages/*
%{_datadir}/locale/*
/usr/share/license/%{name}

