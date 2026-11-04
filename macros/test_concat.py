import os
from oletools.olevba import decompress_stream

streams_dir = r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams'

# Let's concatenate streams from 020 to 031
concat_data = bytearray()
for i in range(21, 31):
    fpath = os.path.join(streams_dir, f'stream_{i:03d}_*.bin')
    import glob
    matches = glob.glob(fpath)
    if matches:
        with open(matches[0], 'rb') as f:
            b = f.read()
            # If the block has 4-byte header, is it b[4:]?
            # Let's check length:
            if len(b) == 8196:
                concat_data.extend(b[4:])
            else:
                concat_data.extend(b)

print(f'Concatenated data len: {len(concat_data)}')

# Now let's search for MS-OVBA compressed streams in concat_data!
for i in range(len(concat_data) - 10):
    if concat_data[i] == 1:
        hdr = concat_data[i+1] | (concat_data[i+2] << 8)
        if ((hdr >> 12) & 7) == 3:
            try:
                decomp = decompress_stream(concat_data[i:])
                if len(decomp) > 200:
                    text = decomp.decode('cp1251', errors='replace')
                    name = 'unknown'
                    for line in text.splitlines()[:10]:
                        if 'attribute vb_name' in line.lower():
                            name = line
                    print(f'Concat at {i}: decomp {len(decomp)} bytes! Module: {name}')
            except Exception:
                pass

