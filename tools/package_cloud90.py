"""Package only Cloud90's side-by-side runtime, wrapper and installer."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--runtime', type=Path, default=ROOT / 'out/gpu-win/sacredfire.exe')
    parser.add_argument('--output', type=Path, default=ROOT / 'dist/CTRCloud90')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    runtime = args.output / 'sacredfire-cloud90.exe'
    shutil.copyfile(args.runtime, runtime)
    for file in ['cloud90_run.py', 'install-cloud90.ps1']:
        shutil.copyfile(ROOT / 'packaging/windows' / file, args.output / file)
    shutil.copyfile(ROOT / 'LICENSE', args.output / 'LICENSE')
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    manifest = dict(bridgeVersion='cloud90-ctr-1',
                    launcherBridgeVersion='1',
                    gameImageSha256='9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31',
                    runtimeSha256=sha(runtime), wrapperSha256=sha(args.output / 'cloud90_run.py'),
                    source='https://github.com/jluisgomezmontal/ctr-cloud90', sourceRevision=revision)
    subprocess.run(['git', 'archive', '--format=tar', '--prefix=ctr-cloud90/',
                    '--output='+str((args.output / 'source.tar').resolve()), revision], cwd=ROOT, check=True)
    (args.output / 'SOURCE.txt').write_text(
        f'Corresponding source: {manifest["source"]}/tree/{revision}\n'
        f'git clone {manifest["source"]}.git\ncd ctr-cloud90\ngit checkout {revision}\n'
        'git submodule update --init --recursive\n'
        'See CLOUD90.md and the original Windows build instructions. source.tar contains the tracked project source; submodules are pinned in Git.\n',
        encoding='utf-8')
    (args.output / 'cloud90-ctr.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
    (args.output / 'README.txt').write_text(
        'Cloud90 CTR module (GPL-2.0-or-later). Source: '+manifest['source']+'\n'
        'Needs OCTR v0.6.4 Windows portable and your own supported USA 1.21 dump.\n'
        'Close CTR, then run: powershell -ExecutionPolicy Bypass -File install-cloud90.ps1 -PortablePath "D:\\path\\to\\portable"\n'
        'The original runtime, client, game files and saves are retained. Open via Cloud90 Game Link 1.2.2, then press Play in the original client.\n',
        encoding='utf-8')
    archive = shutil.make_archive(str(args.output), 'zip', args.output)
    print(archive)


if __name__ == '__main__':
    main()
