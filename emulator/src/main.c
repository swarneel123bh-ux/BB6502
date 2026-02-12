#include "include/debug.h"

// Main program entry point
int main(int argc, char **argv) {

  parseCmdLineArgs(argc, argv);
  initDbg();
  runDebugger();
  cleanUpDbg();

  return 0;
}
