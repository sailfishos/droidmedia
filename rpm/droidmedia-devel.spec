Name:          droidmedia-devel
Summary:       Android media wrapper library development package
Version:       0.20170214.0
Release:       1
License:       ASL 2.0
Source0:       %{name}-%{version}.tgz
BuildRequires: meson
BuildRequires: ninja
Suggests:      libhybris
Suggests:      droidmedia = %{version}-%{release}

%description
%{summary}

%prep
%setup -q

%build
%meson -Dversion=%{version}-%{release}
meson rewrite kwargs set project / version %{version}-%{release}
%meson_build

%install
%meson_install

%files
%defattr(-,root,root,-)
%{_libdir}/libdroidmedia.a
%{_includedir}/droidmedia/*.h
%{_datadir}/droidmedia/hybris.c
%{_libdir}/pkgconfig/droidmedia.pc
