import os
from oletools.olevba import decompress_stream

# Let's inspect BlockParams in stream_010:
with open(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams\stream_010_type31_offset31266.bin', 'rb') as f:
    data = f.read()

decomp = decompress_stream(data[4529:])
text = decomp.decode('cp1251', errors='replace')
with open(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\BlockParams.bas', 'w', encoding='utf-8') as f:
    f.write(text)

print(f'Saved BlockParams.bas ({len(text)} chars)')
print('First 500 chars of BlockParams:')
print(text[:500])

