EXECUTABLE = ./raven.exe

SOURCES = $(shell find . -name "*.c")
HEADERS = $(shell find . -name "*.h")

OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
DEPENDS = $(patsubst %.c, %.d, $(SOURCES))

CFLAGS = -g -Wall
LDFLAGS =

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

all: $(EXECUTABLE)

.phony: run
run: all
	$(EXECUTABLE) ${RAVEN_MUDLIB}

.phony: debug
debug: all
		gdb $(EXECUTABLE)

.phony: leaks
leaks: all
		valgrind --leak-check=full $(EXECUTABLE)

.phony: clean
clean:
	- $(RM) $(EXECUTABLE) $(OBJECTS) $(DEPENDS)

.phony: re
re: clean all

-include $(DEPENDS)
