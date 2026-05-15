import struct

SCALE = 65536
with open('recip.bin', 'wb') as f:
    # Dummy entry for index 0 so that divisor d is at offset d * 2
    f.write(struct.pack('<H', 0))
    
    for d in range(1, 1024):
        val = SCALE // d
        if val > 65535:
            val = 65535
        f.write(struct.pack('<H', val))
