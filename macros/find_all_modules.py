import sys
import os
sys.path.append(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros')
from extract_full_project import full_buffer
from oletools import olevba
import struct

def try_decompress(start):
    curr = start + 1
    decomp = bytearray()
    while curr < len(full_buffer):
        if curr + 2 > len(full_buffer): break
        h = struct.unpack('<H', full_buffer[curr:curr+2])[0]
        size = (h & 0xFFF) + 3
        sig = (h >> 12) & 7
        if sig != 3: break
        cdata = bytearray(b'\x01') + full_buffer[curr:curr+size]
        try:
            d = olevba.decompress_stream(cdata)
            decomp.extend(d)
        except Exception as e:
            pass
        curr += size
    return bytes(decomp)

found = {}
for p in range(len(full_buffer) - 5):
    if full_buffer[p] == 1:
        hdr = full_buffer[p+1] | (full_buffer[p+2] << 8)
        if ((hdr >> 12) & 7) == 3:
            d = try_decompress(p)
            if len(d) > 200:
                t = d.decode('cp1251', errors='replace')
                for line in t.splitlines()[:5]:
                    if 'attribute vb_name' in line.lower():
                        name = line.split('=')[1].strip().strip('"')
                        if name not in found or len(t) > len(found[name][1]):
                            found[name] = (p, t)

print(f'Total modules found: {len(found)}')
for name, (pos, text) in sorted(found.items()):
    print(f'Module: {name:20s} at {pos}, length {len(text)}')
    with open(os.path.join(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros', f'{name}.bas'), 'w', encoding='utf-8') as f:
        f.write(text)
