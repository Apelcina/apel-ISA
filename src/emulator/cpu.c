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

/* addr comes from arbitrary register+immediate arithmetic at runtime, so
   it can legitimately be anything up to UINT32_MAX. Checking
   "addr + width > mem_size" directly in 32-bit arithmetic would wrap
   around for an addr near UINT32_MAX, making an obviously-out-of-bounds
   access look in-bounds. Widening to uint64_t for the comparison avoids
   that wraparound entirely. */
static bool in_bounds(const cpu_t *cpu, uint32_t addr, uint32_t width) {
    return (uint64_t)addr + width <= cpu->mem_size;
}

uint32_t cpu_mem_read8(cpu_t *cpu, uint32_t addr) {
    if (!in_bounds(cpu, addr, 1)) {
        cpu->halted = true;
        return 0;
    }
    return cpu->mem[addr];
}

uint32_t cpu_mem_read16(cpu_t *cpu, uint32_t addr) {
    if (!in_bounds(cpu, addr, 2)) {
        cpu->halted = true;
        return 0;
    }
    return (uint32_t)cpu->mem[addr] | ((uint32_t)cpu->mem[addr + 1] << 8);
}

uint32_t cpu_mem_read32(cpu_t *cpu, uint32_t addr) {
    if (!in_bounds(cpu, addr, 4)) {
        cpu->halted = true;
        return 0;
    }
    return (uint32_t)cpu->mem[addr] | ((uint32_t)cpu->mem[addr + 1] << 8) |
           ((uint32_t)cpu->mem[addr + 2] << 16) |
           ((uint32_t)cpu->mem[addr + 3] << 24);
}

void cpu_mem_write8(cpu_t *cpu, uint32_t addr, uint32_t value) {
    if (!in_bounds(cpu, addr, 1)) {
        cpu->halted = true;
        return;
    }
    cpu->mem[addr] = (uint8_t)(value & 0xFF);
}

void cpu_mem_write16(cpu_t *cpu, uint32_t addr, uint32_t value) {
    if (!in_bounds(cpu, addr, 2)) {
        cpu->halted = true;
        return;
    }
    cpu->mem[addr] = (uint8_t)(value & 0xFF);
    cpu->mem[addr + 1] = (uint8_t)((value >> 8) & 0xFF);
}

void cpu_mem_write32(cpu_t *cpu, uint32_t addr, uint32_t value) {
    if (!in_bounds(cpu, addr, 4)) {
        cpu->halted = true;
        return;
    }
    cpu->mem[addr] = (uint8_t)(value & 0xFF);
    cpu->mem[addr + 1] = (uint8_t)((value >> 8) & 0xFF);
    cpu->mem[addr + 2] = (uint8_t)((value >> 16) & 0xFF);
    cpu->mem[addr + 3] = (uint8_t)((value >> 24) & 0xFF);
}
