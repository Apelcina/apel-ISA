#include "cpu.h"
#include "dump.h"
#include "loop.h"
#include <stdio.h>

#define I_TYPE(opcode_, rs_, rt_, imm_)                                      \
    (((uint32_t)(opcode_) << 26) | ((uint32_t)(rs_) << 22) |                 \
     ((uint32_t)(rt_) << 18) | ((uint32_t)(imm_) & 0x3FFFFu))
#define R_TYPE(rs_, rt_, rd_, funct_)                                        \
    (((uint32_t)(rs_) << 22) | ((uint32_t)(rt_) << 18) |                     \
     ((uint32_t)(rd_) << 14) | (uint32_t)(funct_))

int main(void) {
    cpu_t cpu;
    if (!cpu_init(&cpu)) {
        printf("cpu_init failed\n");
        return 1;
    }

    /* sum 1..5, using J instead of the BEQ r0,r0 stand-in */
    uint32_t program[] = {
        I_TYPE(0x08, 0, 1, 0), I_TYPE(0x08, 0, 2, 1), I_TYPE(0x08, 0, 3, 6),
        I_TYPE(0x07, 2, 3, 3), R_TYPE(1, 2, 1, 0x00), I_TYPE(0x08, 2, 2, 1),
        (0x02u << 26) | 3u,    0x0Du,
    };
    size_t len = sizeof(program) / sizeof(program[0]);
    cpu_load_program(&cpu, program, len);

    printf("--- before running ---\n");
    dump_registers(&cpu, stdout);

    cpu_run(&cpu, 100);

    printf("\n--- after running (r1 should hold 15) ---\n");
    dump_registers(&cpu, stdout);

    printf("\n--- program bytes in memory ---\n");
    dump_memory(&cpu, stdout, 0, (uint32_t)(len * 4));

    cpu_destroy(&cpu);
    return 0;
}
