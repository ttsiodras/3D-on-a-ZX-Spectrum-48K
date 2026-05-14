SECTION data_user

PUBLIC _mask_table
align 256
_mask_table:
    BINARY "mask.bin"

PUBLIC _scr_ofs
align 256
_scr_ofs:
    BINARY "scr_ofs.bin"

PUBLIC _g_points_count
align 2
_g_points_count:
    BINARY "points_count.bin"

PUBLIC _g_points_raw
align 2
_g_points_raw:
    BINARY "points.bin"

PUBLIC _g_sincos
align 2
_g_sincos:
    BINARY "sincos.bin"
