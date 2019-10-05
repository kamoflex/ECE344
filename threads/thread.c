#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"

#define RUNNING 0
#define READY 1
#define TO_BE_KILLED 2
#define DEATHROW 3 
#define DEAD 4
#define BLOCKED 5
/* This is the wait queue structure */

struct thread_identifier{
	Tid id;
        struct thread_identifier* next;	
};
struct wait_queue {
	int counter;
	struct thread_identifier* head;
};
struct thread_queue {
	
	int counter;
	struct thread_identifier* head; 
};
/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
	Tid thread_id;
	ucontext_t thread_context;
	int thread_state;
	void* stack_head;
	struct wait_queue* waitq;
	//pointer to first byte in stack
	struct thread* next_thread; 
};





//GLOBAL VARIABLES
/********************************************************************************************************/
int used_thread_list[THREAD_MAX_THREADS]={0}; //This holds binary values for whether a thread is in use or nah
struct thread thread_list[THREAD_MAX_THREADS];
 Tid current_thread;// = (struct thread *)malloc(sizeof(struct thread));
struct thread_queue *thread_ready_queue;
struct thread_queue *thread_kill_queue;
int created_threads=1;
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
	thread_list[current_thread].waitq = wait_queue_create();
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
	interrupts_on();

	thread_main(arg); // call thread_main() function with arg
	ret = thread_exit();
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
	// all threads are done, so process should exit
	free(thread_ready_queue->head);
	free(thread_kill_queue->head);
	free(thread_list[current_thread].waitq);
	free(thread_ready_queue);
	free(thread_kill_queue);
	//free(thread_list[current_thread].stack_head);
	//free(thread_list[current_thread].stack_head);
	for(int i = 0; i<THREAD_MAX_THREADS;i++){
		used_thread_list[i] =0;
		free(thread_list[i].stack_head);

	}
	exit(0);
}

///////////////////////////////////////////////////////////////////////////

Tid
thread_create(void (*fn) (void *), void *parg)
{
	int enabled = interrupts_off();
	created_threads++;
	//REMOVE ABOVE LINE LATER;	
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
		interrupts_set(enabled);
		return THREAD_NOMORE;
	}else if(found_thread_vacancy == 1 ){
		
		thread_list[new_thread_id].thread_id = new_thread_id;
		used_thread_list[new_thread_id] = 1;	//indicates that the thread in the list is now being used
		thread_list[new_thread_id].next_thread = NULL;
		thread_list[new_thread_id].thread_state = READY;
		thread_list[new_thread_id].waitq = wait_queue_create();
		int ret = getcontext(&thread_list[new_thread_id].thread_context);
		assert(!ret);
	//STACK STUFF
		void* stack = (void *)malloc (THREAD_MIN_STACK);
		if(stack == NULL){
			interrupts_set(enabled);
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
			interrupts_set(enabled);
			return new_thread_id;
		}
		else{
			struct thread_identifier *temp_thread = thread_ready_queue->head;
			while(temp_thread->next != NULL){ //iterate until last node in ready_queue
				temp_thread = temp_thread->next;
			}
			temp_thread->next = new_thread;//set the last element as current thread
			thread_ready_queue->counter++;	
			interrupts_set(enabled);
			return new_thread_id;
		}
	}
	interrupts_set(enabled);
	return THREAD_NOMEMORY;


}

///////////////////////////////////////////////////////////////////////////

