# Local package builds

The tracked RPM spec is `packaging/rpm/ptyxis-tc.spec`; Debian packaging is
in `debian/`. Both retain the private /opt prefix and TabColors settings.
Meson installs all custom icons in the private hicolor theme. Both recipes
also call `custom/install-icons.sh` to install named custom variants and
upstream full-color/symbolic icons in the public /usr/share hicolor theme.
The public launcher uses `ptyxis-tc-blue-border-gray3` by name.

## Fedora RPM

From a clean, committed checkout with the documented Fedora dependencies:

```sh
mkdir -p local-rpm/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}
git --no-pager archive --prefix=ptyxis-tc-50.2/ HEAD | gzip -n > local-rpm/SOURCES/ptyxis-tc-50.2.tar.gz
rpmbuild -ba --define "_topdir $PWD/local-rpm" packaging/rpm/ptyxis-tc.spec
```

The ignored `local-rpm/SPECS/ptyxis-tc.spec` is a local copy; the tracked spec
is authoritative. Update package release numbers when creating new releases.

## Ubuntu 26.04 amd64 DEB

Build inside Ubuntu 26.04, including when your host runs Fedora. Export a
clean source checkout and prepare an upstream archive without `debian/`:

```sh
mkdir -p /tmp/ptyxis-tc-deb-build
cd /tmp/ptyxis-tc-deb-build
git --no-pager -C ~/ptyxis-tc archive --prefix=ptyxis-tc-50.2/ HEAD | tar -xf -
git --no-pager -C ~/ptyxis-tc archive --prefix=ptyxis-tc-50.2/ HEAD -- . ':!debian' | gzip -n > ptyxis-tc_50.2.orig.tar.gz
```

In an Ubuntu 26.04 environment, install `build-essential`, `devscripts` and
`equivs`, then install the declared build dependencies and build:

```sh
cd ptyxis-tc-50.2
sudo mk-build-deps -i -r debian/control
dpkg-buildpackage -us -uc
```

The resulting package is local and unsigned. Both package builds run the
Meson checks. Inspect the completed RPM/DEB contents to confirm that every
`data/icons/ptyxis-tc-*.svg` is present in both hicolor scalable/apps trees,
that the upstream symbolic icon remains, and that the public desktop entry
has `Name=ptyxis-tc` and `Icon=ptyxis-tc-blue-border-gray3`.
