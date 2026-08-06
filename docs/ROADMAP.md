# Roadmap

Each phase should be usable/testable before the next one leans on it.

- [ ] **1. ISA spec** — word size, registers, addressing modes, instruction
      encoding (opcode/funct fields, formats). Written spec + encode/decode
      table, no code yet. See [isa-spec.md](isa-spec.md).
- [ ] **2. Functional emulator** — fetch-decode-execute loop, flat memory,
      register file, minimal memory-mapped console I/O. No caches/pipeline.
- [ ] **3. Assembler** — mnemonics + labels + directives -> machine code,
      built off the same tables as the emulator. Disassembler comes nearly
      free from the same decode table.
- [ ] **4. Toolchain hardening** — simple object format, linker for
      multi-file programs, symbol tables, minimal debugger (single-step,
      register/memory dump).
- [ ] **5. Minimal kernel** — boot sequence, trap/syscall instruction, small
      syscall table (write-char, read-char, exit), maybe a timer interrupt.
      Single address space, no paging/processes yet.
- [ ] **6. Shell** — REPL userland program built on the phase 5 syscalls.
- [ ] **7. Filesystem** — flat file table over a fake block device, plus
      read/write syscalls.
- [ ] **8. Custom language + compiler** — small C-like language (expressions,
      control flow, functions, maybe structs) compiling to our assembly.
      Real test: compile the shell or the snake game.
- [ ] **9. Port real programs** — snake game as the capstone (input polling +
      screen output, exercises syscalls/drivers end to end).
- [ ] **10. Stretch: timing-accurate simulator** — caches, pipeline stages,
      hazards, as a second simulator so the fast functional one stays fast
      for kernel/compiler dev.

## Status

Project scaffolded 2026-08-06. Next up: Phase 1, ISA spec.
