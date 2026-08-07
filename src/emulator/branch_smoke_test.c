#include "cpu.h"
#include "execute.h"
#include "isa.h"
#include "loop.h"
#include <stdio.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

#define R_TYPE(rs_, rt_, rd_, funct_)                                        \
    (((uint32_t)(rs_) << 22) | ((uint32_t)(rt_) << 18) |                     \
     ((uint32_t)(rd_) << 14) | (uint32_t)(funct_))
#define I_TYPE(opcode_, rs_, rt_, imm_)                                      \
    (((uint32_t)(opcode_) << 26) | ((uint32_t)(rs_) << 22) |                 \
     ((uint32_t)(rt_) << 18) | ((uint32_t)(imm_) & 0x3FFFFu))

static void run_one(cpu_t *cpu, uint32_t word) {
    decoded_instr_t d = decode(word);
    execute(cpu, d);
}

static void test_branch_taken_and_not_taken(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu_reg_write(&cpu, 1, 5);
    cpu_reg_write(&cpu, 2, 5);
    cpu_reg_write(&cpu, 3, 9);

    /* BEQ r1, r2, +3 (equal -> taken) */
    run_one(&cpu, I_TYPE(0x04, 1, 2, 3));
    check("BEQ taken jumps to pc+4+3*4", cpu.pc == 0 + 4 + 12);

    cpu.pc = 0;
    /* BEQ r1, r3, +3 (not equal -> not taken, default +4) */
    run_one(&cpu, I_TYPE(0x04, 1, 3, 3));
    check("BEQ not taken advances by 4", cpu.pc == 4);

    cpu.pc = 0;
    /* BNE r1, r3, +2 (not equal -> taken) */
    run_one(&cpu, I_TYPE(0x05, 1, 3, 2));
    check("BNE taken", cpu.pc == 0 + 4 + 8);

    cpu.pc = 0;
    /* BLT r1(5), r3(9), +1 (5 < 9 -> taken) */
    run_one(&cpu, I_TYPE(0x06, 1, 3, 1));
    check("BLT taken (signed less-than)", cpu.pc == 0 + 4 + 4);

    cpu.pc = 0;
    /* BGE r3(9), r1(5), +1 (9 >= 5 -> taken) */
    run_one(&cpu, I_TYPE(0x07, 3, 1, 1));
    check("BGE taken", cpu.pc == 0 + 4 + 4);

    cpu.pc = 8;
    /* BEQ r1, r2, -2 (equal -> taken, negative offset goes backward) */
    run_one(&cpu, I_TYPE(0x04, 1, 2, (uint32_t)-2));
    check("BEQ negative offset goes backward", cpu.pc == 8 + 4 - 8);

    cpu_destroy(&cpu);
}

static void test_loop_sum_1_to_5(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* r1 = sum, r2 = i, r3 = limit (exclusive)
       0:  ADDI r1, r0, 0
       4:  ADDI r2, r0, 1
       8:  ADDI r3, r0, 6
       12: BGE  r2, r3, +3      -> end (offset: 28 = 12+4+3*4)
       16: ADD  r1, r1, r2
       20: ADDI r2, r2, 1
       24: BEQ  r0, r0, -4      -> loop (offset: 12 = 24+4-4*4), unconditional
                                    since r0 always equals r0 - we don't have
                                    a real jump instruction yet
       28: HALT (end:) */
    uint32_t program[] = {
        I_TYPE(0x08, 0, 1, 0),
        I_TYPE(0x08, 0, 2, 1),
        I_TYPE(0x08, 0, 3, 6),
        I_TYPE(0x07, 2, 3, 3),
        R_TYPE(1, 2, 1, 0x00),
        I_TYPE(0x08, 2, 2, 1),
        I_TYPE(0x04, 0, 0, (uint32_t)-4),
        0x0Du,
    };
    size_t program_len = sizeof(program) / sizeof(program[0]);

    check("program fits", cpu_load_program(&cpu, program, program_len));
    uint32_t steps = cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("sum == 15 (1+2+3+4+5)", cpu_reg_read(&cpu, 1) == 15);
    check("i == 6 at exit", cpu_reg_read(&cpu, 2) == 6);
    check("loop actually iterated (>5 steps)", steps > 5);
    printf("  (loop took %u steps)\n", steps);

    cpu_destroy(&cpu);
}

int main(void) {
    test_branch_taken_and_not_taken();
    test_loop_sum_1_to_5();

    if (!failed) {
        printf("PASS: BEQ/BNE/BLT/BGE and a real loop program both correct\n");
    }
    return failed;
}
