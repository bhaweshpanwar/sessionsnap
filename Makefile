CC = gcc
CFLAGS = -Wall -Wextra -g \
	$(shell pkg-config --cflags gtk+-3.0) \
	$(shell pkg-config --cflags x11) \
	-Iinclude

LIBS = \
	$(shell pkg-config --libs gtk+-3.0) \
	$(shell pkg-config --libs x11)

SRC = \
	src/main.c \
	src/capture.c \
	src/session.c \
	src/restore.c \
	src/monitor.c \
	src/gui.c \
	vendor/cJSON.c

OUT = sessionsnap

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)

install: $(OUT)
	cp $(OUT) /usr/local/bin/$(OUT)

uninstall:
	rm -f /usr/local/bin/$(OUT)

test-list: $(OUT)
	./$(OUT) --list

test-snapshot: $(OUT)
	./$(OUT) --snapshot

test-restore: $(OUT)
	./$(OUT) --restore

.PHONY: all clean install uninstall test-list test-snapshot test-restore