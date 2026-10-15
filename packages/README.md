# Ptyxis-TC packages

Current baseline: app 50.2, package revision 6. Fedora 44 x86_64 and Ubuntu
26.04 amd64 packages are unsigned local builds. Revision 6 rebuilds the same
application code as revision 5 with the new change-tracking documentation.

    sudo dnf install ./packages/fedora-44/ptyxis-tc-50.2-6.fc44.x86_64.rpm
    sudo apt install ./packages/ubuntu-26.04/ptyxis-tc_50.2-6ubuntu26.04.1_amd64.deb

Use the command matching your OS. Close and reopen Ptyxis-TC after upgrading.
Matching source and source RPM are under source/. Previous packages are retained.
Verify with `sha256sum -c SHA256SUMS` from this directory.
Build recipes are in ../packaging/ and ../debian/.
