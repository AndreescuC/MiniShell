/**
 * Operating Systems 2013-2017 - Assignment 2
 *
 * Andreescu Constantin, 333CC
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../Parser/utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <unistd.h>

#include "cmd.h"
#include "../Parser/utils.h"
#include "../Parser/parser.h"

#define READ		0
#define WRITE		1

/**
 * Internal change-directory command.
 */
static int shell_cd(simple_command_t *s)
{
	int saved_stdout = DEFAULT_STATUS;
    int saved_stdin = DEFAULT_STATUS;
    int saved_stderr = DEFAULT_STATUS;
	do_redirects(s, &saved_stdout, &saved_stdin, &saved_stderr);
	int status = chdir(s->params->string);
	if (saved_stdin != DEFAULT_STATUS) {
		dup2(saved_stdin, STDIN_FILENO);
		close(saved_stdin);
	}
	if (saved_stdout != DEFAULT_STATUS) {
		dup2(saved_stdout, STDOUT_FILENO);
		close(saved_stdout);
	}
	if (saved_stderr != DEFAULT_STATUS) {
		dup2(saved_stderr, STDERR_FILENO);
		close(saved_stderr);
	}

	return status == 0 ? 1 : 0;
}

/**
 * Internal exit/quit command.
 */
static int shell_exit(void)
{
    return SHELL_EXIT;
}

/**
 * Checks in command if redirects are necessary and performs
 * @param s
 * @param saved_stdout
 * @param saved_stdin
 * @param saved_stderr
 * @return
 */
static int do_redirects(simple_command_t *s, int *saved_stdout, int *saved_stdin, int *saved_stderr)
{
	int mode;
	if (s->in != NULL && strlen(s->in->string)) {
		int fd_in = open(s->in->string, O_RDWR | O_CREAT, 0777);
		*saved_stdin = dup(STDIN_FILENO);
		dup2(fd_in, STDIN_FILENO);
		close(fd_in);
	}
	if (s->out != NULL && strlen(s->out->string) &&
		s->err != NULL && strlen(s->err->string)) {

		mode = O_RDWR | O_CREAT | O_TRUNC;
		int fd_out_err = open(s->out->string, mode, 0777);
		*saved_stdout = dup(STDOUT_FILENO);
		*saved_stderr = dup(STDERR_FILENO);
		dup2(fd_out_err, STDOUT_FILENO);
		dup2(fd_out_err, STDERR_FILENO);
		close(fd_out_err);
	} else {

		if (s->out != NULL && strlen(s->out->string)) {
			mode = O_RDWR | O_CREAT;
			mode |= s->io_flags == IO_OUT_APPEND ? O_APPEND : O_TRUNC;
			int fd_out = open(s->out->string, mode, 0777);
			*saved_stdout = dup(STDOUT_FILENO);
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		}
		if (s->err != NULL && strlen(s->err->string)) {
			mode = O_RDWR | O_CREAT;
			mode |= s->io_flags == IO_ERR_APPEND ? O_APPEND : O_TRUNC;
			int fd_err = open(s->err->string, mode, 0777);
			*saved_stderr = dup(STDERR_FILENO);
			dup2(fd_err, STDERR_FILENO);
			close(fd_err);
		}
	}
}

/**
 * Checks for basic commands that do not require fork
 * @param s
 * @return
 */
static int check_basic(simple_command_t *s)
{
    if (strcmp(s->verb->string, "exit") == 0 || strcmp(s->verb->string, "quit") == 0) {
        return shell_exit();
    }
    if (strcmp(s->verb->string, "cd") == 0) {
        return shell_cd(s);
    }
    if (strcmp(s->verb->string, "true") == 0) {
        return 1;
    }
    if (strcmp(s->verb->string, "false") == 0) {
        return 0;
    }
    return DEFAULT_STATUS;
}

/**
 * Parse a simple command (internal, environment variable assignment,
 * external command).
 */
static int parse_simple(simple_command_t *s, int level, command_t *father)
{
	int saved_stdout = DEFAULT_STATUS;
    int saved_stdin = DEFAULT_STATUS;
    int saved_stderr = DEFAULT_STATUS;
    int j, argc, reallocation_increment;

    int status = check_basic(s);
    if (status != DEFAULT_STATUS) {
        return status;
    }

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

			do_redirects(s, &saved_stdout, &saved_stdin, &saved_stderr);

			execvp(s->verb->string, argvec);
			break;
		default:
			break;
	}
	waitpid(child_pid, &status, 0);
	return WIFEXITED(status) != 0 ? 1 : 0;
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

int handle_command(command_t *c)
{
    return c->scmd == NULL
           ? parse_command(c, 1, c->up)
           : parse_simple(c->scmd, 1, c->up);
}

/**
 * Parse and execute a command.
 */
int parse_command(command_t *c, int level, command_t *father)
{
	int status1 = DEFAULT_STATUS, status2 = DEFAULT_STATUS;

	if (c->op == OP_NONE) {
		return parse_simple(c->scmd, 1, c);
	}

	switch (c->op) {
	case OP_SEQUENTIAL:
        handle_command(c->cmd1);
        handle_command(c->cmd2);
		break;

	case OP_PARALLEL:
		/* TODO execute the commands simultaneously */
		break;

	case OP_CONDITIONAL_NZERO:
        status1 = handle_command(c->cmd1);
        if(!status1) {
            status2 = handle_command(c->cmd2);
            return status2;
        }
		break;

	case OP_CONDITIONAL_ZERO:
        status1 = handle_command(c->cmd1);
        if(status1) {
            status2 = handle_command(c->cmd2);
            return status2;
        }
        break;

	case OP_PIPE:
		/* TODO redirect the output of the first command to the
		 * input of the second
		 */
		break;

	default:
		return SHELL_EXIT;
	}
	return 1;
}
   