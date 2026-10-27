"""Exercise the wrapper in Python isolated mode, as in the embedded portable."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
import importlib.util
from unittest.mock import patch
import io

WRAPPER = Path(__file__).resolve().parents[1] / 'packaging/windows/cloud90_run.py'
HASH = '9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31'
spec = importlib.util.spec_from_file_location('cloud90_wrapper', WRAPPER)
wrapper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(wrapper)


class LauncherTests(unittest.TestCase):
    def test_relay_partial_lines_ack_and_game_exit_timeout(self):
        with tempfile.TemporaryFile() as stream:
            said = []
            now = [0.0]
            relay = wrapper.BridgeRelay(stream, lambda text, **kw: said.append(text), lambda: now[0])
            stream.write(b'ignored log\nCLOUD90 state 1\nCLOUD90 ack test-1 ')
            stream.seek(0)
            relay.poll()
            self.assertEqual(said, ['CLOUD90 state 1'])
            offset = stream.tell()
            stream.write('ok\nCLOUD90 pause 1\n'.encode())
            stream.seek(offset)
            relay.poll()
            self.assertEqual(said[-2:], ['CLOUD90 ack test-1 ok', 'CLOUD90 pause 1'])
            now[0] = 3.1
            relay.poll()
            relay.poll()
            self.assertEqual(said.count('CLOUD90 state 0'), 1)

    def test_client_launch_inherits_bridge_and_preserves_settings(self):
        with tempfile.TemporaryDirectory() as folder:
            port = Path(folder)
            (port / wrapper.LAUNCHER).touch()
            session = port / 'session'
            session.mkdir()
            ini = port / 'launcher.ini'
            ini.write_text('language=20\n')
            with patch.dict(os.environ, {'CLOUD90_CTR_INBOX': str(session / 'inbox.json')}):
                with patch.object(wrapper.subprocess, 'Popen') as start:
                    start.return_value.poll.return_value = 0
                    start.return_value.returncode = 0
                    self.assertEqual(wrapper.launch_client(port), 0)
                    start.assert_called_once_with([str(port / wrapper.LAUNCHER)], cwd=port)
                    self.assertEqual(os.environ['CLOUD90_CTR_OUTPUT'], str(session / 'native-output.log'))
            self.assertEqual(ini.read_text(), 'language=20\n')

    def test_isolated_portable_launch_and_reject_other_image(self):
        with tempfile.TemporaryDirectory() as folder:
            port = Path(folder)
            (port / 'scripts').mkdir()
            shutil.copyfile(WRAPPER, port / WRAPPER.name)
            (port / 'launcher.ini').write_text('game_dir=game-data\nlanguage=20\n', encoding='utf-8')
            check = port / 'scripts/game_check.py'
            check.write_text(f'def image_sha256(game):\n assert game.name == "game-data"\n return "{HASH}"\n')
            (port / 'run.py').write_text(
                'import os\nfrom pathlib import Path\n'
                'def main():\n'
                ' assert os.environ["BB_ONLINE"] == "0"\n'
                ' assert os.environ["BB_LANGUAGE"] == "20"\n'
                ' assert Path(os.environ["BB_PROBE"]).name == "sacredfire-cloud90.exe"\n'
                ' assert os.environ["CLOUD90_CTR_IMAGE_SHA256"] == "' + HASH + '"\n'
                ' print("LAUNCH_OK")\n return 0\n')
            env = {k: v for k, v in os.environ.items() if k not in ('BB_GAME_DIR', 'BB_LANGUAGE')}
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
