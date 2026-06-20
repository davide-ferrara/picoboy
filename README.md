# picoboy

Game Boy emulator from scratch, targeting Raspberry Pi Pico 2 (RP2350).

## Build

```
make test    # run tests
make emu     # build emulator (host)
make build   # build for Pico (CMake)
make flash   # flash to Pico
```

## Resources

- [Pan Docs — CPU Instruction Set](https://gbdev.io/pandocs/CPU_Instruction_Set.html)
- [gbz80(7) — instruction reference](https://rgbds.gbdev.io/docs/gbz80.7)
- [Optables — opcode cheat sheet](https://gbdev.io/gb-opcodes/optables)
- [Pan Docs (home)](https://gbdev.io/pandocs/)
