CFLAGS = -Wno-deprecated -Wno-unused-result -ffast-math -g -O2 -DDEBUG
LDLIBS =
CC=gcc
OUTPUT=TicTac2Server.elf

.PHONY: doc clean

all:
	$(CC) $(CFLAGS) src/*.c $(LDLIBS) -o $(OUTPUT)
	@echo "Done! :o)\n"

clean:
	@rm -f $(OUTPUT)

doc:
	doxygen
