import struct
import io
import os
import olefile

class VisioLZW:
    DICT_SIZE = 4096
    DICT_MASK = 4095

    @staticmethod
    def adjust_dictionary_offset(pntr):
        if pntr > 4078:
            return pntr - 4078
        else:
            return pntr + 18

    @classmethod
    def decompress(cls, src_bytes):
        src = io.BytesIO(src_bytes)
        res = io.BytesIO()
        buffer = bytearray(cls.DICT_SIZE)
        pos = 0

        while True:
            flag_byte = src.read(1)
            if not flag_byte:
                break
            flag = flag_byte[0]
            mask = 1
            while mask < 0x100:
                is_mask_set = (flag & mask) > 0
                if is_mask_set:
                    data_i = src.read(1)
                    if not data_i:
                        break
                    val = data_i[0]
                    buffer[pos & cls.DICT_MASK] = val
                    pos += 1
                    res.write(data_i)
                else:
                    pt = src.read(2)
                    if len(pt) < 2:
                        break
                    data_ipt1, data_ipt2 = pt[0], pt[1]
                    length = (data_ipt2 & 0x0F) + 3
                    pntr = data_ipt1 + ((data_ipt2 & 0xF0) << 4)
                    pntr = cls.adjust_dictionary_offset(pntr)

                    data_b = bytearray(length)
                    for i in range(length):
                        b = buffer[(pntr + i) & cls.DICT_MASK]
                        data_b[i] = b
                        buffer[(pos + i) & cls.DICT_MASK] = b
                    res.write(data_b)
                    pos += length
                mask <<= 1
        return res.getvalue()

class Pointer:
    def __init__(self, p_type, address, offset, length, format_val):
        self.type = p_type
        self.address = address
        self.offset = offset
        self.length = length
        self.format = format_val

    def is_compressed(self):
        return (self.format & 2) > 0

    def has_pointers(self):
        return (self.type == 20) or (0x1d <= self.format <= 0x1f) or (0x50 <= self.format <= 0x60)

    @classmethod
    def read(cls, data, offset):
        p_type = struct.unpack('<i', data[offset:offset+4])[0]
        address = struct.unpack('<I', data[offset+4:offset+8])[0]
        p_offset = struct.unpack('<I', data[offset+8:offset+12])[0]
        length = struct.unpack('<I', data[offset+12:offset+16])[0]
        format_val = struct.unpack('<h', data[offset+16:offset+18])[0]
        return cls(p_type, address, p_offset, length, format_val)

ole = olefile.OleFileIO(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\K1804VisualEditor.vsd')
vd = ole.openstream('VisioDocument').read()

trailer_ptr = Pointer.read(vd, 0x24)
print(f'Root trailer pointer: offset={trailer_ptr.offset}, len={trailer_ptr.length}, fmt=0x{trailer_ptr.format:04x}, compressed={trailer_ptr.is_compressed()}')

all_streams = []

def process_stream(ptr, depth=0):
    raw = vd[ptr.offset : ptr.offset + ptr.length]
    if ptr.is_compressed():
        try:
            content = VisioLZW.decompress(raw)
        except Exception as e:
            content = raw
            print(f'Decompress error at {ptr.offset}: {e}')
    else:
        content = raw
    all_streams.append((ptr, content))

    if ptr.has_pointers() and len(content) >= 8:
        num_ptrs_offset = struct.unpack('<I', content[0:4])[0]
        if num_ptrs_offset + 4 <= len(content):
            num_ptrs = struct.unpack('<I', content[num_ptrs_offset:num_ptrs_offset+4])[0]
            pos = num_ptrs_offset + 8
            indent = "  " * depth
            print(f'{indent}Stream type={ptr.type} fmt=0x{ptr.format:04x} has {num_ptrs} child pointers (offset {ptr.offset}, len {ptr.length}->{len(content)})')
            for _ in range(num_ptrs):
                if pos + 18 <= len(content):
                    child_ptr = Pointer.read(content, pos)
                    pos += 18
                    process_stream(child_ptr, depth + 1)

process_stream(trailer_ptr)
print(f'Total streams extracted: {len(all_streams)}')

# Check each stream for strings and macros
macros_found = []
for idx, (ptr, content) in enumerate(all_streams):
    # Search for keywords
    text_cp1251 = content.decode('cp1251', errors='ignore')
    keywords = ['sub ', 'function ', 'dim ', 'k1804', 'masterblock', 'connector', 'shape', 'export', 'open ']
    matches = [k for k in keywords if k in text_cp1251.lower()]
    if matches:
        macros_found.append((idx, ptr, matches, text_cp1251, content))
        print(f'Stream {idx}: type={ptr.type}, offset={ptr.offset}, len={len(content)}, matches={matches}')

print(f'\nTotal streams with interesting content: {len(macros_found)}')
with open(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams.txt', 'w', encoding='utf-8') as out_f:
    for idx, ptr, matches, text, raw in macros_found:
        out_f.write(f'{"="*60}\nSTREAM {idx}: type={ptr.type}, offset={ptr.offset}, len={len(raw)}, matches={matches}\n{"="*60}\n')
        out_f.write(text)
        out_f.write('\n\n')

print('Saved to macros/extracted_streams.txt')

