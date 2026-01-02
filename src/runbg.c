#include "runbg.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>

/* Global variables ***********************************************************/
/* Function prototypes ********************************************************/
/* Callbacks ******************************************************************/
/* Function definitions *******************************************************/
/* Public functions ***********************************************************/

pid_t runbg_run(const char* path, ...)
{
	pid_t pid = fork();

	if (pid == 0) {
		va_list args;
		va_start(args, path);
		int n_args = 0;
		const char* arg;
		while ((arg = va_arg(args, const char*)) != NULL) {
			n_args++;
		}
		char** arg_list = malloc((n_args + 3) * sizeof(char*));
		va_start(args, path);
		for (int i = 0; i < n_args; i++) {
			arg_list[i+2] = (char*)va_arg(args, const char*);
		}
		arg_list[0] = "bash";
		arg_list[1] = (char*)path;
		arg_list[n_args+2] = NULL;
		execvp("/bin/bash", arg_list);
		exit(0);
	} else {
		return pid;
	}
}

int runbg_check(pid_t pid, int* running)
{
	int status;
	if (waitpid(pid, &status, WNOHANG) == pid) {
		if (WIFEXITED(status)) {
			*running = 0;
			return WEXITSTATUS(status);
		}
	}
	*running = 1;
	return 0;
}

int runbg_check_wait(pid_t pid)
{
	int status;
	waitpid(pid, &status, 0);
	//TODO: timeout?
	return WEXITSTATUS(status);
}