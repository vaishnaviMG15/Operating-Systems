// File:	rpthread.c

// List all group member's name: Vaishnavi Manthena (vm504) and Sanjana Pendharkar(sp1631)
// username of iLab: vm504 (vm504@iLab1.cs.rutgers.edu)
// iLab Server: iLab1.cs.rutgers.edu

#include "rpthread.h"


// INITAILIZE ALL YOUR VARIABLES HERE
// YOUR CODE HERE
#define STACK_SIZE SIGSTKSZ

//function definitions
void ring(int signum);
void addToRunQueue(rpthread * newThread, runqueue * queue);
//void setBlocked(rpthread * threadOfChoice);
void blockToRun();
void freeThread(rpthread * thread);
static void sched_rr(runqueue * queue);
static void sched_mlfq();
static void schedule();
//void testfunction();
//void printScheduler();



//val is used for unique thread id's
rpthread_t val = 1;

//scheduler is the queue with all the threads which are ready
//This can be thought of as the job queue for RR and the level4 queue for MLFQ
struct runqueue * scheduler; //highest priority in case of MLFQ 

//I will implement a 4 level MLFQ
//The first level can be implemented through the job queue for RR
//The other 3 levels are below

struct runqueue * level1; //least priority
struct runqueue * level2;
struct runqueue * level3; 

//blocked list: to hold the threads requiring a locked lock
struct runqueue * blockedList = NULL;

//To store the most recent/current user thread that is running (excluding scheduler thread)
rpthread * current = NULL;


//For the scheduler to know if the current thread has been blocked
int isBlocked = 0;

//For the scheduler to know if the current thread yielded within its time slice
int didYield = 0;

//To check if the scheduler has started its function
int started = 0;

//sch and sch_t are the context and thread of the scheduler respectively
ucontext_t sch;
rpthread * sch_t;

//signal handler
struct sigaction sa;

//initializing the timer
struct itimerval timer;

//Array to store the exit values of all the threads
void * exitValues[250];
int didExit[250];

//for debugging RR
//int runCounts[150];

//initialize the structures
int isInitialized =0;


//
ucontext_t mainContext;
rpthread * mainThread;
void initialize()
{
	if (isInitialized == 0){

		isInitialized = 1;

		//initializing arrays
		for (int i =0; i<250; i++){
			didExit[i] = 0;
			exitValues[i] = NULL;
			//runCounts[i] = 0;
		
		}

		//registering signal handler for when a timer interrupt occurs
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = &ring;
		sigaction(SIGPROF, &sa, NULL);


		//creating scheduler, thread structure for scheduler
		scheduler = (runqueue *)malloc(sizeof(runqueue));

		//if mlfq option has been selected then space has to be allocated for the other 3 levels as well.

		level1 = (runqueue *)malloc(sizeof(runqueue));
		level2 = (runqueue *)malloc(sizeof(runqueue));
		level3 = (runqueue *)malloc(sizeof(runqueue));

		//initializing the queue with blocked threads
		blockedList = (runqueue *)malloc(sizeof(runqueue));

		//now set the global context so that program goes to schedule function
		//so , we can use sch later whenever we want to go to schedule function
		if (getcontext(&sch) < 0){
			perror("getcontext");
			exit(1);
		}
		//updating golbal context structure
		void *stack=malloc(STACK_SIZE);
		sch.uc_link=NULL;
		sch.uc_stack.ss_sp=stack;
		sch.uc_stack.ss_size=STACK_SIZE;
		sch.uc_stack.ss_flags=0;

		//adding the scheduler's context to its tcb which is in its thread
		makecontext(&sch, schedule, 0);

		

	}

}


