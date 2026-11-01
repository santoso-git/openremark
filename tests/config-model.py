import json
import pathlib
import stat
import subprocess
import tempfile

with tempfile.TemporaryDirectory(dir='build-host') as folder:
    path = pathlib.Path(folder) / 'config.json'
    original = {'api_key': 'fixture-only', 'model': 'old-model', 'timezone': 'America/New_York', 'demo_mode': False, 'custom': {'keep': True}}
    path.write_text(json.dumps(original))
    def run(model):
        return subprocess.run(['build-host/config-model', model, str(path)], capture_output=True)
    assert run('claude-opus-5-5').returncode == 0
    assert json.loads(path.read_text()) == dict(original, model='claude-opus-5-5')
    assert stat.S_IMODE(path.stat().st_mode) == 0o600
    previous = path.read_bytes()
    assert run('bad model').returncode != 0 and path.read_bytes() == previous
    for invalid in ['{bad json', '{}']:
        path.write_text(invalid)
        assert run('claude-opus-5-5').returncode != 0 and path.read_text() == invalid

    # OpenRouter model ids contain a provider prefix and a :free suffix.
    path.write_text(json.dumps(original))
    assert run('google/gemma-4-31b-it:free').returncode == 0
    assert json.loads(path.read_text())['model'] == 'google/gemma-4-31b-it:free'
    assert run('nvidia/nemotron-3.5-super-vl:free').returncode == 0
    assert run('bad model').returncode != 0
    assert run('../../etc').returncode != 0
    assert run('a/b/c').returncode != 0

    # --set KEY reads the value from stdin (keeps secrets out of argv); only known keys are accepted.
    path.write_text(json.dumps(original))
    def setkey(key,value):
        return subprocess.run(['build-host/config-model','--set',key,str(path)],input=value.encode(),capture_output=True)
    assert setkey('timezone','Europe/Stockholm\n').returncode == 0
    saved = json.loads(path.read_text())
    assert saved['timezone'] == 'Europe/Stockholm'
    assert saved['api_key'] == 'fixture-only' and saved['custom'] == {'keep': True}
    assert stat.S_IMODE(path.stat().st_mode) == 0o600
    before = path.read_bytes()
    assert setkey('api_endpoint','http://evil').returncode != 0 and path.read_bytes() == before
    assert setkey('timezone','').returncode != 0 and path.read_bytes() == before
    assert setkey('base_url','https://openrouter.ai/api/v1').returncode == 0
    assert json.loads(path.read_text())['base_url'] == 'https://openrouter.ai/api/v1'
    before = path.read_bytes()
    assert setkey('base_url','ftp://x').returncode != 0 and path.read_bytes() == before
    # The second button's model and effort can be set the same way.
    assert setkey('premium_model','anthropic/claude-haiku-5.5\n').returncode == 0
    assert setkey('premium_effort','none\n').returncode == 0
    saved = json.loads(path.read_text())
    assert saved['premium_model'] == 'anthropic/claude-haiku-5.5' and saved['premium_effort'] == 'none'
    # The tools button's proxy address, key and model; the key comes from stdin like every value.
    assert setkey('tools_base_url','https://proxy.example.com/v1\n').returncode == 0
    assert setkey('tools_api_key','proxy-key\n').returncode == 0
    assert setkey('tools_model','anthropic/claude-haiku-5.5\n').returncode == 0
    saved = json.loads(path.read_text())
    assert saved['tools_base_url'] == 'https://proxy.example.com/v1' and saved['tools_api_key'] == 'proxy-key'
    before = path.read_bytes()
    assert setkey('tools_base_url','ftp://x').returncode != 0 and path.read_bytes() == before
    # Email was removed: its settings are no longer accepted.
    for removed in ['gmail_address','gmail_app_password','name']:
        assert setkey(removed,'x').returncode != 0 and path.read_bytes() == before
print('PASS: model updates preserve key/settings, owner-only permissions, invalid input leaves config unchanged, --set from stdin for known keys')
