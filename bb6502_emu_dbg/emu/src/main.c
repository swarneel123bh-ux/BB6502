// #include "include/debug.h"
// #include "include/debugger.h"
#include "include/fake6502.h"

// Main program entry point
int main(int argc, char **argv) {

	/*dbgInit(argc, argv);
	dbgMainLoop();
	dbgCleanup();*/

	/*
	parseCmdLineArgs(argc, argv);
  initDbg();
  runDebugger();
  cleanUpDbg(); */

  fake6502Init(argc, argv);

  return 0;
}
