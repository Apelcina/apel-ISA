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
#define J_TYPE(opcode_, address_)                                            \
    (((uint32_t)(opcode_) << 26) | ((uint32_t)(address_) & 0x3FFFFFFu))

static void run_one(cpu_t *cpu, uint32_t word) {
    decoded_instr_t d = decode(word);
    execute(cpu, d);
}

static void test_j(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* pc has nonzero top bits, to prove the target really borrows them
       from pc+4 rather than from pc itself or from nowhere. Address
       field deliberately doesn't share a digit with pc+4's "+4" - that
       "4" is just "an instruction is 4 bytes," unrelated to where this
       jump actually lands, and picking a colliding value here made that
       easy to misread. */
    cpu.pc = 0x10000000;
    /* J to word-index 10 (byte offset 40 = 0x28) within that region */
    run_one(&cpu, J_TYPE(0x02, 10));
    check("J target inherits pc+4's top 4 bits", cpu.pc == 0x10000028);

    cpu_destroy(&cpu);
}

static void test_jal(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu.pc = 100;
    run_one(&cpu, J_TYPE(0x03, 10)); /* JAL to word-address 10 = byte 40 */
    check("JAL sets pc to target", cpu.pc == 40);
    check("JAL links r1 = pc+4", cpu_reg_read(&cpu, 1) == 104);

    cpu_destroy(&cpu);
}

static void test_jr_and_jalr(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu_reg_write(&cpu, 3, 200);
    cpu.pc = 50;
    run_one(&cpu, R_TYPE(3, 0, 0, 0x0B)); /* JR r3 */
    check("JR jumps to value in rs", cpu.pc == 200);

    cpu_reg_write(&cpu, 3, 300);
    cpu.pc = 60;
    run_one(&cpu, R_TYPE(3, 0, 7, 0x0C)); /* JALR r7, r3 */
    check("JALR jumps to value in rs", cpu.pc == 300);
    check("JALR links rd = pc+4", cpu_reg_read(&cpu, 7) == 64);

    cpu_destroy(&cpu);
}

static void test_loop_sum_with_real_jump(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* Same sum-1..5 program as branch_smoke_test.c, but the loop-back
       is now a real J instead of the BEQ r0,r0 stand-in.
       0:  ADDI r1, r0, 0
       4:  ADDI r2, r0, 1
       8:  ADDI r3, r0, 6
       12: BGE  r2, r3, +3       -> 28 (end)
       16: ADD  r1, r1, r2
       20: ADDI r2, r2, 1
       24: J    3                -> word-address 3 = byte 12 (loop check)
       28: HALT */
    uint32_t program[] = {
        I_TYPE(0x08, 0, 1, 0), I_TYPE(0x08, 0, 2, 1), I_TYPE(0x08, 0, 3, 6),
        I_TYPE(0x07, 2, 3, 3), R_TYPE(1, 2, 1, 0x00), I_TYPE(0x08, 2, 2, 1),
        J_TYPE(0x02, 3),       0x0Du,
    };
    size_t len = sizeof(program) / sizeof(program[0]);

    check("program fits", cpu_load_program(&cpu, program, len));
    cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("sum == 15, using a real J this time", cpu_reg_read(&cpu, 1) == 15);

    cpu_destroy(&cpu);
}

static void test_call_and_return(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* Demonstrates JAL/JR as a call/return pair. r1 is the link
       register (holds the return address) - deliberately kept separate
       from r2, the "function's" data register, since JAL always writes
       r1. Reusing r1 for both roles in the same program would silently
       clobber the return address - exactly the kind of conflict a real
       calling convention (phase 8) exists to prevent.

       0:  ADDI r2, r0, 10       r2 = 10
       4:  JAL  4 (word-addr)    r1 = 8 (return addr), pc = 16 (func)
       8:  ADD  r5, r0, r2       r5 = r2 (runs after return)
       12: HALT
       16: ADDI r2, r2, 1        func body: r2 += 1
       20: JR   r1               return to address in r1 (8) */
    uint32_t program[] = {
        I_TYPE(0x08, 0, 2, 10),
        J_TYPE(0x03, 4), /* word-address 4 = byte 16 */
        R_TYPE(0, 2, 5, 0x00),
        0x0Du,
        I_TYPE(0x08, 2, 2, 1),
        R_TYPE(1, 0, 0, 0x0B),
    };
    size_t len = sizeof(program) / sizeof(program[0]);

    check("program fits", cpu_load_program(&cpu, program, len));
    uint32_t steps = cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("function incremented r2 to 11", cpu_reg_read(&cpu, 2) == 11);
    check("r5 captured the post-return value", cpu_reg_read(&cpu, 5) == 11);
    check("returned to the right place before halting", cpu.pc == 12);
    check("ran exactly 6 steps (no infinite loop)", steps == 6);

    cpu_destroy(&cpu);
}

int main(void) {
    test_j();
    test_jal();
    test_jr_and_jalr();
    test_loop_sum_with_real_jump();
    test_call_and_return();

    if (!failed) {
        printf("PASS: J/JAL/JR/JALR correct, including a call/return pair\n");
    }
    return failed;
}
