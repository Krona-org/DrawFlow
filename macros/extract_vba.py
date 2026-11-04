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
                    # Uncompressed literal
                    data_i = src.read(1)
                    if not data_i:
                        break
                    val = data_i[0]
                    buffer[pos & cls.DICT_MASK] = val
                    pos += 1
                    res.write(data_i)
                else:
                    # Compressed token
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

print('Searching for compressed streams in VisioDocument...')
found_sections = []
for start in range(len(vd) - 200):
    # Quick filter: let's test if starting here decompresses readable ASCII
    # Most VBA modules start with something recognizable or contain common keywords
    # To be fast, let's test every offset that might look like a stream header
    pass

