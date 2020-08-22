Name:       harbour-unplayer
Version:    2.0.4
Release:    1
Summary:    Simple music player for Sailfish OS
Group:      Applications/Music
License:    GPLv3
URL:        https://github.com/equeim/unplayer

Source0:    https://github.com/equeim/unplayer/archive/%{version}.tar.gz
Patch0:     qtdbusextended.patch
Patch1:     qtmpris.patch

Requires:      sailfishsilica-qt5
BuildRequires: pkgconfig(Qt5Concurrent)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Multimedia)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: cmake
BuildRequires: desktop-file-utils

# TagLib dependencies
BuildRequires: pkgconfig(zlib)
BuildRequires: boost-devel

%define __provides_exclude mimehandler

%global build_type debug
#%%global build_type release

%global harbour ON
#%%global harbour OFF

%global build_directory "%{_builddir}/build-%{_target}-$(%{__grep} VERSION_ID /etc/os-release | cut -d '=' -f 2)"

%global qtdbusextended "%{_builddir}/3rdparty/qtdbusextended"
%global qtdbusextended_build "%{build_directory}/3rdparty/build-qtdbusextended"

%global qtmpris "%{_builddir}/3rdparty/qtmpris"
%global qtmpris_build "%{build_directory}/3rdparty/build-qtmpris"

%global taglib "%{_builddir}/3rdparty/taglib"
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


%build
export PKG_CONFIG_PATH=%{thirdparty_install}/lib/pkgconfig

# Enable -O0 for debug builds
# This also requires disabling _FORTIFY_SOURCE
%if "%{build_type}" == "debug"
    export CFLAGS="$CFLAGS -O0 -Wp,-U_FORTIFY_SOURCE"
    export CXXFLAGS="$CXXFLAGS -O0 -Wp,-U_FORTIFY_SOURCE"
%endif

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
CFLAGS="$CFLAGS -fPIC" CXXFLAGS="$CXXFLAGS -fPIC" %cmake %{taglib} \
    -DCMAKE_INSTALL_PREFIX=%{thirdparty_install} \
    -DLIB_INSTALL_DIR=%{thirdparty_install}/lib \
    -DINCLUDE_INSTALL_DIR=%{thirdparty_install}/include \
    -DCMAKE_BUILD_TYPE=%{build_type} \
    -DBUILD_SHARED_LIBS=OFF \
    -DWITH_MP4=ON
%make_build
# not make_install, because we do not want DESTDIR here
%{__make} install
cd -

cd %{build_directory}
%cmake .. \
    -DCMAKE_BUILD_TYPE=%{build_type} \
    -DHARBOUR=%{harbour} \
    -DQTMPRIS_STATIC=ON \
    -DTAGLIB_STATIC=ON
%make_build

%install
cd %{build_directory}
%make_install
desktop-file-install \
    --delete-original \
    --dir %{buildroot}/%{_datadir}/applications \
    %{buildroot}/%{_datadir}/applications/%{name}.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
