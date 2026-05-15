import struct
import sys
import math

def generate_sphere(scale_factor):
    # We use the same scale as the statue for consistency
    scale = float(scale_factor)
    
    # Original Statue Range:
    # X: -0.285 to +0.275
    # Y: -0.405 to +0.373
    # Z: -0.501 to +0.501
    # We will create a sphere with radius 0.5 centered at (0,0,0)
    # to occupy a similar volume of space.
    radius = 0.45
    
    # To generate a uniform distribution of points on a sphere:
    # Using the Fibonacci Sphere algorithm for a visually pleasing distribution
    num_points = 153 # Match the original point count for performance consistency
    points = []
    
    phi = math.pi * (3. - math.sqrt(5.)) # golden angle in radians

    for i in range(num_points):
        y = 1 - (i / float(num_points - 1)) * 2  # y goes from 1 to -1
        radius_at_y = math.sqrt(1 - y * y)       # radius at y
        theta = phi * i                          # golden angle increment

        x = math.cos(theta) * radius_at_y
        z = math.sin(theta) * radius_at_y
        
        # Scale the sphere to match the statue's general size
        points.append([x * radius, y * radius, z * radius])

    # The original code calculated MAXX to determine the SE (Screen Offset)
    # SE = 256 + (fixed_maxx_x // 16)
    fixed_maxx_x = 0
    for p in points:
        val = abs(int(round(p[0] * scale)))
        if val > fixed_maxx_x:
            fixed_maxx_x = val
            
    # Use this fixed_maxx_x to calculate SE, matching the original logic
    # SE = 256 + MAXX / 16
    se = 256 + (fixed_maxx_x // 16)

    binary_data = bytearray()
    
    # Emit the number of points
    with open("points_count.bin", "wb") as f:
        f.write(struct.pack('<h', len(points)))
    
    for p in points:
        # 1. Scale to fixed point
        fx = int(round(p[0] * scale))
        fy = int(round(p[1] * scale))
        fz = int(round(p[2] * scale))
        
        # 2. Apply the transformation logic from the original points_gen.py
        tx = se - (fx // 14)
        ty = (fy // 9) << 6
        tz = (fz // 9) << 6
        
        # 3. Swap Y and Z for the ASM layout: [X, Z, Y]
        final_points = [tx, tz, ty]
        
        for val in final_points:
            binary_data.extend(struct.pack('<h', val))

    with open("points.bin", "wb") as f:
        f.write(binary_data)
    
    print(f"[CODEGEN] Generated sphere points.bin and points_count.bin with {len(points)} points.")
    print(f"[CODEGEN] Used Scale: {scale}, Computed Fixed MAXX: {fixed_maxx_x}, SE: {se}")

if __name__ == "__main__":
    generate_sphere(8060)
