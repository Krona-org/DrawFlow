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

ole = olefile.OleFileIO(r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\K1804VisualEditor.vsd')
vd = ole.openstream('VisioDocument').read()

# Let's find all 18-byte pointers across the entire file
# Every pointer has:
# type: int32
# address: uint32
# offset: uint32
# length: uint32
# format: int16
# format & 2 != 0 means compressed with VisioLZW

# Let's search for sequences of pointers
# A pointer is valid if:
# 0 <= offset < len(vd) and 0 < length <= len(vd) and offset + length <= len(vd)
# 0 <= type < 256
# format & ~0x1FF == 0

out_dir = r'c:\Users\marus\OneDrive\Desktop\editor_qt6\macros\extracted_streams'
os.makedirs(out_dir, exist_ok=True)

# Find start of pointer table around 285476
start_pos = 285476
while start_pos >= 18:
    prev = start_pos - 18
    p_type = struct.unpack('<i', vd[prev:prev+4])[0]
    address = struct.unpack('<I', vd[prev+4:prev+8])[0]
    p_offset = struct.unpack('<I', vd[prev+8:prev+12])[0]
    length = struct.unpack('<I', vd[prev+12:prev+16])[0]
    format_val = struct.unpack('<h', vd[prev+16:prev+18])[0]
    if (0 <= p_offset < len(vd) and 0 < length <= len(vd) and p_offset + length <= len(vd) and
        0 <= p_type < 256 and (format_val & ~0x1ff) == 0):
        start_pos = prev
    else:
        break

print(f'Pointer table starts at {start_pos}')

pointers = []
pos = start_pos
while pos + 18 <= len(vd):
    p_type = struct.unpack('<i', vd[pos:pos+4])[0]
    address = struct.unpack('<I', vd[pos+4:pos+8])[0]
    p_offset = struct.unpack('<I', vd[pos+8:pos+12])[0]
    length = struct.unpack('<I', vd[pos+12:pos+16])[0]
    format_val = struct.unpack('<h', vd[pos+16:pos+18])[0]
    if (0 <= p_offset < len(vd) and 0 < length <= len(vd) and p_offset + length <= len(vd) and
        0 <= p_type < 256 and (format_val & ~0x1ff) == 0):
        pointers.append((pos, p_type, address, p_offset, length, format_val))
        pos += 18
    else:
        break

print(f'Found {len(pointers)} pointers in table.')

stream_info = []
for idx, (ptr_pos, p_type, address, p_offset, length, format_val) in enumerate(pointers):
    raw = vd[p_offset : p_offset + length]
    is_comp = (format_val & 2) != 0
    if is_comp:
        try:
            decomp = VisioLZW.decompress(raw)
        except Exception as e:
            decomp = raw
    else:
        decomp = raw

    filename = f'stream_{idx:03d}_type{p_type}_offset{p_offset}.bin'
    filepath = os.path.join(out_dir, filename)
    with open(filepath, 'wb') as f:
        f.write(decomp)

    # Check for text strings
    text = decomp.decode('cp1251', errors='ignore')
    keywords = ['sub ', 'function ', 'dim ', 'k1804', 'masterblock', 'connector', 'export', 'page', 'open ', 'write ', 'print #']
    matches = [k for k in keywords if k in text.lower()]
    
    stream_info.append({
        'idx': idx,
        'ptr_pos': ptr_pos,
        'type': p_type,
        'addr': hex(address),
        'offset': p_offset,
        'len': length,
        'decomp_len': len(decomp),
        'comp': is_comp,
        'matches': matches,
        'filename': filename
    })

# Summary
print('\nStreams with interesting matches:')
for s in stream_info:
    if s['matches']:
        print(f"[{s['idx']:03d}] type={s['type']} off={s['offset']} len={s['len']}->{s['decomp_len']} matches={s['matches']}")

