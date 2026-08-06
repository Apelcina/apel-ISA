#include "cpu.h"
#include <stdio.h>

int main(void) {
    cpu_t cpu;
    if (!cpu_init(&cpu)) {
        printf("FAIL: cpu_init allocation failed\n");
        return 1;
    }

    if (cpu.pc != 0 || cpu.halted != false) {
        printf("FAIL: init didn't zero pc/halted\n");
        return 1;
    }

    cpu_reg_write(&cpu, 5, 0xDEADBEEF);
    if (cpu_reg_read(&cpu, 5) != 0xDEADBEEF) {
        printf("FAIL: normal register write/read didn't round-trip\n");
        return 1;
    }

    cpu_reg_write(&cpu, 0, 0x12345678);
    if (cpu_reg_read(&cpu, 0) != 0) {
        printf("FAIL: r0 write should be discarded, read %u\n", cpu_reg_read(&cpu, 0));
        return 1;
    }

    printf("PASS: init, register read/write, r0-is-zero all behave correctly\n");
    cpu_destroy(&cpu);
    return 0;
}
