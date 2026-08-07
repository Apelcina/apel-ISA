#include "isa.h"
#include <stdio.h>
#include <string.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

static void test_spot_checks(void) {
    const instr_info_t *add = lookup_mnemonic("ADD");
    check("ADD found", add != NULL);
    check("ADD format R", add->format == FMT_R);
    check("ADD funct", add->funct == 0x00);
    check("ADD operands", add->operands == OPERANDS_RD_RS_RT);

    const instr_info_t *sll = lookup_mnemonic("SLL");
    check("SLL operands is rd,rt,shamt (not rd,rs,rt)",
          sll->operands == OPERANDS_RD_RT_SHAMT);

    const instr_info_t *addi = lookup_mnemonic("ADDI");
    check("ADDI format I", addi->format == FMT_I);
    check("ADDI opcode", addi->opcode == 0x08);
    check("ADDI operands", addi->operands == OPERANDS_RT_RS_IMM);

    const instr_info_t *lw = lookup_mnemonic("LW");
    check("LW operands is memory syntax rt,imm(rs), distinct from ADDI's shape",
          lw->operands == OPERANDS_RT_IMM_RS);

    const instr_info_t *beq = lookup_mnemonic("BEQ");
    check("BEQ operands takes a label, not a raw imm",
          beq->operands == OPERANDS_RS_RT_LABEL);

    const instr_info_t *j = lookup_mnemonic("J");
    check("J format J", j->format == FMT_J);
    check("J opcode", j->opcode == 0x02);
    check("J operands", j->operands == OPERANDS_LABEL);

    const instr_info_t *halt = lookup_mnemonic("HALT");
    check("HALT takes no operands", halt->operands == OPERANDS_NONE);
}

static void test_unknown_mnemonic(void) {
    check("unknown mnemonic returns NULL", lookup_mnemonic("FOOBAR") == NULL);
    check("wrong case returns NULL (exact match only)",
          lookup_mnemonic("add") == NULL);
}

/* The mnemonic table and decode.c's opcode_format table are two
   independently maintained tables that both encode "what format is
   opcode X" - one by name, one by number. If they ever drift apart
   (e.g. a new instruction added to one but not the other), decode()
   would disagree with what the assembler thinks it just encoded. This
   builds a minimal word for every table entry and checks decode()
   reports the same format back. */
static void test_agrees_with_decode(void) {
    static const char *names[] = {
        "ADD",  "SUB", "AND",  "OR",   "XOR", "NOR", "SLL", "SRL",
        "SRA",  "SLT", "SLTU", "JR",   "JALR", "HALT", "SYSCALL", "BREAK",
        "BEQ",  "BNE", "BLT",  "BGE",  "ADDI", "SLTI", "SLTIU", "ANDI",
        "ORI",  "XORI", "LUI", "LB",   "LH",  "LW",  "LBU", "LHU",
        "SB",   "SH",  "SW",  "J",    "JAL",
    };
    size_t count = sizeof(names) / sizeof(names[0]);
    size_t i;

    for (i = 0; i < count; i++) {
        const instr_info_t *info = lookup_mnemonic(names[i]);
        if (!info) {
            printf("FAIL: %s missing from table\n", names[i]);
            failed = 1;
            continue;
        }

        uint32_t word = (info->opcode << 26);
        if (info->format == FMT_R) {
            word |= info->funct;
        }

        decoded_instr_t d = decode(word);
        if (d.format != info->format) {
            printf("FAIL: %s - mnemonic table says format %d, decode() says %d\n",
                   names[i], info->format, d.format);
            failed = 1;
        }
    }
}

int main(void) {
    test_spot_checks();
    test_unknown_mnemonic();
    test_agrees_with_decode();

    if (!failed) {
        printf("PASS: mnemonic table lookups correct and agree with decode()\n");
    }
    return failed;
}
