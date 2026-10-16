# Ptyxis-TC packages

Latest Fedora 44 x86_64 package: **50.2-7**, including the tab-color and master-reset
updates. Installed and verified on Scopuli, Rocinante, Karakum, and Narlikar.
See [deployment validation](DEPLOYMENT-50.2-7.md).

```sh
sudo dnf install ./packages/fedora-44/ptyxis-tc-50.2-7.fc44.x86_64.rpm
```

Ubuntu 26.04 amd64 remains at package revision 6 (the prior baseline, without
these new features):

```sh
sudo apt install ./packages/ubuntu-26.04/ptyxis-tc_50.2-6ubuntu26.04.1_amd64.deb
```

Packages are unsigned local builds. Close and reopen Ptyxis-TC after upgrading.
Matching source archives and source RPMs are under source/. Previous packages
are retained. Verify with `sha256sum -c SHA256SUMS` from this directory.
Build recipes are in ../packaging/ and ../debian/.
