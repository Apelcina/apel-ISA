#ifndef APEL_EMULATOR_EXECUTE_H
#define APEL_EMULATOR_EXECUTE_H

#include "cpu.h"
#include "isa.h"

void execute(cpu_t *cpu, decoded_instr_t d);

#endif
