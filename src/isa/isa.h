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

#endif
