Name: harbour-unplayer
Summary: Simple music player for Sailfish OS
Version: 0.3.3
Release: 1
Group: Applications/Music
License: GPLv3
URL: https://github.com/equeim/unplayer
Source0: %{name}-%{version}.tar.xz
Requires: libqt5sparql-tracker-direct
Requires: sailfishsilica-qt5
BuildRequires: pkgconfig(mpris-qt5)
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sparql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(zlib)
BuildRequires: boost-devel
BuildRequires: cmake
BuildRequires: desktop-file-utils
BuildRequires: python

# >> macros
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libtag.*$
# << macros

%description
%{summary}

%prep
%setup -q -n %{name}-%{version}

%build
mkdir -p build-%{_arch}/taglib/build
mkdir -p build-%{_arch}/taglib/install

cd build-%{_arch}/taglib/build
cmake "%{_builddir}/3rdparty/taglib" \
    -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DWITH_MP4=ON
VERBOSE=1 %{__make} %{?_smp_mflags}
%{__make} install
cd -

python waf configure --prefix=/usr --out=build-%{_arch}
python waf build -v

%install
rm -rf %{buildroot}

mkdir -p %{buildroot}%{_datadir}/%{name}/lib
cp -d build-%{_arch}/taglib/install/lib/libtag.so* %{buildroot}%{_datadir}/%{name}/lib

python waf install --destdir=%{buildroot} -v
desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
