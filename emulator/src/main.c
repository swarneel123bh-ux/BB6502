// #include "include/debug.h"
#include "include/debugger.h"

// Main program entry point
int main(int argc, char **argv) {

	dbgInit(argc, argv);
	dbgMainLoop();
	dbgCleanup();

	/*
	parseCmdLineArgs(argc, argv);
  initDbg();
  runDebugger();
  cleanUpDbg(); */

  return 0;
}
