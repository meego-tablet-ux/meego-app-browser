"""Write back all data it receives."""

import sys

data = sys.stdin.read(1)
while data:
    sys.stdout.write(data)
    data = sys.stdin.read(1)
sys.stderr.write("byebye")
sys.stderr.flush()