/* create a new thread */
int rpthread_create(rpthread_t *thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {
       // create Thread Control Block
       // create and initialize the context of this thread
       // allocate space of stack for this thread to run
       // after everything is all set, push this thread int
       // YOUR CODE HERE
      
	//This method remains the same for both RR and MLFQ because we always ass a new thread to the top level

       initialize();
	
       
	if (started == 0){
		
		getcontext(&mainContext);
		if(started == 0)
		{
				
			mainThread = (rpthread *)malloc(sizeof(rpthread));
			mainThread -> controlBlock = (tcb *)malloc(sizeof(tcb));
			mainThread -> controlBlock -> tid = val;
			val ++;
	        	mainThread -> controlBlock -> status = READY;	
			mainThread -> controlBlock -> cntx = mainContext;
			mainThread -> controlBlock -> priority = 4;
			addToRunQueue(mainThread, scheduler);
			started = 1;
			setcontext(&sch);

		}

		
	}

	//creating the thread struct 
	rpthread * newThread = (rpthread *)malloc(sizeof (rpthread));
	newThread -> controlBlock = (tcb *)malloc(sizeof(tcb));

       
	//assigning a unique id to this thread
	*thread = val;
	(newThread -> controlBlock) -> tid = *thread;
	val++;

	//Once the thread is created it will be ready to run on CPU
	//updating the status accordingly
	(newThread -> controlBlock) -> status = READY;

	//The priority values will be ignored by the RR scheduler
	(newThread -> controlBlock) -> priority = 4;

	//setting up the context and its stack
	ucontext_t c;
	if (getcontext(&c) < 0){
		perror("getcontext");
		exit(1);
	}
	void * thisStack = malloc(STACK_SIZE);
	c.uc_link = NULL;
	c.uc_stack.ss_sp = thisStack;
	c.uc_stack.ss_size = STACK_SIZE;
	c.uc_stack.ss_flags = 0;
	
	//using make context to set the context of the thread

	if (arg == NULL){
		makecontext(&c, (void *)function, 0);
	}else{
		makecontext(&c, (void *)function, 1,arg);
	}
	

	//the tcb's context is set to this context of the thread
	(newThread -> controlBlock) -> cntx = c;
	

	//We are adding this thread to the runqueue
	addToRunQueue(newThread, scheduler);

    return 0;
};

/* give CPU possession to other user-level threads voluntarily */
int rpthread_yield() {
	
	// change thread state from Running to Ready
	// save context of this thread to its thread control block
	// wwitch from thread context to scheduler context

	// YOUR CODE HERE
	
	
	(current -> controlBlock) -> status = READY;
	
	//This lets the scheduler for MLFQ know that the thread has yielded before its time slice
	didYield = 1;

	
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;

	swapcontext(&((current -> controlBlock)->cntx), &sch);
	
	
	return 0;

};

/* terminate a thread */
void rpthread_exit(void *value_ptr) {
	// Deallocated any dynamic memory created when starting this thread
	
	// YOUR CODE HERE

	//we are exiting from the current thread
	
	int index = current -> controlBlock ->tid; 

	if(value_ptr != NULL){
		//store the value pointed by value_ptr to the array index for this thread id (useful in the join function)
		exitValues[index] = value_ptr;
	}else{
		exitValues[index] = NULL;
	}

	//indicating this thread has exited (useful in the join function)
	didExit[index] = 1;
	
	freeThread(current);

	//So, scheduler knows not to enqueue
	current = NULL;

	
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 0;
	
	setcontext(&sch);

};


/* Wait for thread termination */
int rpthread_join(rpthread_t thread, void **value_ptr) {
	
	// de-allocate any dynamic memory created by the joining thread
  
	// YOUR CODE HERE

	//also when an exit happens all the dynamic memory will be freed automatically
	//The fact that this thread exited will be reflected in the didExit array:
	//that particular entry will be 1 when the thread exits.
	//so wait till the array entry is 1.
	while (didExit[thread]==0){

	}

	//If needed we will store the value returned by the exited array in value_ptr

	if (value_ptr != NULL){
		
		*value_ptr = exitValues[thread];

	}

	return 0;
};

/* initialize the mutex lock */
int rpthread_mutex_init(rpthread_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//initialize data structures for this mutex

	// YOUR CODE HERE
	
	//checking for invalid pointer:
	if(mutex == NULL){
		return -1;
	}
	//allocating mutex struct (already done on user side)
	//mutex = (rpthread_mutex_t *)malloc(sizeof (rpthread_mutex_t));

	//The flag should be initialized to 0.
	mutex -> flag = 0;

	//Initializing the thread of this mutex
	//mutex -> t = (rpthread *)malloc(sizeof(rpthread));
	//we do not need this line because when we create threads we already allocate memory
	//this thread will simply point to that memory

	return 0;

};

/* aquire the mutex lock */
int rpthread_mutex_lock(rpthread_mutex_t *mutex) {
    	// use the built-in test-and-set atomic function to test the mutex
        // if the mutex is acquired successfully, enter the critical section
        // if acquiring mutex fails, push current thread into block list and //  
        // context switch to the scheduler thread

        // YOUR CODE HERE

		if(!(__atomic_test_and_set (&mutex->flag, 1) == 0)){

			//lock could not be acquired cause it is owned by someother thread

			//push current thread into block list
			current -> controlBlock -> status = BLOCKED;
			addToRunQueue(current, blockedList);

			//so that later the scheduler does not enqueue this thread again
			isBlocked = 1;
			
			//saving current context and moving to scheduler
			swapcontext(&((current -> controlBlock)->cntx), &sch);
	

		}

		//The lock has been acquired by current thread

		//This mutex's thread should be this current thread
		mutex -> t = current;
		//I am setting the flag to 1 again, just in case
		mutex -> flag = 1;

        return 0;
};

/* release the mutex lock */
int rpthread_mutex_unlock(rpthread_mutex_t *mutex) {
	// Release mutex and make it available again. 
	// Put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE

	mutex -> flag = 0;
	mutex -> t = NULL;
	//sending everything from blocked list pack to job queue
	blockToRun();

	return 0;
};


/* destroy the mutex */
int rpthread_mutex_destroy(rpthread_mutex_t *mutex) {
	// Deallocate dynamic memory created in rpthread_mutex_init
	//freeThread(mutex -> t);
	//This line is not included because we do not want to free the actual thread
	
	//This line is not included because user is responsible for freeing the mutex	
	//free(mutex);
	
	if (mutex == NULL){
		return -1;
	}else{
		return 0;
	}
	
	return 0;
};

/* scheduler */
static void schedule() {
	// Every time when timer interrup happens, your thread library 
	// should be contexted switched from thread context to this 
	// schedule function

	// Invoke different actual scheduling algorithms
	// according to policy (RR or MLFQ)

	// if (sched == RR)
	//		sched_rr();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE
	
	// schedule policy
	#ifndef MLFQ
		// Choose RR
     		// CODE 1
     		sched_rr(scheduler);
	#else 
		// Choose MLFQ
     		// CODE 2
     		sched_mlfq();
	#endif

}

/* Round Robin (RR) scheduling algorithm */
static void sched_rr(runqueue * queue) {
	// Your own implementation of RR
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	//schedular 
	//1. should enqueue the current thread if:
	//	the thread is not blocked (meaning its in blocked list) and if it is not NULL (meaning it exited)
	//Note: Step 1 only occurs if this is RR. For MLFQ this step is done in sched_mlfq	
	//2. should pop the current head
	//3. set current to this thread
	//4. set isBlocked to false
	//5. free head node's memory 
	
	
	
	#ifndef MLFQ

		if (current != NULL && isBlocked != 1){
			current -> controlBlock -> status = READY;
			addToRunQueue(current, scheduler);

		}
	#endif

	if (queue->head !=NULL){

		//popping head from scheduler
		node * p = queue -> head;
		queue -> head = queue -> head -> next;

		//checking for the case where the scheduler originally just had one thread
		if (queue -> head == NULL){
			queue -> tail = NULL;
		}

		//isolating p
		p->next = NULL;

		//p is now a single unconnected node holding the next thread to schedule

		current = p -> thr;
		isBlocked = 0;
		current -> controlBlock -> status = SCHEDULED;

		free(p);

		timer.it_value.tv_usec = TIMESLICE*1000;
		timer.it_value.tv_sec = 0;
		setitimer(ITIMER_PROF, &timer, NULL);

		//testing
		//runCounts[current -> controlBlock -> tid] ++;
		
		setcontext(&(current -> controlBlock->cntx));

		

	}

}

/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// Your own implementation of MLFQ
	
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	
	// enqueue the current thread back into MLFQ
	// if the thread has not been blocked or has not exited
	if(current != NULL && isBlocked != 1){


		//getting priority of current thread
		int p = current -> controlBlock -> priority;
		current -> controlBlock -> status = READY;

		if (didYield == 1){

			
			//the thread should be placed in same priority if it did  yield
			
			if(p == 1){
				addToRunQueue(current, level1);
			}else if (p == 2){
				
				addToRunQueue(current, level2);
			}else if (p == 3){
				
				addToRunQueue(current, level3);
			}else{
				// p is 4
				
				addToRunQueue(current, scheduler);
			}
			
			//setting didYield to 0 for the next thread (which will be scheduled in RR)
			didYield = 0;

		}else{

				
			//if the thread has not yielded, then it should be placed in a lower priority.
			//if it is already at the lowest priority it has to stay in this priority,
			//else it will move to a lower priority
			if(p == 1){
				addToRunQueue(current, level1);
			}else if (p == 2){
				current -> controlBlock -> priority =1; 
				addToRunQueue(current, level1);

			}else if (p == 3){
				
				current -> controlBlock -> priority =2; 
				addToRunQueue(current, level2);
			}else{
				// p is 4
				
				current -> controlBlock -> priority =3; 
				addToRunQueue(current, level3);
			}
			
		}
	}

	//Now we just need to perform RR in the nonempty level with highest priority
	if (scheduler->head!=NULL){
		sched_rr(scheduler);
	}else if (level3->head != NULL){
		sched_rr(level3);
	}else if (level2->head != NULL){
		sched_rr(level2);
	}else if (level1->head != NULL){
		sched_rr(level1);
	}

}

