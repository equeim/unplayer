Name:       harbour-unplayer
Version:    2.1.1
Release:    1
Summary:    Simple music player for Sailfish OS
Group:      Applications/Music
License:    GPLv3
URL:        https://github.com/equeim/unplayer

Source0:    https://github.com/equeim/unplayer/archive/%{version}.tar.gz
Patch0:     qtmpris.patch
Patch1:     taglib.patch

Requires:      sailfishsilica-qt5
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: pkgconfig(nemonotifications-qt5)
BuildRequires: cmake
BuildRequires: desktop-file-utils

# TagLib dependencies
BuildRequires: pkgconfig(zlib)
BuildRequires: boost-devel

%define __provides_exclude mimehandler

%global debug 0

%global harbour ON
#%%global harbour OFF

%global build_directory %{_builddir}/build-%{_target}-%(version | awk '{print $3}')

%global qtmpris_source_directory %{_builddir}/3rdparty/qtmpris
%global qtmpris_build_directory %{build_directory}/3rdparty/qtmpris

%global taglib_source_directory %{_builddir}/3rdparty/taglib
%global taglib_build_directory %{build_directory}/3rdparty/taglib

%global thirdparty_install_directory %{build_directory}/3rdparty/install

%define patch_if_needed() \
%if 0%(patch -p0 -R --dry-run --force --fuzz=2 --input=%{P:%1} > /dev/null 2>&1; echo $?) != 0 \
%patch%1 \
%endif

%global apply_patches %{lua: \
for i, s in ipairs(patches) do \
    print(rpm.expand("%patch_if_needed "..(i - 1))) \
end \
}


%description
%{summary}


%prep
%setup -q
%apply_patches

%build
export PKG_CONFIG_PATH=%{thirdparty_install_directory}/lib/pkgconfig

# Enable -O0 for debug builds
# This also requires disabling _FORTIFY_SOURCE
%if %{debug}
    export CFLAGS="${CFLAGS:-%optflags} -O0 -Wp,-U_FORTIFY_SOURCE"
    export CXXFLAGS="${CXXFLAGS:-%optflags} -O0 -Wp,-U_FORTIFY_SOURCE"
%endif

mkdir -p %{qtmpris_build_directory}
cd %{qtmpris_build_directory}
%qmake5 %{qtmpris_source_directory} CONFIG+=staticlib QMAKE_CXXFLAGS+="-std=c++17" PREFIX=%{thirdparty_install_directory}
%make_build
# not make_install, because we do not want INSTALL_ROOT here
make install

mkdir -p %{taglib_build_directory}
cd %{taglib_build_directory}
CFLAGS="$CFLAGS -fPIC" CXXFLAGS="$CXXFLAGS -fPIC" %cmake %{taglib_source_directory} \
    -DCMAKE_INSTALL_PREFIX=%{thirdparty_install_directory} \
    -DLIB_INSTALL_DIR=%{thirdparty_install_directory}/lib \
    -DINCLUDE_INSTALL_DIR=%{thirdparty_install_directory}/include \
    -DBUILD_SHARED_LIBS=OFF \
    -DWITH_MP4=ON
%make_build
# not make_install, because we do not want DESTDIR here
make install

cd %{build_directory}
%cmake .. \
    -DHARBOUR=%{harbour} \
    -DQTMPRIS_STATIC=ON \
    -DTAGLIB_STATIC=ON
%make_build

%install
cd %{build_directory}
%make_install
desktop-file-validate %{buildroot}/%{_datadir}/applications/%{name}.desktop


%files
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.*
