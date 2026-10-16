# Fedora 44 deployment: Ptyxis-TC 50.2-7

Installed on Scopuli, Rocinante, Karakum, and Narlikar on October 16, 2026.
Narlikar received a first installation; the other three were upgraded.

The package includes independent active/inactive tab backgrounds and text colors,
black default text, white active underline, active #e8e8e8/inactive #c0bfbc defaults,
and the confirmed master reset in Preferences → Appearance.

## Validation

- Built one Fedora 44 x86_64 RPM and matching source RPM from the authoritative
  working tree, including the new source/header/test files.
- Package build passed all four Meson checks. The source regression suites
  previously passed tab rendering, session/Preferences, native dialogs, and reset
  coverage before packaging.
- All four installed payloads passed RPM verification (excluding the generated
  ghost schema cache). All four installed executable hashes are identical.
- All four normal installed launchers executed a real host shell command and
  produced PTYXIS_TC_OK, on private virtual displays with temporary settings.
  Optional container discovery was disabled only in the smoke-test environment;
  the normal user-session service was used for spawning the shell.
- Saved TC settings hashes were unchanged on all machines. Regular Ptyxis package
  versions were unchanged. No running user terminals were stopped.
- Scopuli's stale offline update queue was cleared with user authorization.
  The other three had no queued offline transaction.

## Backups and logs

Each machine has a private pre-install backup of its original installed TC files,
color/session configuration, palettes, and exported settings at the path below.
The directory includes settings.dconf, previous-install-and-settings.tar.gz,
and manifest.json. These backups contain terminal metadata and should remain private.

- scopuli: `/home/rczys/.local/state/org.gnome.Ptyxis.TabColors/install-backups/50.2-7-20261016T210350285808Z`
- rocinante: `/home/rczys/.local/state/org.gnome.Ptyxis.TabColors/install-backups/50.2-7-20261016T210347972154Z`
- karakum: `/home/rczys/.local/state/org.gnome.Ptyxis.TabColors/install-backups/50.2-7-20261016T210345164450Z`
- narlikar: `/home/rczys/.local/state/org.gnome.Ptyxis.TabColors/install-backups/50.2-7-20261016T210346612471Z`

Installed binary SHA256: `c057848b03c452e5c4c4f0ed5521bc16126b781393298f15df002c50b108f320`.

See DEPLOYMENT-50.2-7.json for per-host checks. Source provenance and file digests
are recorded in SOURCE-MANIFEST-50.2-7.json. The source snapshot records the parent
baseline and exact working-tree inputs used for the build; it was built before
recording this deployment commit. Source-code digests identify the build inputs
independently of later package/deployment documentation updates.

Packages remain unsigned local builds. No Ubuntu package was rebuilt for this
Fedora-only deployment; Ubuntu revision 6 remains the previous baseline.
