import os
import glob
from oletools.olevba import decompress_stream

streams_dir = r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams'
files = sorted(glob.glob(os.path.join(streams_dir, '*.bin')))

print("Testing MS-OVBA decompression on extracted streams...")

all_decompressed_vba = []

for fpath in files:
    with open(fpath, 'rb') as f:
        data = f.read()

    fname = os.path.basename(fpath)
    # Search for MS-OVBA signature (0x01) followed by valid chunk header
    for i in range(len(data) - 10):
        if data[i] == 1:
            hdr = data[i+1] | (data[i+2] << 8)
            sig = (hdr >> 12) & 7
            if sig == 3:
                try:
                    decomp = decompress_stream(data[i:i+30000])
                    if len(decomp) > 50:
                        text = decomp.decode('cp1251', errors='replace')
                        if any(k in text.lower() for k in ['sub ', 'end sub', 'function ', 'dim ', 'attribute ', 'k1804', 'msgbox']):
                            all_decompressed_vba.append((fname, i, len(decomp), text))
                            print(f'Found MS-OVBA in {fname} at {i}: decompressed {len(decomp)} bytes!')
                except Exception:
                    pass

print(f'\nTotal MS-OVBA decompressed modules: {len(all_decompressed_vba)}')

with open(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\all_vba_code.txt', 'w', encoding='utf-8') as out_f:
    for fname, off, dlen, text in all_decompressed_vba:
        out_f.write(f'{"="*70}\nFILE: {fname} at offset {off} (len {dlen})\n{"="*70}\n')
        out_f.write(text)
        out_f.write('\n\n')

print('Saved all VBA code to macros/all_vba_code.txt')

