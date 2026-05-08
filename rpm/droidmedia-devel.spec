Name:          droidmedia-devel
Summary:       Android media wrapper library development package
Version:       0.20170214.0
Release:       1
License:       ASL 2.0
Source0:       %{name}-%{version}.tgz
BuildRequires: meson
BuildRequires: ninja
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(libresourceqt5) >= 1.29
Suggests:      libhybris
Suggests:      droidmedia = %{version}-%{release}

%description
%{summary}

%package -n droidmedia-flashlight
Summary:       Android-backed flashlight D-Bus service
Requires:      droidmedia
Provides:      sailfish-flashlight-provider

%description -n droidmedia-flashlight
Android-backed implementation of the Jolla settings flashlight D-Bus service.

%prep
%setup -q

%build
%meson
meson rewrite kwargs set project / version %{version}
%meson_build

%install
%meson_install

%files
%{_libdir}/libdroidmedia.a
%{_includedir}/droidmedia/*.h
%{_datadir}/droidmedia/hybris.c
%{_libdir}/pkgconfig/droidmedia.pc

%files -n droidmedia-flashlight
%{_bindir}/droidmedia-flashlight
%{_datadir}/dbus-1/services/org.sailfish.flashlight.provider.service
