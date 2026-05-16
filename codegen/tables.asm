SECTION data_user

PUBLIC _mask_table
align 256
_mask_table:
    BINARY "codegen/mask.bin"

PUBLIC _recip_table
align 256
_recip_table:
    BINARY "codegen/recip.bin"

PUBLIC _scr_ofs_lo
align 256
_scr_ofs_lo:
    BINARY "codegen/scr_ofs_lo.bin"

PUBLIC _scr_ofs_hi
align 256
_scr_ofs_hi:
    BINARY "codegen/scr_ofs_hi.bin"

PUBLIC _g_points_count
align 2
_g_points_count:
    BINARY "codegen/points_count.bin"

PUBLIC _g_points_raw
align 2
_g_points_raw:
    BINARY "codegen/points.bin"

PUBLIC _g_sincos
align 2
_g_sincos:
    BINARY "codegen/sincos.bin"

PUBLIC _x_shift_table
align 256
_x_shift_table:
    BINARY "codegen/x_shift.bin"
