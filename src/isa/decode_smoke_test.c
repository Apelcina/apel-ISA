#include "isa.h"
#include <stdio.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

int main(void) {
    /* ADD rd=3, rs=1, rt=2 (opcode 0x00, funct 0x00, shamt 0) */
    {
        uint32_t word = (1u << 22) | (2u << 18) | (3u << 14) | 0x00;
        decoded_instr_t d = decode(word);
        check("ADD format", d.format == FMT_R);
        check("ADD opcode", d.opcode == 0x00);
        check("ADD rs", d.rs == 1);
        check("ADD rt", d.rt == 2);
        check("ADD rd", d.rd == 3);
        check("ADD shamt", d.shamt == 0);
        check("ADD funct", d.funct == 0x00);
    }

    /* ADDI rt=5, rs=1, imm=100 (opcode 0x08) */
    {
        uint32_t word = (0x08u << 26) | (1u << 22) | (5u << 18) | 100u;
        decoded_instr_t d = decode(word);
        check("ADDI format", d.format == FMT_I);
        check("ADDI opcode", d.opcode == 0x08);
        check("ADDI rs", d.rs == 1);
        check("ADDI rt", d.rt == 5);
        check("ADDI imm", d.imm == 100);
    }

    /* J address=12345 (opcode 0x02) */
    {
        uint32_t word = (0x02u << 26) | 12345u;
        decoded_instr_t d = decode(word);
        check("J format", d.format == FMT_J);
        check("J opcode", d.opcode == 0x02);
        check("J address", d.address == 12345u);
    }

    /* Reserved opcode 0x01 - should decode as invalid, not crash */
    {
        uint32_t word = (0x01u << 26);
        decoded_instr_t d = decode(word);
        check("reserved opcode -> invalid", d.format == FMT_INVALID);
    }

    if (!failed) {
        printf("PASS: all decode cases correct\n");
    }
    return failed;
}
