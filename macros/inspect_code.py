import os
import glob
import re

streams_dir = r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams'
files = sorted(glob.glob(os.path.join(streams_dir, '*.bin')))

print(f'Total stream files: {len(files)}')

for fpath in files:
    with open(fpath, 'rb') as f:
        data = f.read()
    
    # Try cp1251
    text = data.decode('cp1251', errors='replace')
    
    # Check if this stream has VBA or script text
    lines = text.splitlines()
    code_lines = []
    for line in lines:
        cleaned = ''.join(c if (32 <= ord(c) < 127 or 1040 <= ord(c) <= 1103 or c in '№ёЁ\t ') else ' ' for c in line)
        if any(w in cleaned.lower() for w in ['sub ', 'function ', 'dim ', 'end sub', 'end function', 'as integer', 'as string', 'as long', 'as shape', 'as page', 'msgbox', 'k1804', 'masterblock', 'connector', 'export']):
            code_lines.append(cleaned.strip())
            
    if len(code_lines) > 2:
        fname = os.path.basename(fpath)
        print(f'=== {fname} ({len(code_lines)} code-like lines) ===')
        for cl in code_lines[:15]:
            print('  ', cl[:120])
        print()

