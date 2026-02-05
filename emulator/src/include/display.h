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
    dbgwin.winw = maxcols - 2;                                                 \
    dbgwin.winh = maxlines - (maxlines / 2) - 3;                               \
    dbgwin.winx = 1;                                                           \
    dbgwin.winy = (maxlines / 2) + 2;                                          \
    dbgwin.winbg = newwin(dbgwin.winh + 2, dbgwin.winw + 2, dbgwin.winy - 1,   \
                          dbgwin.winx - 1);                                    \
    dbgwin.win = newwin(dbgwin.winh, dbgwin.winw, dbgwin.winy, dbgwin.winx);   \
    box(dbgwin.winbg, 0, 0);                                                   \
    scrollok(dbgwin.win, TRUE);                                                \
    keypad(dbgwin.win, TRUE);                                                  \
    wrefresh(dbgwin.winbg);                                                    \
    wrefresh(dbgwin.win);                                                      \
    termwin.winw = maxcols - (maxcols / 2) - 2;                                \
    termwin.winh = maxlines - (maxlines / 2) - 1;                              \
    termwin.winx = 1;                                                          \
    termwin.winy = 1;                                                          \
    termwin.winbg = newwin(termwin.winh + 2, termwin.winw + 2,                 \
                           termwin.winy - 1, termwin.winx - 1);                \
    termwin.win =                                                              \
        newwin(termwin.winh, termwin.winw, termwin.winy, termwin.winx);        \
    box(termwin.winbg, 0, 0);                                                  \
    scrollok(termwin.win, TRUE);                                               \
    nodelay(termwin.win, TRUE);                                                \
    keypad(termwin.win, TRUE);                                                 \
    wrefresh(termwin.winbg);                                                   \
    wrefresh(termwin.win);                                                     \
    srcwin.winw = maxcols - (maxcols / 2) - 2;                                 \
    srcwin.winh = maxlines - (maxlines / 2) - 1;                               \
    srcwin.winx = termwin.winx + termwin.winw + 2;                             \
    srcwin.winy = 1;                                                           \
    srcwin.winbg = newwin(srcwin.winh + 2, srcwin.winw + 2, srcwin.winy - 1,   \
                          srcwin.winx - 1);                                    \
    srcwin.win = newwin(srcwin.winh, srcwin.winw, srcwin.winy, srcwin.winx);   \
    box(srcwin.winbg, 0, 0);                                                   \
    scrollok(srcwin.win, TRUE);                                                \
    keypad(srcwin.win, TRUE);                                                  \
    wrefresh(srcwin.winbg);                                                    \
    wrefresh(srcwin.win);                                                      \
  } while (0);

// Get a single character from console
// No need to press return key
#define DISPLAY_CONSOLE_GETCHAR() wgetch(dbgwin.win);

// Get the next command from the console
#define DISPLAY_CONSOLE_GETCMD(buf)                                            \
  do {                                                                         \
    DISPLAY_CONSOLE_ECHO(">>");                                                \
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
window_t srcwin;       // Window for the assembly src currently being run

int dispmain();
