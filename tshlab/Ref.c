int parseline(const char *cmdline, struct cmdline_tokens *tok); 
/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Parameters:
 *   cmdline:  The command line, in the form:
 *
 *                command [arguments...] [< infile] [> oufile] [&]
 *
 *   tok:      Pointer to a cmdline_tokens structure. The elements of this
 *             structure will be populated with the parsed tokens. Characters 
 *             enclosed in single or double quotes are treated as a single
 *             argument. 
 * Returns:
 *   1:        if the user has requested a BG job
 *   0:        if the user has requested a FG job  
 *  -1:        if cmdline is incorrectly formatted
 * 
 * Note:       The string elements of tok (e.g., argv[], infile, outfile) 
 *             are statically allocated inside parseline() and will be 
 *             overwritten the next time this function is invoked.
 */
void sigquit_handler(int sig);
/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */

void clearjob(struct job_t *job);
void initjobs(struct job_t *job_list);
int maxjid(struct job_t *job_list); 
int addjob(struct job_t *job_list, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *job_list, pid_t pid); 
pid_t fgpid(struct job_t *job_list);
struct job_t *getjobpid(struct job_t *job_list, pid_t pid);
struct job_t *getjobjid(struct job_t *job_list, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *job_list, int output_fd);
void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
ssize_t sio_puts(char s[]);
ssize_t sio_putl(long v);
ssize_t sio_put(const char *fmt, ...);
void sio_error(char s[]);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);
/* clearjob - Clear the entries in a job struct */
/* initjobs - Initialize the job list */
/* maxjid - Returns largest allocated job ID */
/* addjob - Add a job to the job list */
/* deletejob - Delete a job whose PID=pid from the job list */
/* fgpid - Return PID of current foreground job, 0 if no such job */
/* getjobpid  - Find a job (by PID) on the job list */
/* getjobjid  - Find a job (by JID) on the job list */
/* pid2jid - Map process ID to job ID */
/* listjobs - Print the job list */
/* usage - print a help message */
/* unix_error - unix-style error routine */
/* app_error - application-style error routine */
/* sio_puts - Put string */
/* sio_putl - Put long */
/* sio_put - Put to the console. only understands %d*/
/* sio_error - Put error message and exit */
/* Signal - wrapper for the sigaction function */

