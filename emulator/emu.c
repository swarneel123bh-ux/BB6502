#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include "fake6502.c"


// Addresses to read from
// The 6502 program MUST write it's own addresses to these ones
// during the setup process
// So that the emulator can read from the correct one
// Regardless of what the assembly writes to
#define EMU_UARTINADDR            0xFFEA 
#define EMU_UARTOUTADDR           0xFFEC
#define EMU_STDOUTBUFADDR         0xFFEE
#define EMU_STDOUTBUFSIZADDR      0xFFE0
#define EMU_STDINBUFADDR          0xFFF2
#define EMU_STDINBUFSIZADDR       0xFFF4

// FlagsEMU_ that the emulator will read for proper function
#define EMU_STDOUT_BUFFLUSH_FLAG  0xFFF6
#define EMU_STDIN_GETDATA_FLAG    0xFFF7
#define EMU_UART_OUT_FLAG         0xFFF8
#define EMU_UART_IN_FLAG          0xFFF9



// Terminal stuff for async input
static struct termios oldt; 
int getKeyAsync();
void restoreTerminal(); 
void setupTerminal(void);
void restoreTerminal(void); 



// 6502 Hardware stuff
uint8_t* RAM6502; // Full address space of the 6502

// The addresses the emulator needs to keep track of
// All of these are loaded from the metadata
uint16_t emuUartIn;
uint16_t emuUartOut;
uint16_t emuStdOutBuf;  // This is only necessary for buffer flush
uint16_t emuStdInBuf;   // This is only necessary for buffered Input




// Externally provided functions for fake6502
uint8_t read6502(uint16_t address) { return RAM6502[address]; } // Reads byte at <address> and returns the value read
void write6502(uint16_t address, uint8_t value) { RAM6502[address] = value; return; } // Writes byte <value> at <address>



// Helper functions for the emulator
uint16_t read_hex_u16(void);
void flushTerminal();
void bufferedInput();
void showmem(uint16_t startaddr, uint16_t endaddr);
void runInteractive();
void runNonInteractive();


// Main function
int main(int argc, char** argv) {

  if (argc < 2) {
    printf("Usage: %s <rom_image_name>\n", argv[0]);
    return 1;
  }

  bool interactive_mode = false;


  FILE* f = fopen(argv[1], "rb");
  if (!f) { perror("fopen: "); return 1; }


  if (argc == 3) { 
    if (!strcmp("-i", argv[2])) { 
      printf("Starting in interactive mode...\n");
      interactive_mode = true; 
    } else { printf("Starting in non-interactive mode...\n"); }
  } 


  // Disable stdout buffering
  setvbuf(stdout, NULL, _IONBF, 0);


  // Read the binary file and put into RAM
  // This is emulator only because later we need to write the programs to ROM
  // Hence we need to also make sure that the programs are addressed properly
  uint8_t tempbyte = 0x00;
  uint16_t tempaddr = 0x0000;
  int readbytes = 0;
  RAM6502 = (uint8_t*) calloc(0x10000, sizeof(uint8_t));
  if (!RAM6502) { perror("calloc: "); return 2; }
  while (fread(&tempbyte, sizeof(uint8_t), 1, f)) { write6502(tempaddr, tempbyte); tempaddr ++; readbytes ++; }
  fclose(f);
  printf("Read %d bytes\n", readbytes);


  // Read the important addresses from the metadata table
  emuUartIn =       read6502(EMU_UARTINADDR)      | read6502(EMU_UARTINADDR + 1) << 8;
  emuUartOut =      read6502(EMU_UARTOUTADDR)     | read6502(EMU_UARTOUTADDR + 1) << 8;
  emuStdOutBuf =    read6502(EMU_STDOUTBUFADDR)   | read6502(EMU_STDOUTBUFADDR + 1) << 8;
  emuStdInBuf =     read6502(EMU_STDINBUFADDR)    | read6502(EMU_STDINBUFADDR + 1) << 8;

  printf("emuUartIn     = %04hx\n", emuUartIn);
  printf("emuUartOut    = %04hx\n", emuUartOut);
  printf("emuStdOutBuf  = %04hx\n", emuStdOutBuf);
  printf("emuStdInBuf   = %04hx\n", emuStdInBuf);


  // Check running mode of the emulator and call the required function
  if (!interactive_mode) { runNonInteractive(); }
  else { runInteractive(); }

  
  // Cleanup
  free(RAM6502);
  printf("Exiting ...\n");
  return 0;
}


