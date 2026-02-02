#pragma once

#include <ncurses.h>
#include <stdarg.h>

// Macro to setup display
#define DISPLAY_INITDISPLAY()                                                  \
  do {                                                                         \
    initscr();                                                                 \
    keypad(stdscr, TRUE);                                                      \
    cbreak();                                                                  \
    nl();                                                                      \
    refresh();                                                                 \
    getmaxyx(stdscr, maxlines, maxcols);                                       \
    dbgwin.winw = maxcols - 1;                                                 \
    dbgwin.winh = maxlines - 1;                                                \
    dbgwin.winx = 0;                                                           \
    dbgwin.winy = 0;                                                           \
    dbgwin.winbg =                                                             \
        newwin(maxlines - (maxlines / 2) - 1, maxcols, (maxlines / 2) + 1, 0); \
    dbgwin.win = newwin(maxlines - (maxlines / 2) - 3, maxcols - 2,            \
                        (maxlines / 2) + 2, 1);                                \
    box(dbgwin.winbg, 0, 0);                                                   \
    scrollok(dbgwin.win, TRUE);                                                \
    keypad(dbgwin.win, TRUE);                                                  \
    wrefresh(dbgwin.winbg);                                                    \
    wrefresh(dbgwin.win);                                                      \
    termwin.winw = maxcols - 1;                                                \
    termwin.winh = maxlines - 1;                                               \
    termwin.winx = 0;                                                          \
    termwin.winy = 0;                                                          \
    termwin.winbg = newwin(maxlines - (maxlines / 2), maxcols, 0, 0);          \
    termwin.win = newwin(maxlines - (maxlines / 2) - 4, maxcols - 2, 1, 1);    \
    box(termwin.winbg, 0, 0);                                                  \
    scrollok(termwin.win, TRUE);                                               \
    nodelay(termwin.win, TRUE);                                                \
    keypad(termwin.win, TRUE);                                                 \
    wrefresh(termwin.winbg);                                                   \
    wrefresh(termwin.win);                                                     \
  } while (0);

// Get a single character from console
#define DISPLAY_CONSOLE_GETCHAR() wgetch(dbgwin.win);

// Get the next command from the console
#define DISPLAY_CONSOLE_GETCMD(buf)                                            \
  do {                                                                         \
    DISPLAY_CONSOLE_ECHO("  [bcdhlmrsq]>>");                                   \
    wgetnstr(dbgwin.win, buf, sizeof(buf) - 1);                                \
  } while (0);

// Echo a buffer to the console
#define DISPLAY_CONSOLE_ECHO(fmtbuf, ...)                                      \
  do {                                                                         \
    mvwprintw(dbgwin.win, getcury(dbgwin.win), getcurx(dbgwin.win),            \
              fmtbuf __VA_OPT__(, ) __VA_ARGS__);                              \
    wrefresh(dbgwin.win);                                                      \
  } while (0);

// Put a character at cursor location at terminal
#define DISPLAY_TERMINAL_PUTCHAR(c)                                            \
  do {                                                                         \
    waddch(termwin.win, c);                                                    \
    wrefresh(dbgwin.win);                                                      \
  } while (0);

// Get non-blocking input from terminal
#define DISPLAY_TERMINAL_GETCHAR() wgetch(termwin.win);

// Window data structure
// Wraps ncurses window
typedef struct window_t {
  WINDOW *win, *winbg;        // ncurses windows for foreground and background
  int winw, winh, winx, winy; // window dimensions and coordinates
  int wincx,
      wincy; // window cursor coordinates, may not be used depending on purpose
} window_t;

int maxlines, maxcols; // Some data from main window
window_t dbgwin;       // Window for the main debugger console
window_t termwin;      // Window for the 6502 terminal

int dispmain();
