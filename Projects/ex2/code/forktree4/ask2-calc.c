#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include "tree.h"
#include "proc-common.h"


void fork_procs(struct tree_node *node, int pipe_fd) {
	int wait_status, pfd[2];
	int children = node->nr_children;
	int content = atoi(node->name);
	int content2[2];
	pid_t pid[children];
	if (node == NULL) {
		perror("node-error");
		exit(1);
	}
	change_pname(node->name);
	if (children == 0) {
		if (write(pipe_fd, &content, sizeof(content)) != sizeof(content)){
			perror("pipe-error: write to pipe");
			exit(1);
		}
		close(pipe_fd);
		raise(SIGSTOP);
		exit(17); //value is arbitrary
	}
	else {
		if (pipe(pfd) < 0) {
			perror("pipe-error");
			exit(1);
		}
		int i;
		for (i=0; i < children; i++) {
			pid[i] = fork();
			if (pid[i] < 0) {
				perror("fork-error");
				exit(1);
			}
			if (pid[i] == 0) {
				close(pfd[0]);
				fork_procs(node->children+i, pfd[1]);

			}
		}
		close(pfd[1]);
		for (i=0; i < children; i++) {
			if (read(pfd[0], &content2[i], sizeof(content2[i])) != sizeof(content2[i])) {
				perror("pipe-error: read from pipe");
				exit(1);
			}
		}
		close(pfd[0]);
		int result;

		switch(node->name[0]) {
			case '+':
				result = content2[0] + content2[1];
	                        break;
			case '*':
			        result = content2[0] * content2[1];
				break;
		        default:					           								      break;
		}
		printf("%ld, Calculation:  %i %s %i = %i\n", (long)getpid(), content2[0], node->name, content2[1], result);
		if (write(pipe_fd, &result, sizeof(result)) != sizeof(result)) {
			perror("pipe-error: write to pipe");
			exit(1);
		}
		close(pipe_fd);
		raise(SIGSTOP);
		for (i=0; i < children; i++) {
			kill(pid[i],SIGCONT);
			pid[i] = wait(&wait_status);
			if (pid[i] < 0) {
				perror("wait-error");
				exit(1);
			}
			explain_wait_status(pid[i], wait_status);
		}
		exit(12);
	}
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
int main(int argc, char **argv)
{	
	int something;
	int pfd[2]; /*from pipe-example.c*/
	pid_t pid;
	int status;
	struct tree_node *root;
	
	if (argc != 2) {
		printf("Usage: ./<file> <tree_file>\n");
		exit(1);
	}
	
	root = get_tree_from_file(argv[1]);
	
	printf("Parent: Creating pipe...\n");
        if (pipe(pfd) < 0) {
                perror("pipe-error");
                exit(1);
        }

        printf("Parent: Creating child...\n");
        pid = fork();
        if (pid < 0) {
                /* fork failed */
                perror("fork-error");
                exit(1);
        }
	if (pid == 0) {
                /* In child process */
		close(pfd[0]);
                fork_procs(root,pfd[1]);
        }
	/* Parents */
	// now we read

	close(pfd[1]); //we dont write...
	if (read(pfd[0], &something, sizeof(something)) != sizeof(something)) {
                perror("10 pipe-error: read from pipe");
                exit(1);
        }
	close(pfd[0]);
	show_pstree(pid);

	kill(pid, SIGCONT); /* We use signals, not sleep() */

	pid = wait(&status);
	explain_wait_status(pid, status);

	printf("The result of the expression is: %i\n", something);
	return 0;
}
