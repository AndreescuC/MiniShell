CC = gcc
CFLAGS = -Wall

CMD_SOURCES = src/Command/cmd.c
PARSER_SOURCES = src/Parser/parser.tab.c src/Parser/parser.yy.c src/Parser/utils-lin.c
MAIN = src/main.c
CMD_HEADERS = src/Command/cmd.h
PARSER_HEADERS = src/Parser/parser.h src/Parser/parser.tab.h src/Parser/utils.h
FLEX = src/Parser/parser.y src/Parser/parser.l

all: build

build: $(CMD_SOURCES) $(PARSER_SOURCES) $(MAIN) $(CMD_HEADERS) $(PARSER_HEADERS) $(FLEX)
	$(CC) $(CFLAGS) -o mini-shell $(MAIN) $(CMD_SOURCES) $(PARSER_SOURCES)

clean: mini-shell
	rm -rf mini-shell
