#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "tree.h"
#include "proc-common.h"

#define SLEEP_PROC_SEC  10
#define SLEEP_TREE_SEC  3

void fork_procs(struct tree_node *node) {
	int wait_status;
	pid_t pid_child;
	change_pname(node->name);
	if (node->nr_children == 0) {
		printf("%s: Created\n", node->name);
		printf("%s: Sleeping...\n", node->name);
		sleep(SLEEP_PROC_SEC);
		printf("%s: Exiting...\n", node->name);
		exit(17); //value is arbitrary
	}
	printf("%s: Created\n", node->name);
	int i;
	for (i = 0; i < node->nr_children; i++) {
		pid_child = fork();
		if (pid_child < 0) {
			perror("fork-error");
			exit(1);
		}
		if(pid_child == 0) {
			fork_procs(node->children + i);
		}
	}
	printf("%s: Waiting...\n", node->name);
	for (i=0; i < node->nr_children; i++) {
		pid_child = wait(&wait_status);
		if (pid_child < 0) {
			perror("wait-error");
			exit(1);
		}
	}
	printf("%s: Exiting...\n", node->name);
	exit(21); //value is arbitrary
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
	pid_t pid;
	int status;
	struct tree_node *root;
	
	if (argc != 2) {
		printf("Usage: ./<file> <tree_file>\n");
		exit(1);
	}
	
	root = get_tree_from_file(argv[1]);
	printf("Print tree:\n");
	print_tree(root);
	printf("\n");
	
	/* Similar as in 1.1 */
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
	/* wait_for_ready_children(1); */

	/* for ask2-{fork, tree} */
	sleep(SLEEP_TREE_SEC);

	/* Print the process tree root at pid */
	show_pstree(pid);

	/* for ask2-signals */
	/* kill(pid, SIGCONT); */

	/* Wait for the root of the process tree to terminate */
	pid = wait(&status);
	explain_wait_status(pid, status);

	return 0;
}
