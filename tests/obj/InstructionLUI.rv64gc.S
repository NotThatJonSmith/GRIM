// LUI (load upper immediate) uses the same opcode as RV32I. LUI places the
// 20-bit U-immediate into bits 31–12 of register rd and places zero in the
// lowest 12 bits. The 32-bit result is sign-extended to 64 bits.
    lui a0, 0x00000
    lui a1, 0xfffff
    lui a2, 1
    lui a3, 0x80000
    lui a4, 0x7ffff
    lui a5, 0xa5a5a
    lui a6, 0x5a5a5
