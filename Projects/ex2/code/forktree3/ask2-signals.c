#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

void fork_procs(struct tree_node *root)
{	
	int wait_status;
	pid_t pid_child[root->nr_children];
	/*
	 * Start
	 */
	printf("PID = %ld, name %s, starting...\n",(long)getpid(), root->name);
	change_pname(root->name);

	if (root->nr_children == 0) {
		raise(SIGSTOP);
		printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
		exit(15); //arbitrary value for leaf procs
	}
	
	else {
		int i;
		for (i = 0; i < root->nr_children; i++) {
	        	pid_child[i] = fork();
        	        if (pid_child[i] < 0) {
                	        perror("fork-error");
                        	exit(1);
                	}
                	if(pid_child[i] == 0) {
                        	fork_procs(root->children + i);
                	}
        	}
		
		wait_for_ready_children(root->nr_children);

		raise(SIGSTOP);
		printf("PID = %ld, name = %s is awake\n",(long)getpid(), root->name);
		
		for (i=0; i < root->nr_children; i++) {
			kill(pid_child[i],SIGCONT);
               		pid_child[i] = wait(&wait_status);
			if (pid_child[i] < 0) {
				perror("wait-error");
				exit(1);
			}
			explain_wait_status(pid_child[i], wait_status);
        	}
		exit(12); //arbitrary value for non-leaf procs
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

int main(int argc, char *argv[])
{
	pid_t pid;
	int status;
	struct tree_node *root;

	if (argc < 2){
		fprintf(stderr, "Usage: %s <tree_file>\n", argv[0]);
		exit(1);
	}

	/* Read tree into memory */
	root = get_tree_from_file(argv[1]);
	printf("Print tree:\n");
        print_tree(root);
        printf("\n");

	/* Fork root of process tree */
	pid = fork();
	if (pid < 0) {
		perror("main: fork");
		exit(1);
	}
	if (pid == 0) {
		/* Child */
		fork_procs(root);
		exit(1);
	}

	/*
	 * Father
	 */
	/* for ask2-signals */
	wait_for_ready_children(1);

	/* for ask2-{fork, tree} */
	/* sleep(SLEEP_TREE_SEC); */

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	kill(pid, SIGCONT);

	/* Wait for the root of the process tree to terminate */
	wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
