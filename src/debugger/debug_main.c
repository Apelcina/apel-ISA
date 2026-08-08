#include "cpu.h"
#include "disassemble.h"
#include "dump.h"
#include "file_io.h"
#include "isa.h"
#include "loop.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

#define MAX_WORDS 65536

static void print_current_instruction(const cpu_t *cpu) {
    if (cpu->halted) {
        printf("[HALTED] pc=0x%08X\n", cpu->pc);
        return;
    }
    uint32_t word = cpu_fetch(cpu);
    decoded_instr_t d = decode(word);
    char buf[64];
    disassemble(cpu->pc, d, buf, sizeof(buf));
    printf("pc=0x%08X: %s\n", cpu->pc, buf);
}

static void print_help(void) {
    printf("commands:\n"
           "  step [n]        (s)  execute n instructions (default 1)\n"
           "  run             (r)  run until halted\n"
           "  where                show the current instruction again\n"
           "  regs                 dump all registers\n"
           "  mem <hexaddr> [len]  hex+ASCII dump of memory\n"
           "  help            (h)  show this list\n"
           "  quit            (q)  exit\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <program.bin>\n", argv[0]);
        return 1;
    }

    static uint32_t words[MAX_WORDS]; /* static: see asm_main.c for why */
    size_t count;
    if (!read_binary_file(argv[1], words, MAX_WORDS, &count)) {
        fprintf(stderr, "error: could not read '%s'\n", argv[1]);
        return 1;
    }

    cpu_t cpu;
    if (!cpu_init(&cpu)) {
        fprintf(stderr, "error: out of memory\n");
        return 1;
    }
    if (!cpu_load_program(&cpu, words, count)) {
        fprintf(stderr, "error: program too large for emulator memory\n");
        cpu_destroy(&cpu);
        return 1;
    }

    printf("apel-debug: loaded %zu words from '%s'\n", count, argv[1]);
    print_help();
    print_current_instruction(&cpu);

    char line[128];
    while (1) {
        printf("(apel-debug) ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) {
            break; /* EOF */
        }

        char *cmd = strtok(line, " \t\r\n");
        if (!cmd) {
            continue;
        }

        if (strcmp(cmd, "step") == 0 || strcmp(cmd, "s") == 0) {
            char *n_str = strtok(NULL, " \t\r\n");
            int n = n_str ? atoi(n_str) : 1;
            if (n < 1) {
                n = 1;
            }
            int i;
            for (i = 0; i < n && !cpu.halted; i++) {
                cpu_step(&cpu);
            }
            print_current_instruction(&cpu);
        } else if (strcmp(cmd, "run") == 0 || strcmp(cmd, "r") == 0) {
            uint32_t steps = cpu_run(&cpu, 1000000);
            printf("ran %u step%s\n", steps, steps == 1 ? "" : "s");
            print_current_instruction(&cpu);
        } else if (strcmp(cmd, "where") == 0) {
            print_current_instruction(&cpu);
        } else if (strcmp(cmd, "regs") == 0) {
            dump_registers(&cpu, stdout);
        } else if (strcmp(cmd, "mem") == 0) {
            char *addr_str = strtok(NULL, " \t\r\n");
            char *len_str = strtok(NULL, " \t\r\n");
            if (!addr_str) {
                printf("usage: mem <hexaddr> [len]\n");
                continue;
            }
            uint32_t addr = (uint32_t)strtoul(addr_str, NULL, 16);
            uint32_t len = len_str ? (uint32_t)strtoul(len_str, NULL, 10) : 64;
            dump_memory(&cpu, stdout, addr, len);
        } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
            print_help();
        } else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
            break;
        } else {
            printf("unknown command '%s' (try 'help')\n", cmd);
        }
    }

    cpu_destroy(&cpu);
    return 0;
}
