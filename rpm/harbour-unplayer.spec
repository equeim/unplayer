Name: harbour-unplayer
Summary: Simple music player for Sailfish OS
Version: 0.3.3
Release: 1
Group: Applications/Music
License: GPLv3
URL: https://github.com/equeim/unplayer
Source0: %{name}-%{version}.tar.xz
Requires: nemo-qml-plugin-policy-qt5
Requires: sailfishsilica-qt5
BuildRequires: pkgconfig(audioresource-qt)
BuildRequires: pkgconfig(gstreamer-1.0)
BuildRequires: pkgconfig(mpris-qt5)
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(Qt5Quick)
BuildRequires: pkgconfig(Qt5Sparql)
BuildRequires: pkgconfig(sailfishapp)
BuildRequires: desktop-file-utils
BuildRequires: python

%description
%{summary}

%prep
%setup -q -n %{name}-%{version}

%build
./waf configure build --prefix=/usr --out=build-%{_arch}

%install
rm -rf %{buildroot}
./waf install --destdir=%{buildroot}
desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
