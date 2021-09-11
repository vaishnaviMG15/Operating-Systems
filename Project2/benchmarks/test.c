#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../rpthread.h"

/* A scratch program template on which to call and
 * test rpthread library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void func(){

	for(int i = 0; i < 10; i++){
		printf("tid: %i, value: %d",  , i);
	}

}
int main(int argc, char **argv) {

	/* Implement HERE */

	//int arg1 = 1;
	//int arg2 = 2
	pthread_t t1, t2;
	pthread_create(&t1, NULL, func, NULL);
	pthread_create(t2, NULL, func, NULL);

	return 0;
}
