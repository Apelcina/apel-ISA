#include "execute.h"

/* Sign-extend an 18-bit field (our I-type immediate width) to a full
   32-bit signed value. */
static int32_t sign_extend18(uint32_t imm) {
    return ((int32_t)(imm << 14)) >> 14;
}

/* If condition is true, sets cpu->pc to the branch target and returns
   true (caller must skip the default +4). Otherwise leaves pc alone and
   returns false. Shifting happens in unsigned space on purpose - left-
   shifting a negative signed int is undefined behavior in C, whereas
   unsigned left shift always just wraps modulo 2^32, which is exactly
   the two's-complement bit pattern we want for a negative offset. */
static bool take_branch_if(cpu_t *cpu, decoded_instr_t d, bool condition) {
    if (!condition) {
        return false;
    }
    uint32_t offset = (uint32_t)sign_extend18(d.imm) << 2;
    cpu->pc = cpu->pc + 4 + offset;
    return true;
}

/* Returns true if it set pc itself (jump/branch/JR/JALR) - caller must
   then skip the default +4. */
static bool execute_r(cpu_t *cpu, decoded_instr_t d) {
    uint32_t rs = cpu_reg_read(cpu, d.rs);
    uint32_t rt = cpu_reg_read(cpu, d.rt);

    switch (d.funct) {
    case 0x00: /* ADD */
        cpu_reg_write(cpu, d.rd, rs + rt);
        return false;
    case 0x01: /* SUB */
        cpu_reg_write(cpu, d.rd, rs - rt);
        return false;
    case 0x0B: /* JR */
        cpu->pc = rs;
        return true;
    case 0x0C: /* JALR */
        cpu_reg_write(cpu, d.rd, cpu->pc + 4);
        cpu->pc = rs;
        return true;
    case 0x0D: /* HALT */
        cpu->halted = true;
        return false;
    default:
        /* Not implemented yet (or genuinely reserved) - real illegal-
           instruction handling is a later design decision. Stop rather
           than silently doing nothing. */
        cpu->halted = true;
        return false;
    }
}

/* Returns true if it set pc itself (a taken branch) - caller must then
   skip the default +4. */
static bool execute_i(cpu_t *cpu, decoded_instr_t d) {
    uint32_t rs = cpu_reg_read(cpu, d.rs);

    switch (d.opcode) {
    case 0x04: /* BEQ */
        return take_branch_if(cpu, d, rs == cpu_reg_read(cpu, d.rt));
    case 0x05: /* BNE */
        return take_branch_if(cpu, d, rs != cpu_reg_read(cpu, d.rt));
    case 0x06: /* BLT */
        return take_branch_if(cpu, d, (int32_t)rs < (int32_t)cpu_reg_read(cpu, d.rt));
    case 0x07: /* BGE */
        return take_branch_if(cpu, d, (int32_t)rs >= (int32_t)cpu_reg_read(cpu, d.rt));
    case 0x08: /* ADDI */
        cpu_reg_write(cpu, d.rt, rs + (uint32_t)sign_extend18(d.imm));
        return false;
    default:
        cpu->halted = true;
        return false;
    }
}

/* J/JAL target: borrows its top 4 bits from pc+4 (the address *after*
   the jump), not from the jump instruction's own pc - matches the
   pseudo-direct addressing scheme in docs/isa-spec.md. */
static bool execute_j(cpu_t *cpu, decoded_instr_t d) {
    uint32_t next_pc = cpu->pc + 4;
    uint32_t target = (next_pc & 0xF0000000u) | (d.address << 2);

    switch (d.opcode) {
    case 0x02: /* J */
        cpu->pc = target;
        return true;
    case 0x03: /* JAL */
        cpu_reg_write(cpu, 1, next_pc); /* r1 = link register (placeholder, see spec) */
        cpu->pc = target;
        return true;
    default:
        cpu->halted = true;
        return false;
    }
}

void execute(cpu_t *cpu, decoded_instr_t d) {
    bool pc_overridden = false;

    switch (d.format) {
    case FMT_R:
        pc_overridden = execute_r(cpu, d);
        break;
    case FMT_I:
        pc_overridden = execute_i(cpu, d);
        break;
    case FMT_J:
        pc_overridden = execute_j(cpu, d);
        break;
    case FMT_INVALID:
        cpu->halted = true;
        break;
    }

    if (!cpu->halted && !pc_overridden) {
        cpu->pc += 4;
    }
}
