CC=gcc
DBG=lldb

.PHONY: all dbg test clean run

all: bin dbg

bin:
	$(CC) src/main.c src/include/debug.c src/include/helpers.c src/include/fake6502.c src/include/disasm.c -lpthread -o build/debug6502

dbg:
	$(CC) src/main.c src/include/debug.c src/include/helpers.c src/include/fake6502.c src/include/disasm.c -lpthread -o build/debug6502dbg -g3

run: bin
	build/debug6502 tests/build/uart_test.out

test: dbg
	lldb build/debug6502dbg

clean:
	rm -rf build/*
