#include "loop.h"
#include "execute.h"
#include "isa.h"

uint32_t cpu_fetch(const cpu_t *cpu) {
    const uint8_t *p = &cpu->mem[cpu->pc];
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

bool cpu_load_program(cpu_t *cpu, const uint32_t *words, size_t count) {
    if (count * 4 > cpu->mem_size) {
        return false;
    }

    for (size_t i = 0; i < count; i++) {
        uint32_t addr = (uint32_t)(i * 4);
        uint32_t w = words[i];
        cpu->mem[addr + 0] = (uint8_t)(w & 0xFF);
        cpu->mem[addr + 1] = (uint8_t)((w >> 8) & 0xFF);
        cpu->mem[addr + 2] = (uint8_t)((w >> 16) & 0xFF);
        cpu->mem[addr + 3] = (uint8_t)((w >> 24) & 0xFF);
    }
    return true;
}

void cpu_step(cpu_t *cpu) {
    if (cpu->halted) {
        return;
    }
    uint32_t word = cpu_fetch(cpu);
    decoded_instr_t d = decode(word);
    execute(cpu, d);
}

uint32_t cpu_run(cpu_t *cpu, uint32_t max_steps) {
    uint32_t steps = 0;
    while (!cpu->halted && steps < max_steps) {
        cpu_step(cpu);
        steps++;
    }
    return steps;
}
