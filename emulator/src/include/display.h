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
void DISPLAY_INITDISPLAY();

// Get a single character from console
// No need to press return key
char DISPLAY_CONSOLE_GETCHAR();

// Echo a buffer to the console
void DISPLAY_CONSOLE_ECHO(const char *fmt, ...);

// Get the next command from the console
void DISPLAY_CONSOLE_GETCMD(char *buf, size_t bufsiz);

// Put a character at cursor location at terminal
void DISPLAY_TERMINAL_PUTCHAR(char c);

// Get non-blocking input from terminal
char DISPLAY_TERMINAL_GETCHAR();
