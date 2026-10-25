"""Exercise the wrapper in Python isolated mode, as in the embedded portable."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

WRAPPER = Path(__file__).resolve().parents[1] / 'packaging/windows/cloud90_run.py'
HASH = '9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31'


class LauncherTests(unittest.TestCase):
    def test_isolated_portable_launch_and_reject_other_image(self):
        with tempfile.TemporaryDirectory() as folder:
            port = Path(folder)
            (port / 'scripts').mkdir()
            shutil.copyfile(WRAPPER, port / WRAPPER.name)
            (port / 'launcher.ini').write_text('game_dir=game-data\n', encoding='utf-8')
            check = port / 'scripts/game_check.py'
            check.write_text(f'def image_sha256(game):\n assert game.name == "game-data"\n return "{HASH}"\n')
            (port / 'run.py').write_text(
                'import os\nfrom pathlib import Path\n'
                'def main():\n'
                ' assert os.environ["BB_ONLINE"] == "0"\n'
                ' assert Path(os.environ["BB_PROBE"]).name == "sacredfire-cloud90.exe"\n'
                ' assert os.environ["CLOUD90_CTR_IMAGE_SHA256"] == "' + HASH + '"\n'
                ' print("LAUNCH_OK")\n return 0\n')
            env = {k: v for k, v in os.environ.items() if k != 'BB_GAME_DIR'}
            env['BB_ONLINE'] = '1'
            result = subprocess.run([sys.executable, '-I', str(port / WRAPPER.name)], env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('LAUNCH_OK', result.stdout)
            check.write_text('def image_sha256(game):\n return "other-image"\n')
            result = subprocess.run([sys.executable, '-I', str(port / WRAPPER.name)], env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertNotIn('LAUNCH_OK', result.stdout)


if __name__ == '__main__':
    unittest.main()