// Helper function for Interactive Mode
// Gets the input and does the required job
// It Takes a reference to the running var 
// As the exiting is handle by this function
void getNextCmd(bool* running) {
    char c = getchar();
    switch(c) {
      case 'q': 
      *running = false; 
      break;

      case 'r': 
      printf("PC=%d SP=%d A=%d X=%d Y=%d status=%d\n", pc, sp, a, x, y, status);
      break;

      case 's':
      printf("PC=%d\n", pc);
      step6502();
      break;

      case 'e':
      *running = false;
      break;

      case 'm': {
        char buf[32];
        uint16_t start, end;

        printf("Start (hex): ");
        scanf(" %hx", &start);

        printf("End (hex): ");
        scanf(" %hx", &end);

        printf("Showing mem contents from %hx to %hx\n", start, end);
        getchar();

        if (end < start) {
          printf("Invalid range\n");
          break;
        }

        showmem(start, end);
        break;
      }

      case 'c':
      // Pressing this once will put the emulator into continue state and we cannot step it anymore
      while (*running) {
        step6502();

        // Flush stdout signal
        if (read6502(EMU_STDOUT_BUFFLUSH_FLAG)) { 
          flushTerminal(); 
          write6502(EMU_STDOUT_BUFFLUSH_FLAG, 0x00); 
        }

      }
      break;

      default:
      break;
    }

    return;
}


// Run the emulator in interactive mode...for debuggin
// We cannot access the keyboard in normal mode while debugging
// Because i cannot figure out how to implement that while also being in stepping mode
// So for now keyboard needs Non-Interactive mode
// Good luck with debugging any issues with that lol
void runInteractive() {
  bool running = true;

  uint8_t terminalFlushFlag = 0;

  reset6502();

  while (running) { 
    // Read the memory for requests and execute
    if (read6502(EMU_STDOUT_BUFFLUSH_FLAG)) { 
      flushTerminal(); 
      write6502(EMU_STDOUT_BUFFLUSH_FLAG, 0x00); 
    }

    if (read6502(EMU_STDIN_GETDATA_FLAG)) { 
      bufferedInput(); 
      write6502(EMU_STDIN_GETDATA_FLAG, 0x00);
    }

    getNextCmd(&running);
  }

  return;
}


// Writes to RAM6502 at addr of the UART reg
// Make sure that byt is a uint8_t type
// Set to 1 so the processor knows it needs to read
// Later we will trigger an interrupt from this macro 
#define putByteInUART(byt) {\
  write6502(emuUartIn, byt);\
  write6502(EMU_UART_IN_FLAG, 0x01);\
  irq6502();\
}


// reads a byte from UART reg
#define readByteFromUART() {\
  putchar(read6502(emuUartOut));\
  write6502(EMU_UART_OUT_FLAG, 0x00);\
}



// Run the emulator in non-interactive mode
// Closer to real functioning on the 6502
// Keyboard enabled here
void runNonInteractive() {
  bool running = true;

  reset6502();


  setupTerminal();    // Set terminal to async mode

  while (running) { 
    step6502();

    // Stdout print request
    if (read6502(EMU_STDOUT_BUFFLUSH_FLAG)) {   
      flushTerminal(); 
      write6502(EMU_STDOUT_BUFFLUSH_FLAG, 0x00); 
    }

    // Read from UART if required 
    if (read6502(EMU_UART_OUT_FLAG)) { readByteFromUART(); }

    int key = getKeyAsync();
    if (key == -1) { continue; } 

    // Key pressed, set to uart and trigger interrupt
    putByteInUART((uint8_t)key);
  }

}


