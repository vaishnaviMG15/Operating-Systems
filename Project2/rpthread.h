// File:	rpthread_t.h

// List all group member's name: Vaishnavi Manthena (vm504) and Sanjana Pendharkar (sp1631)
// username of iLab: vm504 (vm504@iLab1.cs.rutgers.edu)
// iLab Server: iLab1.cs.rutgers.edu

#ifndef RTHREAD_T_H
#define RTHREAD_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_RTHREAD macro */
#define USE_RTHREAD 1

#ifndef TIMESLICE
/* defined timeslice to 5 ms, feel free to change this while testing your code
 * it can be done directly in the Makefile*/
#define TIMESLICE 5
#endif

#define READY 0
#define SCHEDULED 1
#define BLOCKED 2
#define DONE 3


/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
//#include <stdatomic.h>

typedef uint rpthread_t;

//typedef struct threadControlBlock {
typedef struct  {
	/* add important states in a thread control block */
	// thread Id
	rpthread_t tid;

	// thread status
	int status;

	// thread context
	ucontext_t cntx;
	
	//thread priority (for MLFQ)
	//This denotes the level that the thread is ready currently in or the running thread currently came out of 
	//could be 1 (lowest priority), 2, 3, 4(highest priority)
	int priority;
	
	// And more ...

	// YOUR CODE HERE
} tcb; 

typedef struct rpthread{

	tcb * controlBlock;

} rpthread;

/* mutex struct definition */
typedef struct rpthread_mutex_t {
	/* add something here */

	// YOUR CODE HERE

	//mutex inialization variable
	//This will be 0 when the mutex is available
	//1 when the mutex is not
	int flag;

	//pointer to the thread holding the mutex
	rpthread * t;


} rpthread_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE


typedef struct node{

	rpthread * thr;
	struct node * next;


}node;


typedef struct runqueue{

	//head
	node * head;
	//tail
	node * tail;

	

} runqueue;


/* Function Declarations: */

/* create a new thread */
int rpthread_create(rpthread_t *thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int rpthread_yield();

/* terminate a thread */
void rpthread_exit(void *value_ptr);

/* wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr);

/* initial the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex);

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex);

/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex);

/*print the run counts*/
//void printRunCounts();

#ifdef USE_RTHREAD
#define pthread_t rpthread_t
#define pthread_mutex_t rpthread_mutex_t
#define pthread_create rpthread_create
#define pthread_exit rpthread_exit
#define pthread_join rpthread_join
#define pthread_mutex_init rpthread_mutex_init
#define pthread_mutex_lock rpthread_mutex_lock
#define pthread_mutex_unlock rpthread_mutex_unlock
#define pthread_mutex_destroy rpthread_mutex_destroy
#endif

#endif
