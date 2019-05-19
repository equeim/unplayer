Name:       harbour-unplayer
Version:    1.4.1
Release:    1
Summary:    Simple music player for Sailfish OS
Group:      Applications/Music
License:    GPLv3
URL:        https://github.com/equeim/unplayer

Source0:    https://github.com/equeim/unplayer/archive/%{version}.tar.gz
Patch0:     qtdbusextended.patch
Patch1:     qtmpris.patch
Patch2:     taglib.patch

Requires:      sailfishsilica-qt5
Requires:      nemo-qml-plugin-dbus-qt5
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: cmake
BuildRequires: desktop-file-utils

# qt5-qtmultimedia-devel installs plugins' CMake modules so we need plugins at build time, otherwise CMake will fail
# https://bugs.merproject.org/show_bug.cgi?id=1943
BuildRequires: qt5-qtmultimedia-plugin-audio-alsa
BuildRequires: qt5-qtmultimedia-plugin-audio-pulseaudio
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstaudiodecoder
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstcamerabin
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstmediacapture
BuildRequires: qt5-qtmultimedia-plugin-mediaservice-gstmediaplayer
BuildRequires: qt5-qtmultimedia-plugin-playlistformats-m3u
BuildRequires: qt5-qtmultimedia-plugin-resourcepolicy-resourceqt

# TagLib dependencies
BuildRequires: pkgconfig(zlib)
BuildRequires: boost-devel

BuildRequires: ninja

%global build_type debug
#%%global build_type release

%global harbour ON
#%%global harbour OFF

%global build_directory "%{_builddir}/build-%{_arch}"

%global qtdbusextended "%{_builddir}/3rdparty/qtdbusextended-0.0.3"
%global qtdbusextended_build "%{build_directory}/3rdparty/build-qtdbusextended"

%global qtmpris "%{_builddir}/3rdparty/qtmpris-1.0.0"
%global qtmpris_build "%{build_directory}/3rdparty/build-qtmpris"

%global taglib "%{_builddir}/3rdparty/taglib-79bb14"
%global taglib_build "%{build_directory}/3rdparty/build-taglib"

%global thirdparty_install "%{build_directory}/3rdparty/install"


%description
%{summary}


%prep
%setup -q
# patch if not patched
if ! %{__patch} -p0 -R --dry-run -f -i "%{P:0}"; then
%patch0
fi
if ! %{__patch} -p0 -R --dry-run -f -i "%{P:1}"; then
%patch1
fi
if ! %{__patch} -p0 -R --dry-run -f -i "%{P:2}"; then
%patch2
fi


%build
export PKG_CONFIG_PATH=%{thirdparty_install}/lib/pkgconfig

if [ ! -d %{thirdparty_install} ] || [ -z "$(ls -A %{thirdparty_install})" ]; then
    %{__mkdir_p} %{qtdbusextended_build}
    cd %{qtdbusextended_build}
    %qmake5 %{qtdbusextended} CONFIG+='%{build_type} staticlib' PREFIX=%{thirdparty_install}
    %make_build
    # not make_install, because we do not want DESTDIR here
    %{__make} install
    cd -

    %{__mkdir_p} %{qtmpris_build}
    cd %{qtmpris_build}
    %qmake5 %{qtmpris} CONFIG+='%{build_type} staticlib' PREFIX=%{thirdparty_install}
    %make_build
    # not make_install, because we do not want DESTDIR here
    %{__make} install
    cd -

    %{__mkdir_p} %{taglib_build}
    cd %{taglib_build}
    %cmake %{taglib} \
        -G Ninja \
        -DCMAKE_INSTALL_PREFIX=%{thirdparty_install} \
        -DLIB_INSTALL_DIR=%{thirdparty_install}/lib \
        -DINCLUDE_INSTALL_DIR=%{thirdparty_install}/include \
        -DCMAKE_BUILD_TYPE=%{build_type} \
        -DCMAKE_CXX_FLAGS="-fPIC" \
        -DBUILD_SHARED_LIBS=OFF \
        -DWITH_MP4=ON
    %ninja_build
    # not ninja_install, because we do not want DESTDIR here
    %{__ninja} install %{__ninja_common_opts}
    cd -
fi

cd %{build_directory}
%cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=%{build_type} \
    -DHARBOUR=%{harbour} \
    -DQTMPRIS_STATIC=ON \
    -DTAGLIB_STATIC=ON
%ninja_build

%install
cd %{build_directory}
%ninja_install
desktop-file-install \
    --delete-original \
    --dir %{buildroot}/%{_datadir}/applications \
    %{buildroot}/%{_datadir}/applications/%{name}.desktop

#install -m 644 -D %{_libdir}/libstdc++.so.6 %{buildroot}/%{_datadir}/%{name}/lib/libstdc++.so.6

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
