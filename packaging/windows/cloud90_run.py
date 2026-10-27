"""Validated offline entry point for Cloud90 Game Link; no game data included."""
import os
from pathlib import Path
import sys
import subprocess
import time

SUPPORTED_IMAGE = '9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31'
LAUNCHER = 'Crash\u2122 Team Racing Nitro-Fueled.exe'


class BridgeRelay:
    """Only this session's native bridge writes this file; launcher logs stay private."""
    def __init__(self, stream, emit=print, clock=time.monotonic):
        self.stream, self.emit, self.clock = stream, emit, clock
        self.pending = b''
        self.last_state = clock()
        self.playing = False

    def poll(self):
        self.pending += self.stream.read(65536)
        while b'\n' in self.pending:
            line, self.pending = self.pending.split(b'\n', 1)
            text = line.decode('utf-8', errors='replace').strip()
            if not text.startswith('CLOUD90 '):
                continue
            self.emit(text, flush=True)
            if text in ('CLOUD90 state 0', 'CLOUD90 state 1'):
                self.last_state = self.clock()
                self.playing = text.endswith('1')
        if len(self.pending) > 4096:
            self.pending = b''
        # The client can remain open after stopping the game or during a restart.
        if self.playing and self.clock() - self.last_state > 3:
            self.emit('CLOUD90 state 0', flush=True)
            self.playing = False


def launch_client(port):
    launcher = port / LAUNCHER
    if not launcher.is_file():
        raise RuntimeError(f'No se encontró el cliente {LAUNCHER}')
    inbox = Path(os.environ['CLOUD90_CTR_INBOX']).resolve()
    output = inbox.parent / 'native-output.log'
    # This path belongs to the new per-launch session, never to launcher logs/saves.
    with output.open('wb'):
        pass
    os.environ['CLOUD90_CTR_OUTPUT'] = str(output)
    with output.open('rb') as stream:
        process = subprocess.Popen([str(launcher)], cwd=port)
        relay = BridgeRelay(stream)
        try:
            while process.poll() is None:
                relay.poll()
                time.sleep(0.1)
            relay.poll()
        finally:
            print('CLOUD90 state 0', flush=True)
        return process.returncode


def game_directory(port, env):
    if env.get('BB_GAME_DIR'):
        return Path(env['BB_GAME_DIR']).resolve()
    ini = port / 'launcher.ini'
    if ini.is_file():
        for line in ini.read_text(encoding='utf-8-sig').splitlines():
            key, _, value = line.partition('=')
            if key.strip() == 'game_dir' and value.strip():
                candidate = Path(value.strip())
                return (candidate if candidate.is_absolute() else port / candidate).resolve()
    return port / 'CUSA13795-1.21'


def main():
    port = Path(__file__).resolve().parent
    os.chdir(port)
    sys.path.insert(0, str(port))
    sys.path.insert(0, str(port / 'scripts'))
    from game_check import image_sha256
    game = game_directory(port, os.environ)
    if image_sha256(game) != SUPPORTED_IMAGE:
        raise RuntimeError('Cloud90 CTR requiere CUSA13795 1.21 con la imagen compatible; no se aplicaron modificadores')
    os.environ['BB_GAME_DIR'] = str(game)
    os.environ['BB_PROBE'] = str(port / 'bin' / 'sacredfire-cloud90.exe')
    os.environ['BB_ONLINE'] = '0'
    os.environ['CLOUD90_CTR_IMAGE_SHA256'] = SUPPORTED_IMAGE
    if '--launcher' in sys.argv[1:]:
        return launch_client(port)
    # Legacy direct mode also respects the language saved by the client.
    ini = port / 'launcher.ini'
    if 'BB_LANGUAGE' not in os.environ and ini.is_file():
        for line in ini.read_text(encoding='utf-8-sig').splitlines():
            key, _, value = line.partition('=')
            if key.strip() == 'language' and value.strip():
                os.environ['BB_LANGUAGE'] = value.strip()
                break
    import run
    return run.main()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as error:
        print(f'CLOUD90 error {error}', file=sys.stderr)
        sys.exit(2)