// Feel free to add any other functions you need

// YOUR CODE HERE
/*
	This function calls the scheduler.
	This is what is supposed to happen when the timer goes off.

*/
void ring(int signum){


	swapcontext(&((current -> controlBlock)->cntx), &sch);

}


//code to enqueue a thread to the end of a particular queue
void addToRunQueue(rpthread * newThread, struct runqueue * queue){

	if((queue -> tail) != NULL){
		//there is at least on element in the runqqueue
		node *t  = (node *)malloc(sizeof (node));
       		t -> thr = newThread;
		t -> next = NULL;

		(queue -> tail)->next = t;
       		(queue -> tail) = t;	


	}else{

		//There is nothing in sheduler till now
		node * t = (node *)malloc(sizeof (node));
		t -> thr = newThread;
		t -> next = NULL;

		queue -> tail = t;
	    	queue -> head = t;	

	}
}

/*
//helper function to add a thread to the blocked list
void setBlocked(rpthread * threadOfChoice) {
	node * head = (node *)malloc(sizeof (node));
	head -> thr = threadOfChoice;
	head -> next = blockedList;
	blockedList = head;
}
*/


//helper function to add all threads in the block queue to the runqueue
void blockToRun() {
	//Take each thread from the blocked list and add it to runqueue based on priority
	//Free that node in blocked list
	node * ptr = blockedList -> head;
	node * prev;
	prev = ptr;
	while (ptr != NULL){

		ptr->thr->controlBlock->status = READY;
		#ifndef MLFQ

			//RR
			//just add to queue 'scheduler'
			addToRunQueue(ptr -> thr, scheduler);

		#else

			int p = ptr -> thr -> controlBlock -> priority;
					
			if(p == 1){
				addToRunQueue(ptr-> thr, level1);
			}else if (p == 2){
				
				addToRunQueue(ptr->thr, level2);
			}else if (p == 3){
				
				addToRunQueue(ptr->thr, level3);
			}else{
				// p is 4
				
				addToRunQueue(ptr->thr, scheduler);
			}	


		#endif

		ptr = ptr -> next;
		free(prev);
		prev = ptr;

	}

	blockedList -> head = NULL;
	blockedList -> tail = NULL;
}

