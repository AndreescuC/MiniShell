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
static int shell_cd(simple_command_t *s, char* redirected_input)
{
	int saved_stdout = DEFAULT_STATUS;
    int saved_stdin = DEFAULT_STATUS;
    int saved_stderr = DEFAULT_STATUS;
	do_redirects(s, &saved_stdout, &saved_stdin, &saved_stderr);
	int status;
	if (strcmp(redirected_input, "") != 0) {
		status = chdir(redirected_input);
	} else {
		status = chdir(s->params->string);
	}
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
		s->err != NULL && strlen(s->err->string) &&
        strcmp(s->out->string, s->err->string) == 0) {

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

static int assign_env_var(const char *name, word_t* value, int overwrite)
{
	char buff[100] = "";
	word_t* puppet = value;
	while (puppet != NULL) {
		if (strlen(puppet->string)) {
            if (puppet->expand == true) {
                strcat(buff, getenv(puppet->string));
            } else {
                strcat(buff, puppet->string);
            }
		}
		puppet=puppet->next_part;
	}
	int status = setenv(name, buff, overwrite);
	return status == 0 ? 1 : 0 ;
}

/**
 * Checks for basic commands that do not require fork
 * @param s
 * @return
 */
static int check_basic(simple_command_t *s, char *redirected_input)
{
    if (strcmp(s->verb->string, "exit") == 0 || strcmp(s->verb->string, "quit") == 0) {
        return shell_exit();
    }
    if (strcmp(s->verb->string, "cd") == 0) {
        return shell_cd(s, redirected_input);
    }
    if (strcmp(s->verb->string, "true") == 0) {
        return 1;
    }
    if (strcmp(s->verb->string, "false") == 0) {
        return 0;
    }
	if (s->verb->next_part != NULL &&
            strcmp(s->verb->next_part->string, "=") == 0) {
		return assign_env_var(
				s->verb->string,
				s->verb->next_part->next_part,
				1
		);
	}
    return DEFAULT_STATUS;
}

/**
 * Run commands by creating an anonymous pipe (cmd1 | cmd2)
 */
static int do_on_pipe(command_t *cmd1, command_t *cmd2, int level,
                      command_t *father)
{

    int saved_stdout = DEFAULT_STATUS;
    int saved_stdin = DEFAULT_STATUS;
    int saved_stderr = DEFAULT_STATUS;
    int saved_stdin_pipe = DEFAULT_STATUS;

    int status = check_basic(cmd1->scmd, "");
    if (status == DEFAULT_STATUS) {

        int pipe_obj[2], j;
        pipe(pipe_obj);

        pid_t child_pid = fork();
        switch (child_pid) {
            case -1:
                DIE(child_pid, "fork");
            case 0:
                close(pipe_obj[0]);
                dup2(pipe_obj[1], STDOUT_FILENO);
                close(pipe_obj[1]);

                int argc;
                char **argvec = get_argv(cmd1->scmd, &argc);

                do_redirects(cmd1->scmd, &saved_stdout, &saved_stdin, &saved_stderr);

                execvp(cmd1->scmd->verb->string, argvec);
                printf("Execution failed for '%s'\n", cmd1->scmd->verb->string);
                exit(FAILED_CHILD);
            default:
                break;
        }
        waitpid(child_pid, &status, 0);

        saved_stdin_pipe = dup(STDIN_FILENO);
        close(pipe_obj[1]);
        dup2(pipe_obj[0], STDIN_FILENO);
        close(pipe_obj[0]);

        status = check_basic(cmd2->scmd, "");
        if (status != DEFAULT_STATUS) {
            return status;
        }
        char **argvec;
        child_pid = fork();
        switch (child_pid) {
            case -1:
                DIE(child_pid, "fork");
            case 0:

                argvec = (char**)malloc(2 * sizeof(char*));
                for (j=0; j<2; j++) {
                    argvec[j] = (char*)malloc(50 * sizeof(char));
                }

                do_redirects(cmd2->scmd, &saved_stdout, &saved_stdin, &saved_stderr);
                int size;
                argvec = get_argv(cmd2->scmd, &size);
                execvp(cmd2->scmd->verb->string, argvec);

                printf("Execution failed for '%s'\n", cmd1->scmd->verb->string);
                exit(FAILED_CHILD);
            default:
                break;
        }
        waitpid(child_pid, &status, 0);
        dup2(saved_stdin_pipe, STDIN_FILENO);
        close(saved_stdin_pipe);
        return WIFEXITED(status) != 0
               ? (WEXITSTATUS(status) == FAILED_CHILD ? 0 : 1)
               : 0;
    } else {
        return parse_simple(cmd2->scmd, 1, cmd2);
    }

}

static int run_on_pipe(simple_command_t *s, int prev_pipe[2], int next_pipe[2], bool stdin_flag,
                       bool stdout_flag) {

    int saved_stdout, saved_stdin, saved_stderr;
    saved_stdin = saved_stdout = saved_stderr = DEFAULT_STATUS;
    int status = check_basic(s, "");
    if (status != DEFAULT_STATUS) {
        return status;
    }
    pid_t child_pid = fork();
    switch (child_pid) {
        case -1:
            DIE(child_pid, "fork");
        case 0:

            if (stdin_flag == true) {
                dup2(prev_pipe[0], STDIN_FILENO);
                close(prev_pipe[1]);
                close(prev_pipe[0]);
            }

            if (stdout_flag == true) {
                dup2(next_pipe[1], STDOUT_FILENO);
                close(next_pipe[0]);
                close(next_pipe[1]);
            }

            int argc;
            char **argvec = get_argv(s, &argc);
            do_redirects(s, &saved_stdout, &saved_stdin, &saved_stderr);
            execvp(s->verb->string, argvec);
            printf("Execution failed for '%s'\n", s->verb->string);
            exit(FAILED_CHILD);
        default:
            break;
    }
    if (stdin_flag == true) {
        close(prev_pipe[0]);
        close(prev_pipe[1]);
    }
    waitpid(child_pid, &status, 0);
    return WIFEXITED(status) != 0
           ? (WEXITSTATUS(status) == FAILED_CHILD ? 0 : 1)
           : 0;
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
    int argc;
    char **argvec;

    int status = check_basic(s, "");
    if (status != DEFAULT_STATUS) {
        return status;
    }

	pid_t child_pid = fork();
	switch (child_pid) {
		case -1:
			DIE(child_pid, "fork");
		case 0:
            argvec = get_argv(s, &argc);
            do_redirects(s, &saved_stdout, &saved_stdin, &saved_stderr);
            execvp(s->verb->string, argvec);
            printf("Execution failed for '%s'\n", s->verb->string);
            exit(FAILED_CHILD);
		default:
			break;
	}
    waitpid(child_pid, &status, 0);
    return WIFEXITED(status) != 0
           ? (WEXITSTATUS(status) == FAILED_CHILD ? 0 : 1)
           : 0;
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

int handle_command(command_t *c)
{
    return c->scmd == NULL
           ? parse_command(c, 1, c->up)
           : parse_simple(c->scmd, 1, c->up);
}

command_t* dfs_in_order(command_t *c, int *pipeSize)
{
    int leftTreeSize = 0, rightTreeSize = 0;
    if (c->scmd != NULL) {
        *pipeSize = 1;
        return c;
    }
    command_t* c1 = dfs_in_order(c->cmd1, &leftTreeSize);
    command_t* puppet = c1;
    command_t* c2 = dfs_in_order(c->cmd2, &rightTreeSize);
    while (puppet->list_next != NULL) {
        puppet = puppet->list_next;
    }
    puppet->list_next = c2;
    *pipeSize = leftTreeSize + rightTreeSize;
    return c1;
}

int handle_pipe(command_t *c)
{
    int  index = 0, i, pipeSize = 0;
    command_t* puppet = dfs_in_order(c, &pipeSize);
    pipeSize ++;
    int pipes[pipeSize][2];
    for (i=0; i<pipeSize; i++) {
        pipe(pipes[i]);
    }
    run_on_pipe(puppet->scmd, pipes[index], pipes[index+1], false, true);
    index ++;
    puppet = puppet->list_next;
    while (puppet->list_next != NULL) {
        run_on_pipe(puppet->scmd, pipes[index], pipes[index+1], true, true);
        puppet = puppet->list_next;
        index ++;
    }
    int status = run_on_pipe(puppet->scmd, pipes[index], pipes[index+1], true, false);

    return status;
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
        return handle_pipe(c);

	default:
		return SHELL_EXIT;
	}
	return 1;
}
   