#ifndef RUNBG_H
#define RUNBG_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Execute a scipr in the background.
 *
 * The runbg_run function allows you to execute a script in the background.
 * It takes the path to the script as an argument and optionally a variable
 * list of arguments to pass to the script during its execution.
 *
 * @param path The path to the script to be executed.
 * @param ... A variable list of arguments for the script (optional).
 * @return Returns the process ID (pid_t) of the background script if the
 *         execution is successful. If an error occurs in creating the process,
 *         it returns -1 and sets errno appropriately.
 *
 * @note This function creates a child process using fork and then replaces
 *       the child process's image with the specified script. The child
 *       process runs in the background, and the parent process continues
 *       its normal execution.
 *
 * Usage example:
 * @code
 * pid_t pid = runbg_run("/path/to/script", "-arg1", "-arg2", NULL);
 * if (pid == -1) {
 *     perror("Error executing the script in the background");
 * } else {
 *     printf("Background script, Process ID: %d\n", pid);
 * }
 * @endcode
 */
pid_t runbg_run(const char* path, ...);

/**
 * @brief Check the status of a background process and determine if it's still
 *        running.
 *
 * The runbg_check function allows you to check the status of a background
 * process specified by its process ID (PID) and determine if it's still
 * running.
 *
 * @param pid The process ID (PID) of the background process to check.
 * @param running Pointer to an integer where the running status will be stored.
 *                It will be set to 1 if the process is still running, and 0 if
 *                it has terminated.
 * @return Returns an exit status value if the specified process has terminated.
 *         If the process is still running, it returns 0. If an error occurs,
 *         it returns -1 and sets errno appropriately.
 *
 * @note This function can be used to determine whether a background process
 *       has terminated and obtain its exit status. If the function returns 0,
 *       the process is still running, and the 'running' parameter will be set
 *       to 1.
 *       A nonzero value indicates that the process has terminated, and the
 *       value contains the exit status of the process. In this case, the
 *       'running' parameter will be set to 0.
 */
int runbg_check(pid_t pid, int* running);

/**
 * @brief Check the status of a background process in blocking mode.
 *
 * @param pid The process ID (PID) of the background process to check.
 */
int runbg_check_wait(pid_t pid);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* RUNBG_H */