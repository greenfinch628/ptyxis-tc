%global debug_package %{nil}
%global tc_prefix /opt/ptyxis-tab-colors-50.1
Name: ptyxis-tc
Version: 50.2
Release: 5%{?dist}
Summary: Ptyxis with hostname tab colors and right-click clipboard shortcuts
License: GPL-3.0-or-later
URL: https://github.com/greenfinch628/ptyxis-tc
Source0: %{name}-%{version}.tar.gz
BuildRequires: gcc, meson, ninja-build, gettext-devel, desktop-file-utils
BuildRequires: pkgconfig(glib-2.0), pkgconfig(gtk4), pkgconfig(libadwaita-1)
BuildRequires: pkgconfig(json-glib-1.0), pkgconfig(vte-2.91-gtk4), pkgconfig(libportal-gtk4)
Requires: bash

%description
Custom Ptyxis with hostname tab colors, persistent manual colors and
right-click clipboard actions. Uses separate settings from Fedora Ptyxis.
Built from the accompanying Ptyxis-TC source archive.

%prep
%autosetup

%build
meson setup build-rpm --prefix=%{tc_prefix} --buildtype=debugoptimized -Dapp-id=org.gnome.Ptyxis.TabColors -Dgschema-id=org.gnome.Ptyxis.TabColors -Dgschema-path=/org/gnome/Ptyxis/TabColors/
meson compile -C build-rpm

%check
meson test -C build-rpm --print-errorlogs

%install
DESTDIR=%{buildroot} meson install -C build-rpm --no-rebuild
install -m755 custom/ptyxis-tab-colors %{buildroot}%{tc_prefix}/bin/ptyxis-tab-colors
mkdir -p %{buildroot}%{_bindir} %{buildroot}%{_datadir}/applications %{buildroot}%{_sysconfdir}/ptyxis-tc
install -m755 custom/ptyxis-tab-colors %{buildroot}%{_bindir}/ptyxis-tc
install -m755 custom/ptyxis-tab-colors %{buildroot}%{_bindir}/ptyxis-tab-colors
install -m644 custom/org.gnome.Ptyxis.TabColors.desktop %{buildroot}%{_datadir}/applications/
printf '[Hosts]\n\n[Aliases]\n' > %{buildroot}%{_sysconfdir}/ptyxis-tc/tab-colors.ini
custom/install-icons.sh %{buildroot}%{_datadir}
install -m644 custom/tab-colors.ini tab-colors.example.ini
# Cache files are regenerated after package installation.
rm -f %{buildroot}%{tc_prefix}/share/glib-2.0/schemas/gschemas.compiled
rm -f %{buildroot}%{tc_prefix}/share/icons/hicolor/icon-theme.cache
rm -f %{buildroot}%{tc_prefix}/share/applications/mimeinfo.cache

%post
/usr/bin/glib-compile-schemas %{tc_prefix}/share/glib-2.0/schemas || :

%files
%license COPYING
%doc custom/README.md tab-colors.example.ini
%config(noreplace) %{_sysconfdir}/ptyxis-tc/tab-colors.ini
%{tc_prefix}
%ghost %{tc_prefix}/share/glib-2.0/schemas/gschemas.compiled
%{_bindir}/ptyxis-tc
%{_bindir}/ptyxis-tab-colors
%{_datadir}/applications/org.gnome.Ptyxis.TabColors.desktop
%{_datadir}/icons/hicolor/scalable/apps/ptyxis-tc-*.svg
%{_datadir}/icons/hicolor/scalable/apps/org.gnome.Ptyxis.TabColors.svg
%{_datadir}/icons/hicolor/symbolic/apps/org.gnome.Ptyxis.TabColors-symbolic.svg

%changelog
* Thu Oct 08 2026 Bob Czys <greenfinch628@users.noreply.github.com> - 50.2-5
- Use a white active-tab underline and muted light-gray inactive colors.

* Wed Oct 07 2026 Bob Czys <greenfinch628@users.noreply.github.com> - 50.2-4
- Install named custom icons and use gray 3 as the default; rename launcher.

* Wed Oct 07 2026 Bob Czys <greenfinch628@users.noreply.github.com> - 50.2-3
- Add middle-click context menu in right-click clipboard mode; fix popover ownership.

* Tue Oct 06 2026 Bob Czys <greenfinch628@users.noreply.github.com> - 50.2-2
- Add persistent Preferences switch for right-click copy/paste.

* Mon Oct 05 2026 Bob Czys <scopuli@czysnet> - 50.2-1
- Rebuild merged custom and upstream history on Scopuli.
