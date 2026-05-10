# Previous failed experiment
#
# SCALE = 16384
# with open("recip.bin", "wb") as f:
#     for de in range(768):
#         val = 0 if de == 0 else int(SCALE / de)
#         f.write(min(255, val).to_bytes(1, 'little'))

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

with open("scr_ofs.bin", "wb") as f:
    for y in range(192):
        f.write(py2saddr(y).to_bytes(2, 'little'))
