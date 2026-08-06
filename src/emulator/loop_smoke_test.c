#include "cpu.h"
#include "loop.h"
#include <stdio.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

int main(void) {
    cpu_t cpu;
    if (!cpu_init(&cpu)) {
        printf("FAIL: cpu_init allocation failed\n");
        return 1;
    }

    /* 0:  ADDI r1, r0, 10   r1 = 10
       4:  ADDI r2, r0, 5    r2 = 5
       8:  ADD  r3, r1, r2   r3 = 15
       12: SUB  r4, r1, r2   r4 = 5
       16: HALT */
    uint32_t program[] = {
        (0x08u << 26) | (0u << 22) | (1u << 18) | 10u,
        (0x08u << 26) | (0u << 22) | (2u << 18) | 5u,
        (1u << 22) | (2u << 18) | (3u << 14) | 0x00u,
        (1u << 22) | (2u << 18) | (4u << 14) | 0x01u,
        0x0Du,
    };
    size_t program_len = sizeof(program) / sizeof(program[0]);

    check("program fits in memory", cpu_load_program(&cpu, program, program_len));

    uint32_t steps = cpu_run(&cpu, 100);

    check("ran exactly 5 steps", steps == 5);
    check("halted after HALT", cpu.halted == true);
    check("pc stopped at HALT's address", cpu.pc == 16);
    check("r1 == 10", cpu_reg_read(&cpu, 1) == 10);
    check("r2 == 5", cpu_reg_read(&cpu, 2) == 5);
    check("r3 == 15 (10+5)", cpu_reg_read(&cpu, 3) == 15);
    check("r4 == 5 (10-5)", cpu_reg_read(&cpu, 4) == 5);

    if (!failed) {
        printf("PASS: full program loaded, fetched from memory, and run to HALT\n");
    }

    cpu_destroy(&cpu);
    return failed;
}
