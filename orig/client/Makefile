CFLAGS = -Wno-deprecated-declarations -Wno-unused-result -ffast-math -O2 -D__DEBUG__
LDLIBS = -lagl -laldmb -ldumb  -lGL -lGLU `allegro-config --libs`
CC=gcc
OUTPUT=TicTac2.elf

.PHONY: doc clean

all:
	$(CC) $(CFLAGS) src/*.c $(LDLIBS) -o $(OUTPUT)
	@echo "Done! :o)\n"

clean:
	@rm -f $(OUTPUT)

doc:
	doxygen
