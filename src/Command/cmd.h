/**
 * Operating Sytems 2013-2017 - Assignment 2
 */

#ifndef _CMD_H
#define _CMD_H

#include "../Parser/parser.h"

#define SHELL_EXIT -100

/**
 * Parse and execute a command.
 */
int parse_command(command_t *cmd, int level, command_t *father);
static int do_redirects(simple_command_t *s, int *saved_stdout, int *saved_stdin, int *saved_stderr);

#endif /* _CMD_H */
