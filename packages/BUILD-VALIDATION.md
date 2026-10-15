# Baseline package validation — 2026-10-15

App 50.2, package revision 6. Built from f31f445 application code plus the
heading-only CHANGELOG.md and CONTRIBUTING.md. No application code changes.
Fedora 44 x86_64: RPM and source RPM rebuilt; all four Meson checks passed;
RPM digest verification and upgrade dry run passed.
Ubuntu 26.04 amd64: native DEB rebuild; all four Meson checks passed;
installation and --version launch passed in a disposable Ubuntu container.
Packages are unsigned. Desktop interaction was not retested in Ubuntu.
