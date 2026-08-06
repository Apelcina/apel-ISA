#include "cpu.h"
#include <stdlib.h>
#include <string.h>

bool cpu_init(cpu_t *cpu) {
    cpu->mem = malloc(APEL_MEM_SIZE);
    if (!cpu->mem) {
        return false;
    }
    cpu->mem_size = APEL_MEM_SIZE;
    cpu_reset(cpu);
    return true;
}

void cpu_destroy(cpu_t *cpu) {
    free(cpu->mem);
    cpu->mem = NULL;
    cpu->mem_size = 0;
}

void cpu_reset(cpu_t *cpu) {
    memset(cpu->regs, 0, sizeof(cpu->regs));
    cpu->pc = 0;
    memset(cpu->mem, 0, cpu->mem_size);
    cpu->halted = false;
}

uint32_t cpu_reg_read(const cpu_t *cpu, unsigned index) {
    return cpu->regs[index];
}

void cpu_reg_write(cpu_t *cpu, unsigned index, uint32_t value) {
    if (index != 0) {
        cpu->regs[index] = value;
    }
}
