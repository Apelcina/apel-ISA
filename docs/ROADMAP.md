# Roadmap

Each phase should be usable/testable before the next one leans on it.

- [x] **1. ISA spec** — word size, registers, addressing modes, instruction
      encoding (opcode/funct fields, formats). Written spec + encode/decode
      table, no code yet. See [isa-spec.md](isa-spec.md).
- [ ] **2. Functional emulator** — fetch-decode-execute loop, flat memory,
      register file, minimal memory-mapped console I/O. No caches/pipeline.
      Fetch-decode-execute/registers/memory/all instructions/dump tool are
      done; memory-mapped console I/O is not started yet.
- [ ] **3. Assembler** — mnemonics + labels + directives -> machine code,
      built off the same tables as the emulator. Disassembler comes nearly
      free from the same decode table. Mnemonics, labels (two-pass
      resolution), and file I/O (.asm in, flat binary out) are done and
      run real programs end to end. Not started: directives (.data/.text/
      etc.), the disassembler itself, and assembling our pseudo-
      instructions (MOV/NOT/CLEAR/NOP from isa-spec.md).
- [ ] **4. Toolchain hardening** — simple object format, linker for
      multi-file programs, symbol tables, minimal debugger (single-step,
      register/memory dump).
- [ ] **5. Minimal kernel** — boot sequence, trap/syscall instruction, small
      syscall table (write-char, read-char, exit), maybe a timer interrupt.
      Single address space, no paging/processes yet.
- [ ] **5b. Stretch: virtual memory** — software-managed TLB (MIPS-style:
      hardware/emulator holds a small translation cache, traps to kernel on
      a miss so the kernel walks whatever page table format it wants).
      Page-table-base register, page-fault trap, kernel-installed mappings.
      No multi-level tables, no swap-to-disk, no scheduler — just enough to
      demo two toy programs with overlapping virtual addresses landing on
      different physical frames. Explore after phase 5 is solid; not
      committed yet.
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

Completed so far:
- Phase 1: ISA spec locked (2026-08-06) — see [isa-spec.md](isa-spec.md).
- Phase 2 (mostly): machine state, decode, execute for every instruction
  in the opcode table, the fetch-decode-execute loop, and a register/
  memory dump tool (2026-08-07). Still missing: memory-mapped console I/O.
- Phase 3 (mostly): mnemonic table, single-line and two-pass multi-line
  assembly with label resolution, and file I/O - a real .asm file now
  assembles to a flat binary and runs on the emulator end to end
  (2026-08-07). Still missing: directives, the disassembler, and
  pseudo-instruction support.

Next up: pick up a remaining piece of Phase 2 or 3, or move on to
Phase 4 (toolchain hardening - object format, linker, debugger).
