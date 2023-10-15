CFLAGS=-pthread -D_POSIX_C_SOURCE=200809L -g -O0
ifneq ($(OS),Windows_NT)
	ifneq ($(DISABLE_SANS),1)
		CFLAGS +=  -fsanitize=address -fsanitize=undefined
	endif
endif
LFLAGS=
OUTPUT=program
LIBS=-lm

DEPFLAGS=-MT $@ -MMD -MP -MF build/$*.Td

SOURCES=$(wildcard *.c)
OBJS:=$(patsubst %.c,build/%.o,$(SOURCES))

.PHONY: all submission clean

$(shell mkdir -p $(DEPDIR) build >/dev/null)

all: build/program

vgbuild: clean
	$(MAKE) DISABLE_SANS=1 all
	echo -e "#!/bin/bash\nvalgrind --leak-check=full ./program \"\$$@\"" > run-valgrind.sh
	chmod +x run-valgrind.sh

%.o: %.c
build/%.o : %.c build/%.d
	$(CC) -Wall -Werror -std=c11 $(CFLAGS) $(DEPFLAGS) -o $@ -c $<
	mv -f build/$*.Td build/$*.d && touch $@

build/program: $(OBJS)
	$(CC) -Wall -Werror -std=c11 $(CFLAGS) $(LFLAGS) -o $@ $^ $(LIBS)
	cp build/program $(OUTPUT)

clean:
	rm -fr build $(OUTPUT)

build/%.d: ;
.PRECIOUS: build/%.d 

include $(wildcard $(patsubst %,build/%.d,$(basename $(SOURCES))))
