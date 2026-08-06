#include "execute.h"

/* Sign-extend an 18-bit field (our I-type immediate width) to a full
   32-bit signed value. Will likely get hoisted somewhere shared once
   branches need the same trick for their offset field. */
static int32_t sign_extend18(uint32_t imm) {
    return ((int32_t)(imm << 14)) >> 14;
}

static void execute_r(cpu_t *cpu, decoded_instr_t d) {
    uint32_t rs = cpu_reg_read(cpu, d.rs);
    uint32_t rt = cpu_reg_read(cpu, d.rt);

    switch (d.funct) {
    case 0x00: /* ADD */
        cpu_reg_write(cpu, d.rd, rs + rt);
        break;
    case 0x01: /* SUB */
        cpu_reg_write(cpu, d.rd, rs - rt);
        break;
    case 0x0D: /* HALT */
        cpu->halted = true;
        break;
    default:
        /* Not implemented yet (or genuinely reserved) - real illegal-
           instruction handling is a later design decision. Stop rather
           than silently doing nothing. */
        cpu->halted = true;
        break;
    }
}

static void execute_i(cpu_t *cpu, decoded_instr_t d) {
    uint32_t rs = cpu_reg_read(cpu, d.rs);

    switch (d.opcode) {
    case 0x08: /* ADDI */
        cpu_reg_write(cpu, d.rt, rs + (uint32_t)sign_extend18(d.imm));
        break;
    default:
        cpu->halted = true;
        break;
    }
}

void execute(cpu_t *cpu, decoded_instr_t d) {
    switch (d.format) {
    case FMT_R:
        execute_r(cpu, d);
        break;
    case FMT_I:
        execute_i(cpu, d);
        break;
    case FMT_J:
    case FMT_INVALID:
        cpu->halted = true;
        break;
    }

    if (!cpu->halted) {
        cpu->pc += 4;
    }
}
