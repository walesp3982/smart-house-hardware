import os

with open('.env', 'r') as f:
    for line in f:
        if line.strip() and not line.startswith('#'):
            key, value = line.strip().split('=', 1)
            os.environ[key] = value
            print(f'-D{key.strip()}=\\"{value.strip()}\\"')