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
    -DCMAKE_CXX_FLAGS="-fPIC" \
    -DBUILD_SHARED_LIBS=OFF \
    -DWITH_MP4=ON
%{__make} %{?_smp_mflags}
%{__make} install
cd -

./waf configure --prefix=/usr \
    --out=build-%{_arch} \
    --taglib-includes="%{_builddir}/build-%{_arch}/taglib/install/include/taglib" \
    --taglib-libpath="%{_builddir}/build-%{_arch}/taglib/install/lib"
./waf build -v

%install
rm -rf %{buildroot}
python waf install --destdir=%{buildroot}
desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
