CC=gcc

all: test
	$(CC) src/main.c src/include/debug.c src/include/helpers.c src/include/fake6502.c -lpthread -o build/debug6502

test:
	$(CC) src/main.c src/include/debug.c src/include/helpers.c src/include/fake6502.c -lpthread -o build/debug6502dbg
	lldb build/debug6502dbg

clean:
	rm -rf build/*

