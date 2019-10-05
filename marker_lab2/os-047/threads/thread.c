#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

#define RUNNING 0
#define READY 1
#define TO_BE_KILLED 2
#define DEATHROW 3 
//#define RUNNING 0
/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in Lab 3 ... */
};

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid thread_id;
	ucontext_t thread_context;
	int thread_state;
	void* stack_head;
	//pointer to first byte in stack
	struct thread* next_thread; 
};

struct thread_identifier{

	Tid id;
        struct thread_identifier* next;	
};

struct thread_queue {
	
	int counter;
	struct thread_identifier* head; 
};

//GLOBAL VARIABLES
/********************************************************************************************************/
int used_thread_list[THREAD_MAX_THREADS]={0}; //This holds binary values for whether a thread is in use or nah
struct thread thread_list[THREAD_MAX_THREADS];
 Tid current_thread;// = (struct thread *)malloc(sizeof(struct thread));
struct thread_queue *thread_ready_queue;
struct thread_queue *thread_kill_queue;
/********************************************************************************************************/


void
thread_init(void)//this is where i handle the thread 0 stuff
{
	//current_thread = (struct thread *)malloc(sizeof(struct thread)); //Creates the thread object of the currently running thread
/////////////////
	thread_ready_queue = (struct thread_queue*)malloc(sizeof(struct thread_queue));//creates the ready_queue with nothing in it
	thread_ready_queue->head = NULL;
	thread_ready_queue->counter =0;
/////////////////
	thread_kill_queue = (struct thread_queue*)malloc(sizeof(struct thread_queue));//creates kill queue
	thread_kill_queue->head = NULL;
	thread_kill_queue->counter =0;
/////////////////
	current_thread = 0;	
	used_thread_list[0] = 1;//sets thread 0 as being used in user_thread_list
	thread_list[current_thread].thread_state = RUNNING; //sets the thread state to running
	thread_list[current_thread].next_thread = NULL; // points to NULL since its not in the ready queue
	thread_list[current_thread].thread_id = 0;
}

///////////////////////////////////////////////////////////////////////////

Tid
thread_id()
{
	return current_thread;
}

///////////////////////////////////////////////////////////////////////////

//STUB function
void
thread_stub(void (*thread_main)(void *), void *arg)
{
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	// all threads are done, so process should exit
	exit(0);
}

///////////////////////////////////////////////////////////////////////////

Tid
thread_create(void (*fn) (void *), void *parg)
{
	//need to allocate thread stack and create thread
	int found_thread_vacancy = 0;
	Tid new_thread_id;
	for(int i = 0;i<THREAD_MAX_THREADS;i++){
		if(used_thread_list[i] == 0){ //found a vacancy
			//this is the new thread_id for the thread
			found_thread_vacancy = 1;
			new_thread_id = i;
			
			break;
		}
		
	}
	if(found_thread_vacancy == 0){
		return THREAD_NOMORE;
	}else if(found_thread_vacancy == 1 ){
		
		//struct thread *new_thread = (struct thread *)malloc(sizeof(struct thread));
		//if(new_thread == NULL){
		//	return THREAD_NOMEMORY;
		//}
		thread_list[new_thread_id].thread_id = new_thread_id;
		used_thread_list[new_thread_id] = 1;	//indicates that the thread in the list is now being used
		thread_list[new_thread_id].next_thread = NULL;
		thread_list[new_thread_id].thread_state = READY;
		int ret = getcontext(&thread_list[new_thread_id].thread_context);
		assert(!ret);
	//STACK STUFF
		void* stack = (void *)malloc (THREAD_MIN_STACK);
		if(stack == NULL){
			return THREAD_NOMEMORY;
		}
		thread_list[new_thread_id].stack_head = stack;
		stack = stack + THREAD_MIN_STACK;
    		stack-= (unsigned long)stack %16;
    		stack-= 8;
		thread_list[new_thread_id].thread_context.uc_mcontext.gregs[REG_RSP] = (unsigned long)stack; //pointer to the first element in the aligned stack
		thread_list[new_thread_id].thread_context.uc_stack.ss_flags = 0;
		thread_list[new_thread_id].thread_context.uc_stack.ss_size = THREAD_MIN_STACK; 
	//Stub stuff
		thread_list[new_thread_id].thread_context.uc_mcontext.gregs[REG_RIP] = (unsigned long)&thread_stub; //pointer to stub function
		thread_list[new_thread_id].thread_context.uc_mcontext.gregs[REG_RDI] =  (unsigned long)fn; // first argument in func
		thread_list[new_thread_id].thread_context.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg; //second argument in func

	//NOW WE PUT IT INTO READY QUEUE
		struct thread_identifier* new_thread = 	(struct thread_identifier *)malloc(sizeof(struct thread_identifier));
		new_thread->id = new_thread_id;
	        new_thread->next= NULL;	
		if( thread_ready_queue->head == NULL){
			thread_ready_queue->head = new_thread;
			thread_ready_queue->counter++;
			return new_thread_id;
		}
		else{
			struct thread_identifier *temp_thread = thread_ready_queue->head;
			while(temp_thread->next != NULL){ //iterate until last node in ready_queue
				temp_thread = temp_thread->next;
			}
			temp_thread->next = new_thread;//set the last element as current thread
			thread_ready_queue->counter++;	
			return new_thread_id;
		}
	}
	return THREAD_NOMEMORY;


}

