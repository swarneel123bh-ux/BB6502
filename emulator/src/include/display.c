#include "display.h"
#include <ncurses.h>
#include <stdarg.h>

int maxlines, maxcols; // Some data from main window
window_t dbgwin;       // Window for the main debugger console
window_t termwin;      // Window for the 6502 terminal
window_t srcwin;       // Window for the assembly src currently being run

// Macro to setup display
void DISPLAY_INITDISPLAY(void) {
  initscr();
  keypad(stdscr, TRUE);
  cbreak();
  nl();
  refresh();
  getmaxyx(stdscr, maxlines, maxcols);
  dbgwin.winw = maxcols - 2;
  dbgwin.winh = maxlines - (maxlines / 2) - 3;
  dbgwin.winx = 1;
  dbgwin.winy = (maxlines / 2) + 2;
  dbgwin.winbg = newwin(dbgwin.winh + 2, dbgwin.winw + 2, dbgwin.winy - 1,
                        dbgwin.winx - 1);
  dbgwin.win = newwin(dbgwin.winh, dbgwin.winw, dbgwin.winy, dbgwin.winx);
  box(dbgwin.winbg, 0, 0);
  scrollok(dbgwin.win, TRUE);
  keypad(dbgwin.win, TRUE);
  wrefresh(dbgwin.winbg);
  wrefresh(dbgwin.win);
  termwin.winw = maxcols - (maxcols / 2) - 2;
  termwin.winh = maxlines - (maxlines / 2) - 2;
  termwin.winx = 1;
  termwin.winy = 1;
  termwin.winbg = newwin(termwin.winh + 2, termwin.winw + 2, termwin.winy - 1,
                         termwin.winx - 1);
  termwin.win = newwin(termwin.winh, termwin.winw, termwin.winy, termwin.winx);
  box(termwin.winbg, 0, 0);
  scrollok(termwin.win, TRUE);
  nodelay(termwin.win, TRUE);
  keypad(termwin.win, TRUE);
  wrefresh(termwin.winbg);
  wrefresh(termwin.win);
  srcwin.winw = maxcols - (maxcols / 2) - 2;
  srcwin.winh = maxlines - (maxlines / 2) - 2;
  srcwin.winx = termwin.winx + termwin.winw + 2;
  srcwin.winy = 1;
  srcwin.winbg = newwin(srcwin.winh + 2, srcwin.winw + 2, srcwin.winy - 1,
                        srcwin.winx - 1);
  srcwin.win = newwin(srcwin.winh, srcwin.winw, srcwin.winy, srcwin.winx);
  box(srcwin.winbg, 0, 0);
  scrollok(srcwin.win, TRUE);
  keypad(srcwin.win, TRUE);
  wrefresh(srcwin.winbg);
  wrefresh(srcwin.win);
}

// Get a single character from console
// No need to press return key
char DISPLAY_CONSOLE_GETCHAR(void) {
  return wgetch(dbgwin.win);
}

// Echo a buffer to the console
void DISPLAY_CONSOLE_ECHO(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  mvwprintw(dbgwin.win, getcury(dbgwin.win), getcurx(dbgwin.win), fmt, args);
  va_end(args);
  wrefresh(dbgwin.win);
}

// Get the next command from the console
void DISPLAY_CONSOLE_GETCMD(char* buf, size_t bufsiz) {
  DISPLAY_CONSOLE_ECHO(">>");
  wgetnstr(dbgwin.win, buf, bufsiz);
}

// Put a character at cursor location at terminal
void DISPLAY_TERMINAL_PUTCHAR(char c) {
  waddch(termwin.win, (c == '\r') ? '\n' : c);
  wrefresh(termwin.win);
}

// Get non-blocking input from terminal
char DISPLAY_TERMINAL_GETCHAR(void) {
  return wgetch(termwin.win);
}
