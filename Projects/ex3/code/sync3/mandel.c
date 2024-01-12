/*
 * mandel.c
 *
 * A program to draw the Mandelbrot Set on a 256-color xterm.
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <errno.h>
#include "mandel-lib.h"

#define MANDEL_MAX_ITERATION 100000

/***************************
 * Compile-time parameters *
 ***************************/
/*
 * Output at the terminal is is x_chars wide by y_chars long
*/
int y_chars = 50;
int x_chars = 90;

/*
 * The part of the complex plane to be drawn:
 * upper left corner is (xmin, ymax), lower right corner is (xmax, ymin)
*/
double xmin = -1.8, xmax = 1.0;
double ymin = -1.0, ymax = 1.0;
	
/*
 * Every character in the final output is
 * xstep x ystep units wide on the complex plane.
 */
double xstep;
double ystep;

int num_threads;

pthread_cond_t cond[7];


/* case: usage of ctrl C*/
void sigint_handler(int signum) {
	reset_xterm_color(1);
	exit(1);
}

int safe_atoi(char *s, int *value) {
	long l;
	char *endp;
	l = strtol(s, &endp, 10);
	if(s != endp && *endp == '\0') {
		*value=l;
		return 0;
	} else
		return -1;
}

void *safe_malloc(size_t size)
{
	void *p;
	if((p=malloc(size))==NULL) {
		fprintf(stderr, "Out of memory, failed to allocate %zd bytes\n",
			size);
		exit(1);
	}
	return p;
}

/*
 * This function computes a line of output
 * as an array of x_char color values.
 */
void compute_mandel_line(int line, int color_val[])
{
	/*
	 * x and y traverse the complex plane.
	 */
	double x, y;

	int n;
	int val;

	/* Find out the y value corresponding to this line */
	y = ymax - ystep * line;

	/* and iterate for all points on this line */
	for (x = xmin, n = 0; n < x_chars; x+= xstep, n++) {

		/* Compute the point's color value */
		val = mandel_iterations_at_point(x, y, MANDEL_MAX_ITERATION);
		if (val > 255)
			val = 255;

		/* And store it in the color_val[] array */
		val = xterm_color(val);
		color_val[n] = val;
	}
}

/*
 * This function outputs an array of x_char color values
 * to a 256-color xterm.
 */
void output_mandel_line(int fd, int color_val[])
{
	int i;
	
	char point ='@';
	char newline='\n';

	for (i = 0; i < x_chars; i++) {
		/* Set the current color, then output the point */
		set_xterm_color(fd, color_val[i]);
		if (write(fd, &point, 1) != 1) {
			perror("compute_and_output_mandel_line: write point");
			exit(1);
		}
	}

	/* Now that the line is done, output a newline character */
	if (write(fd, &newline, 1) != 1) {
		perror("compute_and_output_mandel_line: write newline");
		exit(1);
	}
}
pthread_mutex_t condition_mutex[7] = {
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER,
PTHREAD_MUTEX_INITIALIZER
};

int first=1;
void *compute_and_output_mandel_line(void *thr)
{
	int color_val[x_chars];
	int i = (int)thr;
	//int ft=1;
        while(1) {
		//printf("1\n");
		compute_mandel_line(i, color_val);
		//pthread_cond_signal(&cond[i%num_threads]);
		pthread_mutex_lock(&condition_mutex[i%num_threads]);
	
			printf("Waiting %d\n", (i)%num_threads);
			printf("%d\n", i);
			pthread_cond_wait(&cond[i%num_threads],&condition_mutex[i%num_threads]);
		
		pthread_mutex_unlock(&condition_mutex[i%num_threads]);
		//printf("2\n");
		//if (ft) {
		//pthread_cond_wait(&cond[i%num_threads],&condition_mutex);
		//ft=0;
		//}
		//printf("3\n");
		//output_mandel_line(1, color_val);

		printf("Output %d\n", (i)%num_threads);
		output_mandel_line(1, color_val);
		//pthread_mutex_lock(&count_mutex);
		//output_mandel_line(1, color_val);
		pthread_mutex_lock(&condition_mutex[(i+1)%num_threads]);

		printf("Waking %d\n", (i+1)%num_threads);
		if (pthread_cond_signal(&cond[(i+1)%num_threads]) < 0) {
			perror("sem_post error");
			exit(1);
		}
		 pthread_mutex_unlock(&condition_mutex[(i+1)%num_threads]);

		i += num_threads;
		//if (first)
		//	first=0;
		//pthread_mutex_unlock(&count_mutex);
		if (i >= y_chars) 
			return NULL;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	 int i, ret;
        sigset_t sigset;

        xstep = (xmax - xmin) / x_chars;
        ystep = (ymax - ymin) / y_chars;
        if (argc != 2) {
                perror("Incorrect input, please insert only the number of threadsto create");
                exit(1);
        }
        if (safe_atoi(argv[1], &num_threads) < 0 || num_threads <= 0) {
                perror("input error");
                exit(1);
        }

        /*
         * draw the Mandelbrot Set, one line at a time.
         * Output is sent to file descriptor '1', i.e., standard output.
         */
	//pthread_cond_t cond[num_threads];
        struct sigaction sa;
        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0;
        sigemptyset(&sigset);
        sa.sa_mask = sigset;

        if (sigaction(SIGINT, &sa, NULL) < 0) {
                perror("sigaction");
                exit(1);
        }
 	
	if (pthread_cond_init(&cond[0], NULL) < 0) {
	         perror("pthread_cond_init error");
		         exit(1);
	}
pthread_mutex_lock(&condition_mutex[0]);
        pthread_cond_signal(&cond[0]);
         pthread_mutex_unlock(&condition_mutex[0]);


        //for (i = 0; i < num_threads; i++) {
	//	printf(" m\n");
	//	condition_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
	//}
	//for(i=0; i<num_threads; i++) {
	//	condition_mutex[i] = PTHREAD_MUTEX_INITIALIZER;
	//}

	for (i = 1; i < num_threads; i++) {
    		if (pthread_cond_init(&cond[i], NULL) < 0) {
        		perror("pthread_cond_init error");
        		exit(1);
    		}
	}	

        pthread_t thread[num_threads];
        for (i = 0; i < num_threads; i++) {
                ret = pthread_create(&thread[i], NULL, compute_and_output_mandel_line, (void*)(uintptr_t)i);
                if (ret) {
                        perror("pthread_create error");
                        exit(1);
                }
        }
	printf("wake up first\n");

        for (i = 0; i < num_threads; i++) {
                ret = pthread_join(thread[i], NULL);
                if (ret) {
                        perror("pthread_join error");
                }
        }
        for (i = 0; i < num_threads; i++) {
                if (pthread_cond_destroy(&cond[i]) < 0) {
                        perror("pthread_cond_destroy error");
                        exit(1);
                }
        }
        reset_xterm_color(1);
        return 0;
}
