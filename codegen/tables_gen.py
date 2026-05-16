import struct
import math

def py2saddr(y):
    # ZX Spectrum screen base is 0x4000
    # Y bits [2:0] -> H bits [2:0]  (pixel row within character cell)
    # Y bits [7:6] -> H bits [4:3]  (which third of screen)
    # Y bits [5:3] -> L bits [7:5]  (character row within third)
    h = (y & 0x07) | ((y & 0xC0) >> 3)
    l = (y & 0x38) << 2
    return (0x40 | h) << 8 | l

with open("mask.bin", "wb") as f:
    for v in [128, 64, 32, 16, 8, 4, 2, 1]:
        f.write(v.to_bytes(1, 'little'))

with open("scr_ofs_lo.bin", "wb") as f:
    for y in range(192):
        f.write((py2saddr(y) & 0xff).to_bytes(1, 'little'))

with open("scr_ofs_hi.bin", "wb") as f:
    for y in range(192):
        f.write((py2saddr(y) >> 8).to_bytes(1, 'little'))

# Compute Sincos data
# 72 entries, 5 degrees each
with open("sincos.bin", "wb") as f:
    for i in range(72):
        angle_rad = math.radians(i * 5)
        
        # To make results fit in 16 bit, and avoid scaling
        # in the drawPoints loop, we precompute here all that
        # can be moved out of the inner loop.
        # Yes, lots of magic constants :-)
        
        # si_raw = round(sin * 256)
        # si_final = (si_raw // 3) << 6
        si_raw = int(round(math.sin(angle_rad) * 256))
        si_final = (si_raw // 3) << 6
        
        # co_raw = round(cos * 256)
        # co_final = co_raw // 3
        co_raw = int(round(math.cos(angle_rad) * 256))
        co_final = co_raw // 3
        
        f.write(struct.pack('<hh', si_final, co_final))
