# android.tgz is the Android root used for building droidmedia. Should be provided by the packager.
# build_droidmedia.sh is a shell script that:
# - builds droidmedia
# - moves out/<devicename>/target to target so it can be picked up

%define strip /bin/true
%define __requires_exclude  ^.*$
%define __find_requires     %{nil}
%global debug_package       %{nil}

# Use an ExportFilter: \.armv7hl\.rpm$ armv8el
# TODO: pick this up from OBS
%define _target_cpu armv7hl

Name:          droidmedia
Summary:       Android media wrapper library
Version:       0.0.0
Release:       1
Group:         System/Libraries
License:       TBD
BuildRequires: ubu-trusty
BuildRequires: sudo-for-abuild
Source0:       %{name}-%{version}.tgz
Source1:       android.tgz
Source2:       build_droidmedia.sh

%description
%{summary}

%package       devel
Summary:       droidmedia development headers
Group:         System/Libraries
Requires:      droidmedia = %{version}-%{release}

%description   devel
%{summary}

%prep

%setup -T -c -n droidmedia
tar -zxf %SOURCE1

mkdir -p android/external
pushd android/external
tar -zxf %SOURCE0
mv droidmedia* droidmedia
popd

pushd android
cp %SOURCE2 .
chmod +x build_droidmedia.sh
popd

%build
ubu-chroot -r /srv/mer/sdks/ubu "cd /parentroot/home/abuild/rpmbuild/BUILD/droidmedia/android/ && ./build_droidmedia.sh"

%install
pushd android

mkdir -p $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/lib/
mkdir -p $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/bin/
mkdir -p $RPM_BUILD_ROOT/%{_includedir}/droidmedia/
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/droidmedia/

cp target/product/msm8960/system/lib/libdroidmedia.so \
    $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/lib/

cp target/product/msm8960/system/bin/minimediaservice \
    $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/bin/

cp target/product/msm8960/system/bin/minisfservice \
    $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/bin/

cp android/external/droidmedia/*.h $RPM_BUILD_ROOT/%{_includedir}/droidmedia/
cp android/external/droidmedia/hybris.c $RPM_BUILD_ROOT/%{_datadir}/droidmedia/

popd

%files
%defattr(-,root,root,-)
%{_libexecdir}/droid-hybris/system/lib/libdroidmedia.so
%{_libexecdir}/droid-hybris/system/bin/minimediaservice
%{_libexecdir}/droid-hybris/system/bin/minisfservice

%files devel
%defattr(-,root,root,-)
%{_includedir}/droidmedia/*.h
%{_datadir}/droidmedia/hybris.c
