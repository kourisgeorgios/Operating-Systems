#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

/*
 * Create this process tree:
 * A-+-B---D
 *   `-C
 */
void fork_procs(void)
{
	/*
	 * initial process is A.
	 */
	pid_t Bpid, Cpid, Dpid, kill_child;
	int waitstatus;
	change_pname("A");
	printf("A: Creating...\n");	
	
	Bpid=fork();

	if (Bpid < 0) {
                perror("fork-error");
                exit(1);
        }
	else if (Bpid == 0) {
		change_pname("B");
		printf("B: Creating...\n");
		Dpid=fork();
		if (Dpid < 0) {
        	       perror("fork-error");
 	               exit(1);
        	}
		else if (Dpid == 0) {
			change_pname("D");
			printf("D: Creating...\n");
			printf("D: Sleeping...\n");
			sleep(SLEEP_PROC_SEC);
			printf("D: Exiting...\n");
			exit(13); //return code for D
		}
		else  {
                        printf("B: Waiting...\n");
                        kill_child = wait(&waitstatus);
                        if(kill_child < 0) {
                                perror("wait-error");
                                exit(1);
                        }
                        explain_wait_status(kill_child, waitstatus);
                        printf("B: Exiting...\n");
                        exit(19); //return code for B
                }
        }
	else  {
		Cpid=fork();
		if (Cpid < 0) {
                       perror("fork-error");
                       exit(1);
                }
		else if (Cpid == 0) {
                        change_pname("C");
                        printf("C: Creating...\n");
                        printf("C: Sleeping\n");
                        sleep(SLEEP_PROC_SEC);
                        printf("C: Exiting\n");
                        exit(17); //return code for C
                }
		else {
			printf("A: Waiting...\n");
			kill_child = wait(&waitstatus);
                        if(kill_child < 0) {
				perror("wait-error");
				exit(1);
			}
			explain_wait_status(kill_child, waitstatus);

			kill_child = wait(&waitstatus);
                        if(kill_child < 0) {
                                perror("wait-error");
                                exit(1);
                        }
                        explain_wait_status(kill_child, waitstatus);

		}	
	}
	printf("A: Exiting...\n");
        exit(16);
}

/*
 * The initial process forks the root of the process tree,
 * waits for the process tree to be completely created,
 * then takes a photo of it using show_pstree().
 *
 * How to wait for the process tree to be ready?
 * In ask2-{fork, tree}:
 *      wait for a few seconds, hope for the best.
 * In ask2-signals:
 *      use wait_for_ready_children() to wait until
 *      the first process raises SIGSTOP.
 */
int main(void)
{
	pid_t pid;
	int status;

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs();
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	if (pid < 0) {
		perror("wait-error");
		exit(1);
	}
	explain_wait_status(pid, status);

	return 0;
}
