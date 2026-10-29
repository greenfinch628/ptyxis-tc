#!/usr/bin/env python3
"""Build the integration fixture against the same objects as the application."""
import json, shlex, subprocess, sys
from pathlib import Path
root = Path(__file__).resolve().parents[1]
build = root / (sys.argv[1] if len(sys.argv) > 1 else 'build-tc')
entry = next(e for e in json.loads((build/'compile_commands.json').read_text()) if e['file'].endswith('/main.c'))
command = shlex.split(entry['command'])
obj = str(build/'test-tab-transfer.o')
command[command.index('-o')+1] = obj
command[-1] = str(root/'custom/test-tab-transfer.c')
subprocess.run(command, cwd=build, check=True)
commands = subprocess.check_output(['ninja','-t','commands','src/ptyxis'], cwd=build, text=True)
command = shlex.split(commands.splitlines()[-1])
command[command.index('-o')+1] = str(build/'test-tab-transfer')
command[command.index('src/ptyxis.p/main.c.o')] = obj
command.remove('src/ptyxis.p/ptyxis-window.c.o')
subprocess.run(command, cwd=build, check=True)
