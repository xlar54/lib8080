#include "memory.h"
#include "cpu.h"
#include <stdio.h>

void intercept_bdos_call(struct i8080 *cpu) {
  if (cpu->C == 2) { // BDOS function 2 (C_WRITE) - Console output
    if (cpu->E != 0) {
      putchar((char) cpu->E);
    }
  } else if (cpu->C == 9) { // BDOS function 9 (C_WRITESTR) - Output string
    for (int addr = CONCAT(cpu->D, cpu->E);read_byte(cpu, addr) != '$';addr++) {
      if (read_byte(cpu, addr) != 0) {
        putchar((char) read_byte(cpu, addr));
      }
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: binloader <filename>\n");
    return 1;
  }

  struct i8080 *cpu = create_cpu();
  create_memory(cpu, 65536); // 64 KiB
  reset_cpu(cpu);

  // CP/M Binaries are loaded with a 256 byte offset
  cpu->PC = 0x100;

  // Load the binary as a CP/M program (loaded at offset 0x100)
  load_memory(cpu, argv[1], 0x100);

  // Inject RET at 0x05 to allow for mocking of CP/M BDOS system calls
  write_byte(cpu, 5, 0xC9);

  while (1) {
    step_cpu(cpu);

    // CP/M warm boot (test finished and restarted itself)
    if (cpu->PC == 0) {
      return 0;
    }

    // CP/M BDOS call
    if (cpu->PC == 0x0005) {
      intercept_bdos_call(cpu);
    }
  }
}