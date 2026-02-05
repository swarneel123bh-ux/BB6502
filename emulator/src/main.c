#include "include/debug.h"
#include "include/helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Main program entry point
int main(int argc, char **argv) {

  parseCmdLineArgs(argc, argv);
  initDbg();
  runDebugger();
  cleanUpDbg();

  return 0;
}
