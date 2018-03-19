/**
 * Operating Systems 2013-2017 - Assignment 2
 *
 * Andreescu Constantin, 333CC
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "cmd.h"
#include "utils.h"
#include "parser.h"

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 */
static bool shell_cd(word_t *dir)
{
	/* TODO execute cd */

	return 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
	/* TODO execute exit/quit */

	return 0; /* TODO replace with actual exit code */
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	int status = -99, j, argc, reallocation_increment;
	pid_t child_pid = fork();

	switch (child_pid) {
		case -1:
			DIE(child_pid, "fork");
		case 0:
			argc = 5;
			reallocation_increment = 5;
			char **argvec;
			argvec = (char**)malloc(argc * sizeof(char*));
			for (j=0; j<argc; j++) {
				argvec[j] = (char*)malloc(20 * sizeof(char));
			}
			strcpy(argvec[0], s->verb->string);

			int i = 1;
			word_t *puppet;
			puppet = s->params;
			while (puppet != NULL) {
				if (i >= argc) {
					argvec = (char**)realloc(argvec, (argc + reallocation_increment) * sizeof(char*));
					for (j=argc; j<argc + reallocation_increment; j++) {
						argvec[j] = (char*)malloc(20 * sizeof(char));
					}
					argc += reallocation_increment;
				}
				strcpy(argvec[i], puppet->string);
				puppet = puppet->next_word;
				i++;
			}
			argvec[i] = NULL;
			execvp(s->verb->string, argvec);
			break;
		default:
			break;
	}
	waitpid(child_pid, &status, 0);
	if (WIFEXITED(status))
		printf("Child %d terminated normally, with code %d\n",
			   child_pid, WEXITSTATUS(status));

	return status;
}

/**
 * Process two commands in parallel, by creating two children.
 */
static bool do_in_parallel(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO execute cmd1 and cmd2 simultaneously */

	return true; /* TODO replace with actual exit status */
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2)
 */
static bool do_on_pipe(command_t *cmd1, command_t *cmd2, int level,
		command_t *father)
{
	/* TODO redirect the output of cmd1 to the input of cmd2 */

	return true; /* TODO replace with actual exit status */
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	/* TODO sanity checks */

	if (c->op == OP_NONE) {
		return parse_simple(c->scmd, 1, c);
	}

	switch (c->op) {
	case OP_SEQUENTIAL:
		/* TODO execute the commands one after the other */
		break;

	case OP_PARALLEL:
		/* TODO execute the commands simultaneously */
		break;

	case OP_CONDITIONAL_NZERO:
		/* TODO execute the second command only if the first one
		 * returns non zero
		 */
		break;

	case OP_CONDITIONAL_ZERO:
		/* TODO execute the second command only if the first one
		 * returns zero
		 */
		break;

	case OP_PIPE:
		/* TODO redirect the output of the first command to the
		 * input of the second
		 */
		break;

	default:
		return SHELL_EXIT;
	}

	return 0; /* TODO replace with actual exit code of command */
}
