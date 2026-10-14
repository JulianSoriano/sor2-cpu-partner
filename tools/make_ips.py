"""make_ips.py ORIGINAL PATCHED OUT.ips - write an IPS patch (records merged across gaps < 8 bytes)."""
import sys
a = open(sys.argv[1], 'rb').read(); b = open(sys.argv[2], 'rb').read()
assert len(a) == len(b)
out = bytearray(b'PATCH'); i = 0; n = len(a)
while i < n:
    if a[i] == b[i]: i += 1; continue
    s = i; e = i + 1
    while e < n:
        if a[e] != b[e]: e += 1; continue
        g = e
        while g < n and g - e < 8 and a[g] == b[g]: g += 1
        if g < n and g - e < 8: e = g + 1
        else: break
    while e - s > 0:
        k = min(e - s, 0xFFFF)
        assert s != 0x454F46          # would read as "EOF"
        out += s.to_bytes(3, 'big') + k.to_bytes(2, 'big') + b[s:s + k]; s += k
    i = e
out += b'EOF'
open(sys.argv[3], 'wb').write(out); print(len(out), 'bytes')