// Flush the 6502 stdout to current terminal ... emulator specific only
void flushTerminal() {
  uint16_t readAddr =       (uint16_t)read6502(EMU_STDOUTBUFADDR)     | ((uint16_t)read6502(EMU_STDOUTBUFADDR + 1) << 8);
  uint16_t buffersizeaddr = (uint16_t)read6502(EMU_STDOUTBUFSIZADDR)  | ((uint16_t)read6502(EMU_STDOUTBUFSIZADDR + 1) << 8);
  uint16_t buffersize =     read6502(buffersizeaddr);

  for (uint16_t i = 0; i < buffersize; i++) {
    uint8_t c = read6502(readAddr+ i);
    if (c == 0) continue;

    printf("%c", c);
    write6502(readAddr + i, 0);
  }
}



// Read hex value address
uint16_t read_hex_u16(void) {
  char buf[32];
  char *end;
  unsigned long value;

  if (!fgets(buf, sizeof(buf), stdin)) {
    fprintf(stderr, "Input error\n");
    exit(EXIT_FAILURE);
  }

  errno = 0;
  value = strtoul(buf, &end, 0);  
  // base 0 => handles 0x..., 0X..., or plain hex/decimal

  if (errno != 0 || end == buf) {
    fprintf(stderr, "Invalid hex number\n");
    exit(EXIT_FAILURE);
  }

  if (value > 0xFFFF) {
    fprintf(stderr, "Value out of 16-bit range\n");
    exit(EXIT_FAILURE);
  }

  return (uint16_t)value;
}




// Display 6502 memory contents
void showmem(uint16_t startaddr, uint16_t endaddr) {
  uint16_t addr = startaddr;

  while (1) {
    printf("%04X: ", addr);

    for (uint16_t offset = 0; offset < 16; offset++) {
      uint16_t cur = addr + offset;
      if (cur < addr || cur > endaddr) break;
      printf("%02X ", read6502(cur));
    } printf("\n");

    /* Stop if next step would overflow or exceed endaddr */
    if (addr > endaddr - 16) break;

    addr += 16;
  }
}




// Get buffered input from the terminal and send to 6502 RAM
// Emulator specific
void bufferedInput() {
  printf("Buffered inputting ... : ");
  char buf[256];
  scanf(" %[^\n]", buf);

  // Get the address of the stdin buffer and write to it
  // Also update the size var to the current size
  uint16_t stdinbufaddr =     read6502(EMU_STDINBUFADDR)    | (read6502(EMU_STDINBUFADDR + 1) << 8);
  uint16_t stdinbufsizaddr =  read6502(EMU_STDINBUFSIZADDR) | (read6502(EMU_STDINBUFSIZADDR + 1) << 8);
  uint8_t stdinbufsiz = strlen(buf); 

  write6502(stdinbufsizaddr, stdinbufsiz);

  for (int i = 0; i <= stdinbufsiz; i ++) {
    write6502(stdinbufaddr, buf[i]);
    stdinbufaddr ++;
  }

  // Set the stdin readptr to the beginning of the stdin buf
  // Because we wrote from the beginning
  // This is because the input happened as a buffered input from the 
  // emulator and not from the 6502 assembly prog
  write6502(EMU_STDINBUFADDR, (uint8_t)(emuStdInBuf & 0x00FF));    // Low byte
  write6502(EMU_STDINBUFADDR + 1, (uint8_t)((emuStdInBuf & 0xFF00) >> 8));    // High byte
  
  printf("Buffered inputting done ... :\n");
  return;
}






// Get asynchronous key input
int getKeyAsync() {
  unsigned char c;
  if (read(STDIN_FILENO, &c, 1) == 1) {
    return c;
  }
  return -1;
}



// Set up terminal for async function
void setupTerminal(void) {
  struct termios newt;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;

  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  atexit(restoreTerminal);

  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

  return;
}




// Restore the terminal to old mode before exiting program
void restoreTerminal() { tcsetattr(STDIN_FILENO, TCSANOW, &oldt); }








