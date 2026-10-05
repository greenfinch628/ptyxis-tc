import json, shlex, subprocess
from pathlib import Path
root = Path.cwd()
build = root / 'build'
entry = next(e for e in json.loads((build / 'compile_commands.json').read_text()) if e['file'].endswith('/main.c'))
command = shlex.split(entry['command'])
command[command.index('-o') + 1] = '../test-session-colors.o'
command[-1] = str(root / 'extension/test-session-colors.c')
subprocess.run(command, cwd=build, check=True)
command = shlex.split((root / 'link-command.txt').read_text())
command[command.index('-o') + 1] = '../test-session-colors'
command[command.index('src/ptyxis.p/main.c.o')] = '../test-session-colors.o'
subprocess.run(command, cwd=build, check=True)
