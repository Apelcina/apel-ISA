#ifndef APEL_EMULATOR_CPU_H
#define APEL_EMULATOR_CPU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define APEL_NUM_REGS 16
#define APEL_MEM_SIZE (1u << 20) /* 1 MiB */

typedef struct {
    uint32_t regs[APEL_NUM_REGS];
    uint32_t pc;
    uint8_t *mem;
    size_t mem_size;
    bool halted;
} cpu_t;

/* Allocates cpu->mem (APEL_MEM_SIZE bytes) and resets all state.
   Returns false if the allocation failed. */
bool cpu_init(cpu_t *cpu);
void cpu_destroy(cpu_t *cpu);

/* Zeroes registers/pc/memory/halted without touching the allocation. */
void cpu_reset(cpu_t *cpu);

uint32_t cpu_reg_read(const cpu_t *cpu, unsigned index);
void cpu_reg_write(cpu_t *cpu, unsigned index, uint32_t value);

/* Bounds-checked memory access, little-endian. On out-of-bounds access,
   sets cpu->halted and (for reads) returns 0 - same placeholder
   convention as illegal instructions, pending real trap handling. Reads
   are zero-extended into the return value; sign extension, if wanted,
   is the caller's job. Writes truncate to the access width. */
uint32_t cpu_mem_read8(cpu_t *cpu, uint32_t addr);
uint32_t cpu_mem_read16(cpu_t *cpu, uint32_t addr);
uint32_t cpu_mem_read32(cpu_t *cpu, uint32_t addr);
void cpu_mem_write8(cpu_t *cpu, uint32_t addr, uint32_t value);
void cpu_mem_write16(cpu_t *cpu, uint32_t addr, uint32_t value);
void cpu_mem_write32(cpu_t *cpu, uint32_t addr, uint32_t value);

#endif
