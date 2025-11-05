CC = gcc
CFLAGS = -Wall -Wextra -pedantic -D_GNU_SOURCE -std=gnu99 -Ofast
LDFLAGS = -lncurses -lm

SRC = conway-screensaver.c
OBJ = $(SRC:.c=.o)
EXEC = conway-screensaver

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(OBJ) -o $(EXEC) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: all
	install -Dm755 $(EXEC) /usr/bin/$(EXEC)
	install -Dm644 game_of_life.conf /usr/share/doc/conway-screensaver/game_of_life.conf

install-config:
	mkdir -p ~/.config/conway-screensaver
	cp /usr/share/doc/conway-screensaver/game_of_life.conf ~/.config/conway-screensaver/

uninstall:
	rm -f /usr/bin/$(EXEC)
	rm -f /usr/share/doc/conway-screensaver/game_of_life.conf

uninstall-config:
	rm -f ~/.config/conway-screensaver/game_of_life.conf

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean install install-config uninstall uninstall-config

