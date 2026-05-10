# Previous failed experiment
#
# SCALE = 16384
# with open("recip.bin", "wb") as f:
#     for de in range(768):
#         val = 0 if de == 0 else int(SCALE / de)
#         f.write(min(255, val).to_bytes(1, 'little'))

with open("mask.bin", "wb") as f:
    for v in [128, 64, 32, 16, 8, 4, 2, 1]:
        f.write(v.to_bytes(1, 'little'))
