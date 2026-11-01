import fcntl
import json
import os
import pathlib
import stat
import subprocess
import tempfile
import termios
import time

# configure.sh reads from /dev/tty, so it runs on a pseudo-terminal here.
def configure(answers, folder):
    master, slave = os.openpty()
    def controlling_tty():
        os.setsid()
        fcntl.ioctl(slave, termios.TIOCSCTTY, 0)
    env = dict(os.environ, INK_DIARY_CONFIG_DIR=folder)
    proc = subprocess.Popen(['sh', 'configure.sh'], stdin=slave, stdout=slave, stderr=slave,
                            preexec_fn=controlling_tty, env=env)
    os.close(slave)
    for answer in answers:
        time.sleep(0.3)
        os.write(master, (answer + '\n').encode())
    output = b''
    deadline = time.time() + 10
    while proc.poll() is None and time.time() < deadline:
        time.sleep(0.05)
    try:
        while True:
            chunk = os.read(master, 4096)
            if not chunk:
                break
            output += chunk
    except OSError:
        pass
    os.close(master)
    return proc.wait(timeout=5), output.decode(errors='replace')

with tempfile.TemporaryDirectory(dir='build-host') as folder:
    config = pathlib.Path(folder) / 'config.json'
    # Defaults: OpenRouter, the default free model, UTC.
    code, output = configure(['sk-or-v1-abc123', '', '', ''], folder)
    assert code == 0, output
    saved = json.loads(config.read_text())
    assert saved == {'api_key': 'sk-or-v1-abc123', 'base_url': 'https://openrouter.ai/api/v1',
                     'model': 'nvidia/nemotron-3.5-super-vl:free', 'web_search': True, 'timezone': 'UTC'}, saved
    assert stat.S_IMODE(config.stat().st_mode) == 0o600
    assert 'sk-or-v1-abc123' not in output
    # Explicit values, including a provider-prefixed model and a local endpoint.
    code, output = configure(['key.with.dots', 'http://192.168.1.20:11434/v1', 'google/gemma-4-31b-it:free', 'Europe/Stockholm'], folder)
    assert code == 0, output
    saved = json.loads(config.read_text())
    assert saved['base_url'] == 'http://192.168.1.20:11434/v1' and saved['model'] == 'google/gemma-4-31b-it:free'
    assert saved['timezone'] == 'Europe/Stockholm'
    # Invalid input leaves the previous configuration untouched.
    before = config.read_bytes()
    for answers in (['bad key!', '', '', ''], ['k', '', 'a/b/c', ''], ['k', 'ftp://x', '', ''], ['k', '', 'bad model', ''], ['k', '', '', 'Bad Zone;']):
        code, output = configure(answers, folder)
        assert code != 0 and config.read_bytes() == before, (answers, output)
print('PASS: configure.sh writes base_url/model/web_search/timezone with defaults, owner-only, rejects invalid input')
