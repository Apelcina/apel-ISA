#include "cpu.h"
#include "execute.h"
#include "isa.h"
#include <stdio.h>

static int failed = 0;

static void check(const char *label, int ok) {
    if (!ok) {
        printf("FAIL: %s\n", label);
        failed = 1;
    }
}

static void step(cpu_t *cpu, uint32_t word) {
    decoded_instr_t d = decode(word);
    execute(cpu, d);
}

int main(void) {
    cpu_t cpu;
    if (!cpu_init(&cpu)) {
        printf("FAIL: cpu_init allocation failed\n");
        return 1;
    }

    cpu_reg_write(&cpu, 1, 10); /* r1 = 10 */
    cpu_reg_write(&cpu, 2, 5);  /* r2 = 5 */

    /* ADD r3, r1, r2 -> r3 = 15, pc advances to 4 */
    step(&cpu, (1u << 22) | (2u << 18) | (3u << 14) | 0x00);
    check("ADD result", cpu_reg_read(&cpu, 3) == 15);
    check("ADD advances pc", cpu.pc == 4);

    /* SUB r4, r1, r2 -> r4 = 5, pc advances to 8 */
    step(&cpu, (1u << 22) | (2u << 18) | (4u << 14) | 0x01);
    check("SUB result", cpu_reg_read(&cpu, 4) == 5);
    check("SUB advances pc", cpu.pc == 8);

    /* ADDI r5, r1, 20 -> r5 = 30, pc advances to 12 */
    step(&cpu, (0x08u << 26) | (1u << 22) | (5u << 18) | 20u);
    check("ADDI positive result", cpu_reg_read(&cpu, 5) == 30);
    check("ADDI positive advances pc", cpu.pc == 12);

    /* ADDI r6, r1, -5 (encoded as 18-bit two's complement 0x3FFFB)
       -> r6 = 5, proves sign extension actually happened */
    step(&cpu, (0x08u << 26) | (1u << 22) | (6u << 18) | 0x3FFFBu);
    check("ADDI negative result", cpu_reg_read(&cpu, 6) == 5);
    check("ADDI negative advances pc", cpu.pc == 16);

    /* HALT -> halted becomes true, pc does NOT advance past it */
    step(&cpu, 0x0Du);
    check("HALT sets halted", cpu.halted == true);
    check("HALT does not advance pc", cpu.pc == 16);

    if (!failed) {
        printf("PASS: ADD/SUB/ADDI/HALT and pc advancement all correct\n");
    }

    cpu_destroy(&cpu);
    return failed;
}
