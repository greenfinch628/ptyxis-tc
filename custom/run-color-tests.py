#!/usr/bin/env python3
"""Run tab rendering and full-application regression tests after a Meson build.

Usage: python3 custom/run-color-tests.py [build-directory] [output-directory]
Requires the build dependencies, Xvfb, dbus-run-session, and Python 3.
Tests always use private settings and a separate display; no live sessions change.
"""
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys

source = Path(__file__).resolve().parents[1]
build = Path(sys.argv[1] if len(sys.argv) > 1 else source / 'build-tc').resolve()
output = Path(sys.argv[2] if len(sys.argv) > 2 else build / 'color-tests').resolve()
output.mkdir(parents=True, exist_ok=True)
subprocess.run(['meson', 'compile', '-C', str(build)], check=True)
commands = json.loads((build / 'compile_commands.json').read_text())
entry = next(c for c in commands if Path(c['file']).name == 'main.c')
link_template = shlex.split(subprocess.check_output(
    ['ninja', '-C', str(build), '-t', 'commands', 'src/ptyxis'], text=True).splitlines()[-1])
for name, test_source in [('test-session', 'test-session-colors.c'),
                          ('test-dialogs', 'test-preferences-dialogs.c'),
                          ('test-reset', 'test-reset-settings.c')]:
    command = shlex.split(entry['command'])
    command[command.index(entry['file'])] = str(source / 'custom' / test_source)
    for option, suffix in [('-o', '.o'), ('-MF', '.d'), ('-MQ', '.o')]:
        command[command.index(option) + 1] = str(output / (name + suffix))
    command += ['-Wno-declaration-after-statement', '-Wno-missing-declarations']
    subprocess.run(command, cwd=build, check=True)
    link = link_template.copy()
    link[link.index('-o') + 1] = str(output / name)
    link = [str(output / (name + '.o')) if item == 'src/ptyxis.p/main.c.o' else item for item in link]
    subprocess.run(link, cwd=build, check=True)
flags = shlex.split(subprocess.check_output(
    ['pkg-config', '--cflags', '--libs', 'gtk4', 'libadwaita-1', 'vte-2.91-gtk4'], text=True))
subprocess.run(['cc', '-Wall', '-Wextra', '-Wno-deprecated-declarations', '-Wno-unused-parameter',
                '-Wno-unused-function', str(source / 'custom/test-tab-colors.c'),
                '-I' + str(source / 'src'), '-o', str(output / 'test-tabs'), *flags], check=True)
schemas = output / 'schemas'
schemas.mkdir(exist_ok=True)
for schema in (build / 'src').glob('*.gschema.xml'):
    (schemas / schema.name).write_bytes(schema.read_bytes())
subprocess.run(['glib-compile-schemas', str(schemas)], check=True)
environment = dict(os.environ, GTK_A11Y='none', GSK_RENDERER='cairo', GIO_USE_VFS='local',
                   GSETTINGS_BACKEND='memory', GSETTINGS_SCHEMA_DIR=str(schemas),
                   XDG_CONFIG_HOME=str(output / 'config'), XDG_DATA_HOME=str(output / 'data'),
                   XDG_CACHE_HOME=str(output / 'cache'), XDG_STATE_HOME=str(output / 'state'))
for name in ['config', 'data', 'cache', 'state']:
    (output / name).mkdir(exist_ok=True)
for name in ['test-tabs', 'test-session', 'test-dialogs', 'test-reset']:
    log = output / (name + '.log')
    with log.open('w') as stream:
        result = subprocess.run(['dbus-run-session', '--', 'xvfb-run', '-a', str(output / name)],
                                cwd=output, env=environment, stdout=stream, stderr=subprocess.STDOUT,
                                timeout=60)
    lines = log.read_text().splitlines()
    if result.returncode:
        print(log.read_text(), file=sys.stderr)
        raise SystemExit(result.returncode)
    print(next(line for line in lines if line.startswith('PASS:')))
print('Screenshots and logs:', output)
