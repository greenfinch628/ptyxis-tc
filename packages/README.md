# Ptyxis-TC packages

Current release: 50.2, package revision 5, built from source commit
066489c. Fedora 44 x86_64 RPM and Ubuntu 26.04 amd64 DEB are built on
their respective distributions and locally unsigned.

    sudo dnf install ./packages/fedora-44/ptyxis-tc-50.2-5.fc44.x86_64.rpm
    sudo apt install ./packages/ubuntu-26.04/ptyxis-tc_50.2-5ubuntu26.04.1_amd64.deb

Use only the command for your OS. Close all Ptyxis-TC windows and reopen
after upgrading. Revision 5 includes the 3-pixel white active underline,
60% light-gray inactive-color blend, and all previously committed features.

The source RPM and complete source archive are under source/, alongside the
Debian .dsc, .debian.tar.xz, and .orig.tar.gz needed to rebuild the DEB.
See ../packaging/README.md for rebuild instructions. Older DEBs are retained
for history; revision 5 is the current package. Verify current artifacts with:

    cd packages
    sha256sum -c SHA256SUMS

Meson build and all four checks passed in both environments. Custom tab-color
integration tests passed on Fedora. RPM upgrade dry-run and Ubuntu package
installation smoke checks are performed before publishing.
