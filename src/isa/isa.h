#ifndef APEL_ISA_H
#define APEL_ISA_H

#include <stdint.h>

typedef enum {
    FMT_INVALID = 0,
    FMT_R,
    FMT_I,
    FMT_J
} format_t;

/* Raw bit fields only - no sign extension, no semantics. Which fields are
   meaningful depends on `format`; unused fields are left zero. */
typedef struct {
    format_t format;
    uint32_t opcode;
    uint32_t rs;
    uint32_t rt;
    uint32_t rd;
    uint32_t shamt;
    uint32_t funct;
    uint32_t imm;     /* I-type, raw 18-bit value */
    uint32_t address; /* J-type, raw 26-bit value */
} decoded_instr_t;

decoded_instr_t decode(uint32_t word);

/* Sign-extend an 18-bit field (our I-type immediate width) to a full
   32-bit signed value. Shared by execute() (interpreting an operand)
   and the disassembler (printing one) - moved here once a second real
   consumer needed it, not before. */
int32_t sign_extend18(uint32_t imm);

/* Operand shapes, one per distinct assembly-text layout a mnemonic can
   have - not the same granularity as `format`. Several mnemonics share
   a format but need different operand parsing (e.g. ADD rd,rs,rt vs.
   SLL rd,rt,shamt are both R-type; LW rt,imm(rs) vs. ADDI rt,rs,imm are
   both I-type). A future disassembler needs this same information, to
   know which decoded fields are actually meaningful to print for a
   given mnemonic - hence living here rather than only in the
   assembler. */
typedef enum {
    OPERANDS_NONE,        /* HALT */
    OPERANDS_RD_RS_RT,    /* ADD rd, rs, rt */
    OPERANDS_RD_RT_SHAMT, /* SLL rd, rt, shamt */
    OPERANDS_RS,          /* JR rs */
    OPERANDS_RD_RS,       /* JALR rd, rs */
    OPERANDS_RT_RS_IMM,   /* ADDI rt, rs, imm */
    OPERANDS_RT_IMM,      /* LUI rt, imm */
    OPERANDS_RT_IMM_RS,   /* LW rt, imm(rs) - memory operand syntax */
    OPERANDS_RS_RT_LABEL, /* BEQ rs, rt, label */
    OPERANDS_LABEL        /* J label */
} operand_pattern_t;

typedef struct {
    const char *mnemonic;
    format_t format;
    uint32_t opcode;
    uint32_t funct; /* only meaningful when format == FMT_R */
    operand_pattern_t operands;
} instr_info_t;

/* Exact-match lookup (case-sensitive for now). Returns NULL if `name`
   isn't a recognized mnemonic. */
const instr_info_t *lookup_mnemonic(const char *name);

/* Reverse lookup for the disassembler: given a decoded encoding, find
   the matching table entry. funct is ignored unless format == FMT_R.
   Returns NULL for a genuinely unrecognized encoding (same cases
   decode()/execute() treat as invalid). */
const instr_info_t *lookup_by_encoding(format_t format, uint32_t opcode,
                                        uint32_t funct);

#endif
