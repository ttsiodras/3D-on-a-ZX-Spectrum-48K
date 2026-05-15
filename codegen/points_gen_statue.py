import struct
import sys
import statue_data

def generate_points(scale_factor):
    points = statue_data.POINTS
    maxx_float = statue_data.MAXX
    
    # Use the scaling factor provided via command line
    # The original fixed-point scale was approximately 8960
    scale = float(scale_factor)
    
    # Re-calculate the fixed-point MAXX based on the X-axis
    # since that's what the original code did
    fixed_maxx_x = 0
    for p in points:
        val = abs(int(round(p[0] * scale)))
        if val > fixed_maxx_x:
            fixed_maxx_x = val
            
    # Use this fixed_maxx_x to calculate SE, matching the original logic
    # SE = 256 + MAXX / 16
    se = 256 + (fixed_maxx_x // 16)

    binary_data = bytearray()
    
    # Emit the number of points as a 16-bit signed integer in a separate file
    with open("points_count.bin", "wb") as f:
        f.write(struct.pack('<h', len(points)))
    
    for p in points:
        # 1. Scale to fixed point
        fx = int(round(p[0] * scale))
        fy = int(round(p[1] * scale))
        fz = int(round(p[2] * scale))
        
        # 2. Apply the transformation logic
        tx = se - (fx // 14)
        ty = (fy // 9) << 6
        tz = (fz // 9) << 6
        
        # 3. Swap Y and Z for the ASM layout: [X, Z, Y]
        final_points = [tx, tz, ty]
        
        for val in final_points:
            binary_data.extend(struct.pack('<h', val))

    with open("points.bin", "wb") as f:
        f.write(binary_data)
    
    print(f"[CODEGEN] Generated points.bin and points_count.bin with {len(points)} points.")
    print(f"[CODEGEN] Used Scale: {scale}, Computed Fixed MAXX: {fixed_maxx_x}, SE: {se}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 points_gen.py <scale_factor>")
        sys.exit(1)
    generate_points(sys.argv[1])
