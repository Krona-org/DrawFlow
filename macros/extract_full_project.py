import os, glob
from oletools import olevba
import struct

streams_dir = r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams'
all_files = sorted(glob.glob(os.path.join(streams_dir, '*.bin')))

print(f'Total stream files: {len(all_files)}')

full_buffer = bytearray()
for fpath in all_files:
    with open(fpath, 'rb') as f:
        b = f.read()
    if len(b) == 8196:
        full_buffer.extend(b[4:])
    else:
        full_buffer.extend(b)

print(f'Full concatenated buffer size: {len(full_buffer)} bytes')

found_modules = {}

for i in range(len(full_buffer) - 10):
    if full_buffer[i] == 1:
        hdr = full_buffer[i+1] | (full_buffer[i+2] << 8)
        if ((hdr >> 12) & 7) == 3:
            compressed_current = i + 1
            decomp = bytearray()
            while compressed_current < len(full_buffer):
                if compressed_current + 2 > len(full_buffer):
                    break
                compressed_chunk_header = struct.unpack('<H', full_buffer[compressed_current:compressed_current + 2])[0]
                chunk_size = (compressed_chunk_header & 0x0FFF) + 3
                chunk_signature = (compressed_chunk_header >> 12) & 0x07
                if chunk_signature != 3:
                    break
                
                chunk_data = bytearray(b'\x01') + full_buffer[compressed_current:compressed_current+chunk_size]
                try:
                    d = olevba.decompress_stream(chunk_data)
                    decomp.extend(d)
                except Exception:
                    pass
                compressed_current += chunk_size
            
            if len(decomp) > 50:
                t = decomp.decode('cp1251', errors='replace')
                if 'attribute vb_name' in t.lower():
                    # extract name
                    mod_name = 'Unknown'
                    for line in t.splitlines()[:10]:
                        if 'attribute vb_name' in line.lower():
                            parts = line.split('=')
                            if len(parts) > 1:
                                mod_name = parts[1].strip().strip('"')
                    # keep longest version if duplicate
                    if mod_name not in found_modules or len(t) > len(found_modules[mod_name][1]):
                        found_modules[mod_name] = (i, t)

print(f'\n=== FOUND {len(found_modules)} UNIQUE VBA MODULES ===')
for mod_name, (off, text) in sorted(found_modules.items()):
    print(f'Module: {mod_name:20s} (offset={off}, length={len(text)} chars)')
    fname = f'{mod_name}.bas'
    out_path = os.path.join(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros', fname)
    with open(out_path, 'w', encoding='utf-8') as out_f:
        out_f.write(text)
    print(f'  Saved to {fname}')

