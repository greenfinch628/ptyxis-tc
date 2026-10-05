import json, shlex, subprocess
from pathlib import Path
root = Path.cwd()
build = root / 'build'
entry = next(e for e in json.loads((build / 'compile_commands.json').read_text()) if e['file'].endswith('/main.c'))
command = shlex.split(entry['command'])
command[command.index('-o') + 1] = '../test-right-click.o'
command[-1] = str(root / 'extension/test-right-click.c')
command.insert(1, '-D_GNU_SOURCE')
subprocess.run(command, cwd=build, check=True)
command = shlex.split((root / 'link-command.txt').read_text())
command[command.index('-o') + 1] = '../test-right-click'
command[command.index('src/ptyxis.p/main.c.o')] = '../test-right-click.o'
command.remove('src/ptyxis.p/ptyxis-terminal.c.o')
subprocess.run(command, cwd=build, check=True)
