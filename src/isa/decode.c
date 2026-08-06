#include "isa.h"

/* Mirrors the opcode table in docs/isa-spec.md exactly. Any opcode not
   listed here defaults to FMT_INVALID (enum value 0). */
static const format_t opcode_format[64] = {
    [0x00] = FMT_R, /* R-type, see funct table */

    [0x02] = FMT_J, /* J */
    [0x03] = FMT_J, /* JAL */

    [0x04] = FMT_I, /* BEQ */
    [0x05] = FMT_I, /* BNE */
    [0x06] = FMT_I, /* BLT */
    [0x07] = FMT_I, /* BGE */
    [0x08] = FMT_I, /* ADDI */
    [0x09] = FMT_I, /* SLTI */
    [0x0A] = FMT_I, /* SLTIU */
    [0x0C] = FMT_I, /* ANDI */
    [0x0D] = FMT_I, /* ORI */
    [0x0E] = FMT_I, /* XORI */
    [0x0F] = FMT_I, /* LUI */

    [0x20] = FMT_I, /* LB */
    [0x21] = FMT_I, /* LH */
    [0x23] = FMT_I, /* LW */
    [0x24] = FMT_I, /* LBU */
    [0x25] = FMT_I, /* LHU */
    [0x28] = FMT_I, /* SB */
    [0x29] = FMT_I, /* SH */
    [0x2B] = FMT_I, /* SW */
};

decoded_instr_t decode(uint32_t word) {
    decoded_instr_t d = {0};

    d.opcode = (word >> 26) & 0x3F;
    d.format = opcode_format[d.opcode];

    switch (d.format) {
    case FMT_R:
        d.rs = (word >> 22) & 0xF;
        d.rt = (word >> 18) & 0xF;
        d.rd = (word >> 14) & 0xF;
        d.shamt = (word >> 6) & 0xFF;
        d.funct = word & 0x3F;
        break;
    case FMT_I:
        d.rs = (word >> 22) & 0xF;
        d.rt = (word >> 18) & 0xF;
        d.imm = word & 0x3FFFF;
        break;
    case FMT_J:
        d.address = word & 0x3FFFFFF;
        break;
    case FMT_INVALID:
        break;
    }

    return d;
}
