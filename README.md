# apel-ISA

A from-scratch RISC-style instruction set architecture, built end-to-end as a
learning project: ISA design → assembler → emulator → kernel → shell →
filesystem → a small custom language and compiler → real programs (snake game)
running on top of it all.

## Decisions locked in

- **Toolchain language:** C
- **Word size:** 32-bit
- **Instruction encoding:** fixed-width (all instructions one word)

These live here because they're load-bearing for everything else; see
[docs/isa-spec.md](docs/isa-spec.md) for the actual encoding design.

## Layout

```
docs/            ISA spec, design notes, roadmap
src/isa/         shared encode/decode tables (used by everything below)
src/emulator/    functional emulator (fetch-decode-execute over flat memory)
src/assembler/   mnemonics + labels + directives -> machine code (+ disassembler)
src/linker/      simple object format / linker
src/debugger/    single-step, register/memory dump
src/kernel/      boot sequence, syscalls, trap handling
src/shell/       userland REPL running under the kernel
src/fs/          minimal filesystem + read/write syscalls
src/lang/        small C-like language + compiler targeting our assembly
programs/        real programs written for the ISA (snake game, etc.)
tests/           test programs / test harness
```

## Roadmap

See [docs/ROADMAP.md](docs/ROADMAP.md) for the phased plan and current status.
