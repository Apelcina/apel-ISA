#include "isa.h"
#include <string.h>

/* Mirrors the opcode and funct tables in docs/isa-spec.md exactly - one
   row per instruction, transcribed directly from the spec rather than
   re-derived. */
static const instr_info_t instructions[] = {
    /* R-type (opcode is always 0x00, funct distinguishes) */
    {"ADD", FMT_R, 0x00, 0x00, OPERANDS_RD_RS_RT},
    {"SUB", FMT_R, 0x00, 0x01, OPERANDS_RD_RS_RT},
    {"AND", FMT_R, 0x00, 0x02, OPERANDS_RD_RS_RT},
    {"OR", FMT_R, 0x00, 0x03, OPERANDS_RD_RS_RT},
    {"XOR", FMT_R, 0x00, 0x04, OPERANDS_RD_RS_RT},
    {"NOR", FMT_R, 0x00, 0x05, OPERANDS_RD_RS_RT},
    {"SLL", FMT_R, 0x00, 0x06, OPERANDS_RD_RT_SHAMT},
    {"SRL", FMT_R, 0x00, 0x07, OPERANDS_RD_RT_SHAMT},
    {"SRA", FMT_R, 0x00, 0x08, OPERANDS_RD_RT_SHAMT},
    {"SLT", FMT_R, 0x00, 0x09, OPERANDS_RD_RS_RT},
    {"SLTU", FMT_R, 0x00, 0x0A, OPERANDS_RD_RS_RT},
    {"JR", FMT_R, 0x00, 0x0B, OPERANDS_RS},
    {"JALR", FMT_R, 0x00, 0x0C, OPERANDS_RD_RS},
    {"HALT", FMT_R, 0x00, 0x0D, OPERANDS_NONE},
    {"SYSCALL", FMT_R, 0x00, 0x3E, OPERANDS_NONE},
    {"BREAK", FMT_R, 0x00, 0x3F, OPERANDS_NONE},

    /* I-type */
    {"BEQ", FMT_I, 0x04, 0, OPERANDS_RS_RT_LABEL},
    {"BNE", FMT_I, 0x05, 0, OPERANDS_RS_RT_LABEL},
    {"BLT", FMT_I, 0x06, 0, OPERANDS_RS_RT_LABEL},
    {"BGE", FMT_I, 0x07, 0, OPERANDS_RS_RT_LABEL},
    {"ADDI", FMT_I, 0x08, 0, OPERANDS_RT_RS_IMM},
    {"SLTI", FMT_I, 0x09, 0, OPERANDS_RT_RS_IMM},
    {"SLTIU", FMT_I, 0x0A, 0, OPERANDS_RT_RS_IMM},
    {"ANDI", FMT_I, 0x0C, 0, OPERANDS_RT_RS_IMM},
    {"ORI", FMT_I, 0x0D, 0, OPERANDS_RT_RS_IMM},
    {"XORI", FMT_I, 0x0E, 0, OPERANDS_RT_RS_IMM},
    {"LUI", FMT_I, 0x0F, 0, OPERANDS_RT_IMM},
    {"LB", FMT_I, 0x20, 0, OPERANDS_RT_IMM_RS},
    {"LH", FMT_I, 0x21, 0, OPERANDS_RT_IMM_RS},
    {"LW", FMT_I, 0x23, 0, OPERANDS_RT_IMM_RS},
    {"LBU", FMT_I, 0x24, 0, OPERANDS_RT_IMM_RS},
    {"LHU", FMT_I, 0x25, 0, OPERANDS_RT_IMM_RS},
    {"SB", FMT_I, 0x28, 0, OPERANDS_RT_IMM_RS},
    {"SH", FMT_I, 0x29, 0, OPERANDS_RT_IMM_RS},
    {"SW", FMT_I, 0x2B, 0, OPERANDS_RT_IMM_RS},

    /* J-type */
    {"J", FMT_J, 0x02, 0, OPERANDS_LABEL},
    {"JAL", FMT_J, 0x03, 0, OPERANDS_LABEL},
};

#define INSTRUCTION_COUNT (sizeof(instructions) / sizeof(instructions[0]))

const instr_info_t *lookup_mnemonic(const char *name) {
    size_t i;
    for (i = 0; i < INSTRUCTION_COUNT; i++) {
        if (strcmp(instructions[i].mnemonic, name) == 0) {
            return &instructions[i];
        }
    }
    return NULL;
}

const instr_info_t *lookup_by_encoding(format_t format, uint32_t opcode,
                                        uint32_t funct) {
    size_t i;
    for (i = 0; i < INSTRUCTION_COUNT; i++) {
        if (instructions[i].format != format || instructions[i].opcode != opcode) {
            continue;
        }
        if (format == FMT_R && instructions[i].funct != funct) {
            continue;
        }
        return &instructions[i];
    }
    return NULL;
}