///////////////////////////////////////////////////////////////////////////

Tid
thread_yield(Tid want_tid)
{//clean up kill queue in here as well as kill function

	if(thread_list[current_thread].thread_state == TO_BE_KILLED){//////COME BACK HERE
		thread_exit();
	}

	if(want_tid == THREAD_SELF || want_tid == current_thread){//run itself
//		sought_id = caller_id;
		 int setcontext_called;

		setcontext_called = 0;
		int ret = getcontext(&thread_list[current_thread].thread_context);
		assert(!ret);
		if(setcontext_called == 1){//this condition is needed for the second return from getconstext()
			
			return current_thread;
		}
		setcontext_called =1;
		ret = setcontext(&thread_list[current_thread].thread_context);
		assert(!ret);
		
	}
	else if(want_tid == THREAD_ANY || ( thread_ready_queue->head!=NULL && (want_tid >= 0 && want_tid <= THREAD_MAX_THREADS - 1) && want_tid == thread_ready_queue->head->id)){//run head
		
		//struct thread_identifier *temp_thread;
		//get the head, output can be THREAD_NONE
		volatile int destroy = 0;
		if(thread_kill_queue->head != NULL){
			destroy = 1; 
		}
		if(destroy == 1){
			struct thread_identifier *rejuvenated_thread= thread_ready_queue->head;//extracted next runner
	
			thread_ready_queue->head = thread_ready_queue->head->next; //making the second node in the queue as the head
			struct thread_identifier *inserted_thread= (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
			rejuvenated_thread->next = NULL;
	
			thread_list[rejuvenated_thread->id].thread_state = RUNNING;


		//now we pop out the out	
			 int setcontext_called=0;
			int ret = getcontext(&thread_list[inserted_thread->id].thread_context);
			assert(!ret);
			if(destroy==1){
				used_thread_list[current_thread] = 0;
				free(thread_list[current_thread].stack_head);
				thread_kill_queue->head = thread_kill_queue->head->next;
				if(thread_kill_queue->head== NULL){
					destroy = 0;
				}
			/////////////////////****************************************
			}

			if(setcontext_called == 1){

				return (rejuvenated_thread->id);
			}
			setcontext_called = 1;
			ret = setcontext(&thread_list[rejuvenated_thread->id].thread_context);
			assert(!ret);
				
		
		}
		else{
			if(thread_ready_queue->head == NULL){//chekcs if arrays is empty
				return THREAD_NONE;
			}

			struct thread_identifier *rejuvenated_thread= thread_ready_queue->head;//extracted next runner
	
			thread_ready_queue->head = thread_ready_queue->head->next; //making the second node in the queue as the head
			struct thread_identifier *inserted_thread= (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
			rejuvenated_thread->next = NULL;
			//////////////////////****************************************************
			inserted_thread->id = current_thread;
	 		inserted_thread->next = NULL; //set it last node's pointer to NULL (old current)
	
			if(thread_ready_queue->head == NULL){
				thread_ready_queue->head = inserted_thread; 
			}
			else{

				struct thread_identifier *temp_thread = thread_ready_queue->head;
				while(temp_thread->next != NULL){ //iterate until last node in ready_queue
					temp_thread = temp_thread->next;
				}

				temp_thread->next = inserted_thread;
		
			}
			thread_list[current_thread].thread_state = READY;
			current_thread = rejuvenated_thread->id;
				
			thread_list[rejuvenated_thread->id].thread_state = RUNNING;


			//now we pop out the out	
			 int setcontext_called=0;
			int ret = getcontext(&thread_list[inserted_thread->id].thread_context);
			assert(!ret);


			if(setcontext_called == 1){

				return (rejuvenated_thread->id);
			}
			setcontext_called = 1;
			ret = setcontext(&thread_list[rejuvenated_thread->id].thread_context);
			assert(!ret);
				
		
		}	

	}
	else if(want_tid >= 0 && want_tid <= THREAD_MAX_THREADS - 1){//RUN Tid

		

		
		if(thread_ready_queue->head == NULL){//fine
			return THREAD_INVALID;
		}
		if(used_thread_list[want_tid] == 0){//fine
			return THREAD_INVALID;
		}

		struct thread_identifier *temp_thread;
		struct thread_identifier *next_thread;	
		temp_thread =thread_ready_queue->head; 	
		next_thread = temp_thread->next;
		while(next_thread!=NULL ){ //this loop is going to be used to find the thread with the corresponding Tid
			if(next_thread->id == want_tid){
				break;
			}
			temp_thread = next_thread;
			next_thread = next_thread->next;
		}
		if(next_thread == NULL){


			return THREAD_INVALID;// this is fine
		}
		else{


			struct thread_identifier *rejuvenated_thread;
			struct thread_identifier *temp = thread_ready_queue->head;
			rejuvenated_thread = next_thread;
			temp_thread->next = next_thread->next;
			rejuvenated_thread->next = NULL;
			struct thread_identifier *inserted_thread = (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
			inserted_thread->id = current_thread;
			inserted_thread->next = NULL;
			if(thread_ready_queue->head == NULL){
				thread_ready_queue->head = inserted_thread;
			}
			else{
				while(temp->next!=NULL){
					temp = temp->next;
				}
				temp->next = inserted_thread;
			} //at this point, the running thread has been pushed into the ready queue

			thread_list[inserted_thread->id].thread_state = READY;
			thread_list[rejuvenated_thread->id].thread_state = RUNNING;
			current_thread = rejuvenated_thread->id;
			int setcontext_called = 0;
			int ret = getcontext(&thread_list[inserted_thread->id].thread_context);
			assert(!ret);
			if(setcontext_called == 1){	
				return (rejuvenated_thread->id);
			}
			setcontext_called = 1;
			ret = setcontext(&thread_list[rejuvenated_thread->id].thread_context);
			assert(!ret);
			//free(rejuvenated_thread);
		}
	}

	else{

		return THREAD_INVALID;

	}
	return THREAD_FAILED;
}

////////////////////////////////////////////////////////////////////////////

Tid
thread_exit()
{//push to kill queue when this function is called 
	if(thread_ready_queue->head == NULL){
//		thread_list[current_thread].thread_state = RUNNING;
		return THREAD_NONE;
	}

	if(thread_kill_queue->head == NULL){
		struct thread_identifier *next_thread = thread_ready_queue->head;
		struct thread_identifier *prev_thread;
		if(thread_ready_queue->head->id == current_thread){
			next_thread = thread_ready_queue->head;
			thread_ready_queue->head = next_thread->next;
			thread_kill_queue->head = next_thread;

		}
		else{
			while(next_thread!=NULL && next_thread->id != current_thread){
				prev_thread = next_thread;	
				next_thread= next_thread->next;
			}
			prev_thread->next = next_thread->next;
			thread_kill_queue->head = next_thread;
		}

		next_thread->next=NULL;
	        thread_list[current_thread].thread_state = DEATHROW;	
		thread_yield(THREAD_ANY);
	}
	
	return THREAD_FAILED;
}

////////////////////////////////////////////////////////////////////////////
Tid
thread_kill(Tid tid)
{
	
	if(thread_ready_queue->head ==  NULL || tid == THREAD_SELF || current_thread == tid ||used_thread_list[tid] == 0 || !(tid>=0 && tid <= THREAD_MAX_THREADS-1)){
		return THREAD_INVALID;
	}
	struct thread_identifier *temp_thread;
	struct thread_identifier *next_thread;	
	temp_thread =thread_ready_queue->head; 	
	next_thread = temp_thread->next;
	while(next_thread!=NULL ){ //this loop is going to be used to find the thread with the corresponding Tid
		if(next_thread->id == tid){
			break;
		}
		temp_thread = next_thread;
		next_thread = next_thread->next;
	}
	if(next_thread == NULL){
		return THREAD_INVALID;// this is fine , tid was not found among list
	}
	else {
		thread_list[tid].thread_state = TO_BE_KILLED;
	        return tid;	
	}

	return THREAD_FAILED;
}
///////////////////////////////////////////////////////////////////////////
/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* make sure to fill the wait_queue structure defined above */
struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	TBD();

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	TBD();
	free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
	TBD();
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
	TBD();
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	TBD();
	return 0;
}

struct lock {
	/* ... Fill this in ... */
};

struct lock *
lock_create()
{
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);

	TBD();

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	assert(lock != NULL);

	TBD();

	free(lock);
}

void
lock_acquire(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

void
lock_release(struct lock *lock)
{
	assert(lock != NULL);

	TBD();
}

struct cv {
	/* ... Fill this in ... */
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	TBD();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	TBD();

	free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);

	TBD();
}
