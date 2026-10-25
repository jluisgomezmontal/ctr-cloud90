"""Validated offline entry point for Cloud90 Game Link; no game data included."""
import os
from pathlib import Path
import sys

SUPPORTED_IMAGE = '9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31'


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
    import run
    return run.main()


if __name__ == '__main__':
    try:
        sys.exit(main())
    except Exception as error:
        print(f'CLOUD90 error {error}', file=sys.stderr)
        sys.exit(2)