//helper: given a thread, it frees all memory associated with it
void freeThread(rpthread * thread){

	//freeing stack, controlBlock, thread
	free(((thread -> controlBlock) -> cntx).uc_stack.ss_sp);
	free(thread -> controlBlock);
	free(thread);

}

/*

//only used this for debugging
void testfunction()
{
	printf("testfunction: you are threaded now \n");
}

//only used this for debugging
void printScheduler()
{
	if(scheduler == NULL)
	{
		printf("rpthread:printScheduler: scheduler is NULL \n");
		return;
	}
	if(scheduler->head == NULL)
	{
		printf("rpthread:printScheduler: scheduler head is NULL \n");
		return;
	}
	node * temphead = scheduler->head;
	int i=1;
	while (temphead != NULL)
	{
		printf("rpthread:printScheduler:from head %d: %u : %d \n",i,temphead->thr->controlBlock->tid, temphead->thr->controlBlock->status);
		i++;
		temphead = temphead->next;
		
	}
	if(scheduler->tail == NULL)
	{
		printf("rpthread:printScheduler: scheduler tail is NULL\n");
		return;
	}
	node * temptail = scheduler->tail;
	i=1;
	while (temptail != NULL)
	{
		printf("rpthread:printScheduler:from head %d: %u : %d\n",i,temptail->thr->controlBlock->tid, temptail->thr->controlBlock->status);
		i++;
		temptail = temptail->next;
		
	}


}
*/
/*
//only used for testing RR
void printRunCounts(){


	for (int i= 1; i<25; i++){
		printf("tid=  %d : number of times it ran: %d\n", i-1, runCounts[i] );

	}	



}
*/

