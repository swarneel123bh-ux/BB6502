#pragma once

#include <ncurses.h>
#include <stdarg.h>

// Window data structure
// Wraps ncurses window
typedef struct window_t {
  WINDOW *win, *winbg;        // ncurses windows for foreground and background
  int winw, winh, winx, winy; // window dimensions and coordinates
  int wincx,
      wincy; // window cursor coordinates, may not be used depending on purpose
} window_t;

extern int maxlines, maxcols; // Some data from main window
extern window_t dbgwin;       // Window for the main debugger console
extern window_t termwin;      // Window for the 6502 terminal
extern window_t srcwin;       // Window for the assembly src currently being run

// Function to setup display
extern void DISPLAY_INITDISPLAY(void);

// Get a single character from console
// No need to press return key
extern char DISPLAY_CONSOLE_GETCHAR(void);

// Echo a buffer to the console
extern void DISPLAY_CONSOLE_ECHO(const char *fmt, ...);

// Get the next command from the console
extern void DISPLAY_CONSOLE_GETCMD(char *buf, size_t bufsiz);

// Put a character at cursor location at terminal
extern void DISPLAY_TERMINAL_PUTCHAR(char c);

// Get non-blocking input from terminal
extern char DISPLAY_TERMINAL_GETCHAR(void);
