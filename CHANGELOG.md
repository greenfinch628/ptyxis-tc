# ptyxis-tc - 50.2-7

- 2026-10-16T21:02:13Z — Changed: Package the tested tab-color and master-reset updates for Fedora 44 deployment to Scopuli, Rocinante, Karakum, and Narlikar, preserving existing per-user settings.

# ptyxis-tc - 50.2

- 2026-10-16T19:01:28Z — Added: Confirmed Reset All Settings action in Appearance, with a backup of preferences/profiles/shortcuts and saved tab-color/session files; restore initial defaults without closing running terminals.
- 2026-10-16T18:54:52Z — Added: Default button for active #e8e8e8 and inactive #c0bfbc backgrounds; use the same pair for first startup and new tabs without manual or hostname overrides, with matching Preferences reset swatches.
- 2026-10-16T18:05:16Z — Changed: Default active and inactive tab text to black regardless of background; show the black default in Preferences and update text swatches on reset while retaining saved custom colors.
- 2026-10-16T18:01:50Z — Changed: Reversed automatic tab text colors for visual evaluation: white on light backgrounds and dark text on dark backgrounds; explicit text overrides and the white active underline remain unchanged.
- 2026-10-16T17:57:24Z — Fixed: Preserve the white active-tab underline independently of automatic or configured text colors, including orange tabs.
- 2026-10-16T17:51:32Z — Fixed: Keep Preferences tab color chooser dialogs alive for their buttons, preventing crashes when opening a chooser around a reset.
- 2026-10-16T16:01:16Z — Added: Independent active/inactive tab background and text defaults in Preferences, separate manual and hostname inactive backgrounds, and persistence for manual color pairs; retained automatic SSH colors and immediate selection styling.
- 2026-10-15T22:26:23Z — Changed: Clarified as-is availability, absence of technical support, and discretionary consideration of bug reports and enhancements without implementation commitments.
