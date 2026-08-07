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

static void test_word_round_trip(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu_reg_write(&cpu, 1, 100);        /* base address */
    cpu_reg_write(&cpu, 2, 0xCAFEF00D); /* data to store */

    run_one(&cpu, I_TYPE(0x2B, 1, 2, 0)); /* SW r2, 0(r1) */
    run_one(&cpu, I_TYPE(0x23, 1, 3, 0)); /* LW r3, 0(r1) */

    check("SW/LW round-trips a word", cpu_reg_read(&cpu, 3) == 0xCAFEF00D);

    cpu_destroy(&cpu);
}

static void test_byte_sign_vs_zero_extend(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu_reg_write(&cpu, 1, 200);
    cpu_reg_write(&cpu, 2, 0xFF); /* just one byte's worth: -1 or 255 */

    run_one(&cpu, I_TYPE(0x28, 1, 2, 0)); /* SB r2, 0(r1) */
    run_one(&cpu, I_TYPE(0x20, 1, 3, 0)); /* LB r3, 0(r1) - signed */
    run_one(&cpu, I_TYPE(0x24, 1, 4, 0)); /* LBU r4, 0(r1) - unsigned */

    check("LB sign-extends 0xFF to -1 (0xFFFFFFFF)",
          cpu_reg_read(&cpu, 3) == 0xFFFFFFFFu);
    check("LBU zero-extends 0xFF to 255", cpu_reg_read(&cpu, 4) == 0xFFu);

    cpu_destroy(&cpu);
}

static void test_halfword_sign_vs_zero_extend(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    cpu_reg_write(&cpu, 1, 300);
    cpu_reg_write(&cpu, 2, 0xFFFF); /* -1 or 65535, depending on extension */

    run_one(&cpu, I_TYPE(0x29, 1, 2, 0)); /* SH r2, 0(r1) */
    run_one(&cpu, I_TYPE(0x21, 1, 3, 0)); /* LH r3, 0(r1) - signed */
    run_one(&cpu, I_TYPE(0x25, 1, 4, 0)); /* LHU r4, 0(r1) - unsigned */

    check("LH sign-extends 0xFFFF to -1 (0xFFFFFFFF)",
          cpu_reg_read(&cpu, 3) == 0xFFFFFFFFu);
    check("LHU zero-extends 0xFFFF to 65535", cpu_reg_read(&cpu, 4) == 0xFFFFu);

    cpu_destroy(&cpu);
}

static void test_store_truncates_not_corrupts_neighbors(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* Prime 4 bytes with a known pattern, then SB only the first byte
       with a wide register value - the other 3 bytes must survive
       untouched, proving SB only ever writes its own byte. */
    cpu_reg_write(&cpu, 1, 400);
    cpu_reg_write(&cpu, 2, 0x11223344);
    run_one(&cpu, I_TYPE(0x2B, 1, 2, 0)); /* SW r2, 0(r1): prime with pattern */

    cpu_reg_write(&cpu, 3, 0xFFFFFF78); /* only the low byte (0x78) should land */
    run_one(&cpu, I_TYPE(0x28, 1, 3, 0)); /* SB r3, 0(r1) */

    run_one(&cpu, I_TYPE(0x23, 1, 4, 0)); /* LW r4, 0(r1) - read the whole word back */
    check("SB only overwrites its own byte, neighbors survive",
          cpu_reg_read(&cpu, 4) == 0x11223378u);

    cpu_destroy(&cpu);
}

static void test_out_of_bounds_halts(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* Address well beyond the 1 MiB buffer */
    cpu_reg_write(&cpu, 1, 0x00200000u);
    run_one(&cpu, I_TYPE(0x23, 1, 2, 0)); /* LW r2, 0(r1) */
    check("plainly out-of-bounds load halts", cpu.halted == true);

    cpu_destroy(&cpu);
}

static void test_out_of_bounds_wraparound(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* addr = 0xFFFFFFFE, width 4: a naive 32-bit "addr + 4 > mem_size"
       check wraps (0xFFFFFFFE + 4 overflows to 0x2), which is LESS than
       mem_size and would wrongly look in-bounds. The 64-bit intermediate
       in in_bounds() must catch this correctly regardless. */
    cpu_reg_write(&cpu, 1, 0xFFFFFFFEu);
    run_one(&cpu, I_TYPE(0x23, 1, 2, 0)); /* LW r2, 0(r1) */
    check("near-UINT32_MAX address doesn't wrap past the bounds check",
          cpu.halted == true);

    cpu_destroy(&cpu);
}

static void test_program_copies_via_memory(void) {
    cpu_t cpu;
    cpu_init(&cpu);

    /* r1 = 0x1000 (source base), r2 = 0x2000 (dest base)
       0:  ADDI r1, r0, 0x100      (small values only - imm is limited here)
       4:  ADDI r2, r0, 0x200
       8:  ADDI r3, r0, 42
       12: SW   r3, 0(r1)          mem[0x100] = 42
       16: LW   r4, 0(r1)          r4 = mem[0x100]
       20: SW   r4, 0(r2)          mem[0x200] = r4 (copy)
       24: LW   r5, 0(r2)          r5 = mem[0x200]
       28: HALT */
    uint32_t program[] = {
        I_TYPE(0x08, 0, 1, 0x100), I_TYPE(0x08, 0, 2, 0x200),
        I_TYPE(0x08, 0, 3, 42),    I_TYPE(0x2B, 1, 3, 0),
        I_TYPE(0x23, 1, 4, 0),     I_TYPE(0x2B, 2, 4, 0),
        I_TYPE(0x23, 2, 5, 0),     0x0Du,
    };
    size_t len = sizeof(program) / sizeof(program[0]);

    check("program fits", cpu_load_program(&cpu, program, len));
    cpu_run(&cpu, 100);

    check("halted", cpu.halted == true);
    check("value survived a memory-to-memory copy", cpu_reg_read(&cpu, 5) == 42);

    cpu_destroy(&cpu);
}

int main(void) {
    test_word_round_trip();
    test_byte_sign_vs_zero_extend();
    test_halfword_sign_vs_zero_extend();
    test_store_truncates_not_corrupts_neighbors();
    test_out_of_bounds_halts();
    test_out_of_bounds_wraparound();
    test_program_copies_via_memory();

    if (!failed) {
        printf("PASS: loads/stores, sign/zero extension, truncation, and "
               "bounds checking all correct\n");
    }
    return failed;
}
