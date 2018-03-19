cmake_minimum_required(VERSION 3.9)
project(MiniShell C)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES
        src/main.c
        src/Command/cmd.c
        src/Command/cmd.h
        src/Parser/parser.h
        src/Parser/parser.tab.c
        src/Parser/parser.tab.h
        src/Parser/utils.h
        src/Parser/utils-lin.c
        src/Parser/parser.yy.c
        src/Parser/parser.l)

add_executable(MiniShell ${SOURCE_FILES})