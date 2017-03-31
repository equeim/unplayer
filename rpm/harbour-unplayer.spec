Name: harbour-unplayer
Summary: Simple music player for Sailfish OS
Version: 0.3.3
Release: 1
Group: Applications/Music
License: GPLv3
URL: https://github.com/equeim/unplayer
Source0: %{name}-%{version}.tar.xz
Requires: sailfishsilica-qt5
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(zlib)
BuildRequires: boost-devel
BuildRequires: cmake
BuildRequires: desktop-file-utils
BuildRequires: python

# >> macros
%define __provides_exclude_from ^%{_datadir}/.*$
%define __requires_exclude ^libdbusextended-qt5.*|libmpris-qt5.*|libtag.*$
# << macros

%description
%{summary}

%prep
%setup -q -n %{name}-%{version}

%build
build_directory="%{_builddir}/build-%{_arch}"

qtdbusextended_build="${build_directory}/3rdparty/qtdbusextended/build"
qtdbusextended_install="${build_directory}/3rdparty/qtdbusextended/install"
mkdir -p "${qtdbusextended_build}"
mkdir -p "${qtdbusextended_install}"

cd "${qtdbusextended_build}"
%qmake5 "%{_builddir}/3rdparty/qtdbusextended-0.0.3" \
    CONFIG+=release
%{__make} %{?_smp_mflags}
%{__make} INSTALL_ROOT="${qtdbusextended_install}" install
cd -

qtmpris_build="${build_directory}/3rdparty/qtmpris/build"
qtmpris_install="${build_directory}/3rdparty/qtmpris/install"
mkdir -p "${qtmpris_build}"
mkdir -p "${qtmpris_install}"

cd "${qtmpris_build}"
%qmake5 "%{_builddir}/3rdparty/qtmpris-0.0.8" \
    CONFIG+=release \
    DBUSEXTENDED_INCLUDEPATH="${qtdbusextended_install}/usr/include/qt5/DBusExtended" \
    DBUSEXTENDED_LIBPATH="${qtdbusextended_install}/usr/lib"
%{__make} %{?_smp_mflags}
%{__make} INSTALL_ROOT="${qtmpris_install}" install
cd -


taglib_build="${build_directory}/3rdparty/taglib/build"
taglib_install="${build_directory}/3rdparty/taglib/install"
mkdir -p "${taglib_build}"
mkdir -p "${taglib_install}"

cd "${taglib_build}"
cmake "%{_builddir}/3rdparty/taglib-1.11.1" \
    -DCMAKE_INSTALL_PREFIX="${taglib_install}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DWITH_MP4=ON
VERBOSE=1 %{__make} %{?_smp_mflags}
%{__make} install
cd -

python waf configure --prefix="%{_prefix}" \
    --out="${build_directory}" \
    --taglib-includepath="${taglib_install}/include/taglib" \
    --taglib-libpath="${taglib_install}/lib" \
    --qtmpris-includepath="${qtmpris_install}/usr/include/qt5/MprisQt" \
    --qtmpris-libpath="${qtmpris_install}/usr/lib"
python waf build -v

%install
rm -rf "%{buildroot}"

lib_dir="%{buildroot}%{_datadir}/%{name}/lib"
build_directory="%{_builddir}/build-%{_arch}"
mkdir -p "${lib_dir}"
cp -d "${build_directory}/3rdparty/qtdbusextended/install/usr/lib"/libdbusextended-qt5.so* "${lib_dir}"
cp -d "${build_directory}/3rdparty/qtmpris/install/usr/lib"/libmpris-qt5.so* "${lib_dir}"
cp -d "${build_directory}/3rdparty/taglib/install/lib"/libtag.so* "${lib_dir}"

python waf install --destdir="%{buildroot}"

desktop-file-install --delete-original \
    --dir "%{buildroot}%{_datadir}/applications" \
    "%{buildroot}%{_datadir}/applications"/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