Tid
thread_yield(Tid want_tid)
{//clean up kill queue in here as well as kill function
	int enabled = interrupts_off();
	//printf("Current state of thread %d: %d\n", current_thread,thread_list[current_thread].thread_state );


	if(want_tid == THREAD_SELF || want_tid == current_thread){//run itself
//		sought_id = caller_id;
		 int setcontext_called;

		setcontext_called = 0;
		int ret = getcontext(&thread_list[current_thread].thread_context);
		assert(!ret);
		if(setcontext_called == 1){//this condition is needed for the second return from getconstext()
			
			interrupts_set(enabled);
			return current_thread;
		}
		setcontext_called =1;
		ret = setcontext(&thread_list[current_thread].thread_context);
		assert(!ret);
		
	}
	else if(want_tid == THREAD_ANY && thread_ready_queue->head == NULL){
	//	printf("We get here 1\n");
		interrupts_set(enabled);
		return THREAD_NONE;
	}
	else if((want_tid == THREAD_ANY && thread_ready_queue->head != NULL ) || ( thread_ready_queue->head!=NULL && (want_tid >= 0 && want_tid <= THREAD_MAX_THREADS - 1) && want_tid == thread_ready_queue->head->id)){//run head
		int running_thread = current_thread;
		int revived_thread;
		//int new_t;		
		if(thread_list[current_thread].thread_state == RUNNING){
			thread_list[current_thread].thread_state = READY;
		}
		if(thread_list[current_thread].thread_state == TO_BE_KILLED){//////COME BACK HERE
			thread_exit();
		}
/*			 REMOVE NODE			*/
		struct thread_identifier *rejuvenated_thread= thread_ready_queue->head;//extracted next runner
		thread_ready_queue->head = thread_ready_queue->head->next; //updating the head to next node
		rejuvenated_thread->next = NULL;
		revived_thread = rejuvenated_thread->id;
		free(rejuvenated_thread);
/*			 INSERT NODE			*/
		if(thread_list[current_thread].thread_state != DEATHROW /*&& thread_list[current_thread].thread_state != BLOCKED*/){
			//insert into ready queue
			struct thread_identifier *inserted_thread= (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
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
		}

		current_thread =revived_thread;
		//new_t = rejuvenated_thread->id;
		if(thread_list[revived_thread].thread_state == READY){
			thread_list[revived_thread].thread_state = RUNNING;
		}
		int setcontext_called=0;
		int ret = getcontext(&thread_list[running_thread].thread_context);//CHECK [INSERTED_THREAD->ID]??
		assert(!ret);

		while(thread_kill_queue->head!= NULL){
			struct thread_identifier *deleted_thread = thread_kill_queue->head;
			thread_kill_queue->head = thread_kill_queue->head->next;
			thread_list[deleted_thread->id].thread_state = DEAD;

			used_thread_list[deleted_thread->id] = 0;
			free(thread_list[deleted_thread->id].stack_head);
			free(deleted_thread);
		}	
		if(setcontext_called == 1){	

			interrupts_set(enabled);
			return (revived_thread);
		}
		setcontext_called = 1;
		ret = setcontext(&thread_list[revived_thread].thread_context);
		assert(!ret);
	}
	else if(want_tid >= 0 && want_tid <= THREAD_MAX_THREADS - 1){//RUN Tid

		int revived_thread;

		
		if(thread_ready_queue->head == NULL){//fine
			interrupts_set(enabled);
			return THREAD_INVALID;
		}
		if(used_thread_list[want_tid] == 0){//fine
			interrupts_set(enabled);
			return THREAD_INVALID;
		}

		struct thread_identifier *temp_thread;
		struct thread_identifier *next_thread;	
		temp_thread =thread_ready_queue->head; 	
		next_thread = temp_thread->next;
		while(next_thread!=NULL && next_thread->id != want_tid){ //this loop is going to be used to find the thread with the corresponding Tid
		//	if(next_thread->id == want_tid){ /*		THE NODES HERE NEED TO BE FREEEEEED			*/
		//		break;
		///	
		//}	
			temp_thread = next_thread;
			next_thread = next_thread->next;
		}
		if(next_thread == NULL){

			interrupts_set(enabled);

			return THREAD_INVALID;// this is fine
		}
		else{


			struct thread_identifier *rejuvenated_thread;
			struct thread_identifier *temp = thread_ready_queue->head;
			rejuvenated_thread = next_thread;
			temp_thread->next = next_thread->next;
			rejuvenated_thread->next = NULL;
			revived_thread = rejuvenated_thread->id;
			free(rejuvenated_thread);
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
			thread_list[revived_thread].thread_state = RUNNING;
			current_thread = revived_thread;
			int setcontext_called = 0;
			int ret = getcontext(&thread_list[inserted_thread->id].thread_context);
			assert(!ret);
			if(setcontext_called == 1){	
				interrupts_set(enabled);
				return (revived_thread);
			}
			setcontext_called = 1;
			ret = setcontext(&thread_list[revived_thread].thread_context);
			assert(!ret);
			//free(rejuvenated_thread);
		}
	}

	else{
		interrupts_set(enabled);

		return THREAD_INVALID;

	}
		interrupts_set(enabled);
	return THREAD_FAILED;
}

////////////////////////////////////////////////////////////////////////////

Tid
thread_exit()
{
	int enabled = interrupts_off();
	/* 			THIS IF THE WAIT STUFF			 */
	if(thread_list[current_thread].waitq->head != NULL){
		thread_wakeup(thread_list[current_thread].waitq, 1);
		wait_queue_destroy(thread_list[current_thread].waitq);

	}
	/*			END OF WAIT STUFF			*/
	if(thread_ready_queue->head == NULL){
		//printf("we get here before end\n");
//		thread_list[current_thread].thread_state = RUNNING;

		interrupts_set(enabled);
		return THREAD_NONE;
	}

	if(thread_kill_queue->head == NULL){
		struct thread_identifier *next_thread = (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
		next_thread->id = current_thread;
		next_thread->next=NULL;
		thread_kill_queue->head = next_thread;
	   	thread_list[current_thread].thread_state = DEATHROW;
		//REMOVE FROM THREAD_READY_QUEUE	
		thread_yield(THREAD_ANY);
		interrupts_set(enabled);
		return current_thread;

	}
	else{
		printf("Kill queue isnt empty, implement this");
	}
	
	return THREAD_FAILED;
}

////////////////////////////////////////////////////////////////////////////
Tid
thread_kill(Tid tid)
{
	int enabled = interrupts_off();
	if(current_thread == tid ||used_thread_list[tid] == 0 || !(tid>=0 && tid <= THREAD_MAX_THREADS-1)){
		interrupts_set(enabled);
		return THREAD_INVALID;
	}
//	if(thread_list[current_thread].waitq->head != NULL &&  thread_list[current_thread].waitq->head->id == tid ){

//		thread_wakeup(thread_list[current_thread].waitq, 1);
//		thread_list[tid].thread_state = TO_BE_KILLED;
//		interrupts_set(enabled);
//	      	return tid;	
//	}
	struct thread_identifier *temp_thread;
	temp_thread =thread_ready_queue->head; 	
	struct thread_identifier *next_thread;	
	next_thread = temp_thread->next;
	if(temp_thread->id == tid){
		thread_list[tid].thread_state = TO_BE_KILLED;
		interrupts_set(enabled);
		return tid;
	}
	else{
		while(next_thread!=NULL ){ //this loop is going to be used to find the thread with the corresponding Tid
			if(next_thread->id == tid){
				break;
			}
			temp_thread = next_thread;
			next_thread = next_thread->next;
		}
		if(next_thread == NULL){
			interrupts_set(enabled);
			return THREAD_INVALID;// this is fine , tid was not found among list
		}
		else {
			//temp_thread->next = next_thread->next;
			//free(next_thread);
			thread_list[tid].thread_state = TO_BE_KILLED;
			interrupts_set(enabled);
	      	 	 return tid;	
		}
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
	wq->head = NULL;
	wq->counter = 0;
	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{

	thread_wakeup(wq, 1);
	free(wq);
	return;
}

Tid
thread_sleep(struct wait_queue *queue)//put running thread in wait queue and run a  thread from ready queue  
{
	int enabled = interrupts_off();
	if(queue == NULL){
		interrupts_set(enabled);//if ther are no other threads to run
		return THREAD_INVALID;
	}
	if(thread_ready_queue->head == NULL){//if only the caller is available
		interrupts_set(enabled);
		return THREAD_NONE;
	}
	int running_thread = current_thread;
	int revived_thread;
	struct	thread_identifier* rejuvenated_thread = thread_ready_queue->head;
	revived_thread = rejuvenated_thread->id;	
	thread_ready_queue->head = thread_ready_queue->head->next; //updating the head to next node
	free(rejuvenated_thread);
	if(thread_list[current_thread].thread_state != DEATHROW /*&& thread_list[current_thread].thread_state != BLOCKED*/){
		//insert into ready queue
		struct thread_identifier *inserted_thread= (struct thread_identifier *)malloc(sizeof(struct thread_identifier));
		inserted_thread->id = current_thread;
 		inserted_thread->next = NULL; //set it last node's pointer to NULL (old current)
 		//thread_list[current_thread].thread_state = READY;
 		/*												LEAVE ABOVE LINE COMMMENTED FOR NOW														*/
		if(queue->head == NULL){
			queue->head = inserted_thread; 
		}
		else{
			struct thread_identifier *temp_thread = queue->head;
			while(temp_thread->next != NULL){ //iterate until last node in ready_queue
				temp_thread = temp_thread->next;
			}
			temp_thread->next = inserted_thread;
		}
	}
	current_thread =revived_thread;
	if(thread_list[revived_thread].thread_state == READY){
		thread_list[revived_thread].thread_state = RUNNING;
	}
	int setcontext_called=0;
	int ret = getcontext(&thread_list[running_thread].thread_context);//CHECK [INSERTED_THREAD->ID]??
	assert(!ret);

	while(thread_kill_queue->head!= NULL){
		struct thread_identifier *deleted_thread = thread_kill_queue->head;
		thread_kill_queue->head = thread_kill_queue->head->next;
		used_thread_list[deleted_thread->id] = 0;
		free(thread_list[deleted_thread->id].stack_head);
		thread_list[deleted_thread->id].thread_state = DEAD;
		free(deleted_thread);
	}	
	if(setcontext_called == 1){	
		interrupts_set(enabled);
		return (revived_thread);
	}
	setcontext_called = 1;
	ret = setcontext(&thread_list[revived_thread].thread_context);
	assert(!ret);




	interrupts_set(enabled);
	return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all) //FINISH IMPLEMETING IF YOU GOT TIME
{
	int enabled = interrupts_off();
	
	if(queue == NULL || queue->head == NULL){
		interrupts_set(enabled);
		return 0;
	}
	if(all == 0){
		//change state from BLOCKED to READY
		struct thread_identifier *switching_thread = queue->head;
		queue->head = switching_thread->next;
		switching_thread->next = NULL;
		thread_list[switching_thread->id].thread_state = READY;
		if(thread_ready_queue->head == NULL){
			thread_ready_queue->head = switching_thread;
			queue->counter = queue->counter - 1;
			interrupts_set(enabled);
			return 1;
		}
		else{//iterate until you reach end of thread_ready_queue and add thread there
			struct thread_identifier* temp_thread = thread_ready_queue->head;
			while(temp_thread->next != NULL){
				temp_thread = temp_thread->next;
			}
			temp_thread->next = switching_thread;
			queue->counter = queue->counter - 1;
			interrupts_set(enabled);
			return 1;
		}
	}
	else if(all == 1){ //wake up all threads, make a counter to count how many were waken up
		//CHANGE STATE OF ALL THREADS TO READY
//		int counter = queue->counter;
		int counter = 0;
		struct thread_identifier* next_thread = queue->head;
		while(next_thread != NULL){
			counter++;
			thread_list[next_thread->id].thread_state = READY;
			next_thread = next_thread->next;
		}
		if(thread_ready_queue->head == NULL){
			thread_ready_queue->head = queue->head;
			queue->counter = 0;
			queue->head = NULL;
			interrupts_set(enabled);
			return counter;
		}
		else{//iterate until you reach end of thread_ready_queue and add thread there
			struct thread_identifier* temp_thread = thread_ready_queue->head;
			while(temp_thread->next != NULL){
				temp_thread = temp_thread->next;
			}
			temp_thread->next = queue->head;
			queue->head = NULL;
			queue->counter = 0;
			interrupts_set(enabled);
			return counter;
		}
	}		
	interrupts_set(enabled);
	return 0;
}

/* suspend current thread until Thread tid exits */
Tid
thread_wait(Tid tid)
{
	int enabled = interrupts_off();
	//printf("tid: %d\n", tid);
	if(tid < 0 || tid >= THREAD_MAX_THREADS  || tid == current_thread || used_thread_list[tid] == 0){
		interrupts_set(enabled);
		return THREAD_INVALID; 
	}
	else{
		thread_sleep(thread_list[tid].waitq);
	}
	interrupts_set(enabled);
	return tid;
}

struct lock {
	struct thread* thread;
	struct wait_queue* waitq;
	int locked;
};

struct lock *
lock_create()
{
	int enabled = interrupts_off();
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);
	lock->thread = NULL;
	lock->waitq = wait_queue_create();
	lock->locked = 0;
	interrupts_set(enabled);

	return lock;
}

void
lock_destroy(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(lock != NULL);

	while(lock->locked == 1){
		thread_sleep(lock->waitq);
	}
	wait_queue_destroy(lock->waitq);
	free(lock);
	interrupts_set(enabled);
	return;
}

void
lock_acquire(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(lock != NULL);
	while(lock->locked == 1){
		thread_sleep(lock->waitq);
	}
	lock->thread = &thread_list[current_thread];
	lock->locked = 1;
	interrupts_set(enabled);
	return;	
}

void
lock_release(struct lock *lock)
{
	int enabled = interrupts_off();
	assert(lock != NULL);
	thread_wakeup(lock->waitq, 1);
	lock->thread = NULL;
	lock->locked = 0;
	interrupts_set(enabled);
	return;

}

struct cv {
	struct wait_queue* waitq;
};

struct cv *
cv_create()
{
	int enabled = interrupts_off();
	struct cv *cv;
	cv = malloc(sizeof(struct cv));
	assert(cv);
	cv->waitq = wait_queue_create();
	interrupts_set(enabled);
	return cv;
}

void
cv_destroy(struct cv *cv)
{
	int enabled = interrupts_off();
	assert(cv != NULL);
	wait_queue_destroy(cv->waitq);
	free(cv);
	interrupts_set(enabled);
	return;
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
	lock_release(lock);
	thread_sleep(cv->waitq);
	lock_acquire(lock);
	interrupts_set(enabled);
	return;
}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
	thread_wakeup(cv->waitq, 0);
	interrupts_set(enabled);
	return;
}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	int enabled = interrupts_off();
	assert(cv != NULL);
	assert(lock != NULL);
	thread_wakeup(cv->waitq, 1);
	interrupts_set(enabled);
	return;
}