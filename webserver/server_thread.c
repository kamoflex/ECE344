#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>
#include <string.h> 
#include <stdbool.h>
#define MAX_SIZE 10000
#define NOT_FOUND 1000000

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;

};

struct lru_node {
	char* file_name;
	struct lru_node *next;

};
struct cache_node {
	struct file_data *file;
	struct cache_node *next;
	int num_of_readers;
	unsigned long index;
};

/* static functions */
pthread_cond_t cv_nfull;
pthread_cond_t cv_nempty;
pthread_mutex_t lock;
int in, out;
pthread_t *thread_array;
int *requests_array;
/*					Lab 5	variables			 	*/
int current_cache_size;
pthread_mutex_t cache_lock;
//struct lru_node *lru_head = NULL; //HEAD SHOULD NEVER POINT TO NULL UNLESS CACHE IS EMPTY
//struct lru_node *lru_tail = NULL; //tail should never point to NULL unless CACHE IS EMPTY 
struct cache_node cache_hash[MAX_SIZE];
int cache_index[MAX_SIZE] ={0};


///////////////////global variables
//hashing function///////////////////////////////////////////////////////////////////////////
unsigned long 
hash (char *str){ //This code is djb2 from www.cse.yorku.ca/~oz/hash.html by Dan Bernstein
	unsigned long hash = 5381;
	int c;
	while((c=*str++)){
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}
/////////////////////////////////////////////////////////////////////////////////////////////

struct cache_node *cache_lookup(struct server *sv, char *file_name);
struct cache_node *cache_insert(struct server *sv, struct file_data *data);
bool cache_evict(struct server *sv,int space_needed);
/* initialize file data */


static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

static void
do_server_request(struct server *sv, int connfd)
{
	int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();

	/* fill data->file_name with name of the file being requested */
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
	/* read file, 
	 * fills data->file_buf with the file contents,
	 * data->file_size with file size. */

	struct cache_node* returned_cache;
	pthread_mutex_lock(&cache_lock);
	returned_cache = cache_lookup(sv, data->file_name);//checks if file was cached or not
	pthread_mutex_unlock(&cache_lock);
	if(returned_cache != NULL){
		request_set_data(rq, returned_cache->file); //check if its the file or the collided file
	}
	else if(returned_cache == NULL){
		//THIS IS CALLED ONLY IF IT WAS NOT FOUND IN CACHE
		ret = request_readfile(rq);

		if (ret == 0) { /* couldn't read file */
			goto out;
		}
		pthread_mutex_lock(&cache_lock);
		returned_cache = cache_lookup(sv, data->file_name);
		if(returned_cache== NULL){
			returned_cache = cache_insert(sv, data);//returned_cache has a chance to get updated to something other than NULL, if successful insert
		}
		pthread_mutex_unlock(&cache_lock);
	}

	/* send file to client */
	pthread_mutex_lock(&cache_lock);//HERE WE NEED A LOCK TO INCREMENT THE NUMBER OF READERS if file was cached	
	if(returned_cache != NULL){ 	
		returned_cache->num_of_readers++;
	}
	pthread_mutex_unlock(&cache_lock);
		
	request_sendfile(rq); //sends file to client

	pthread_mutex_lock(&cache_lock); //HERE WE NEED A LOCK TO DECREMENT THE NUMBER OF READERS if file was cached	
	if(returned_cache != NULL){
		returned_cache->num_of_readers--;
	}
	pthread_mutex_unlock(&cache_lock);

out:
	request_destroy(rq);
	//file_data_free(data);
}

/* entry point functions */
void stub(struct server *sv){
	
	while(!sv->exiting){
		pthread_mutex_lock(&lock);
		while(in == out) {
			pthread_cond_wait(&cv_nempty , &lock);
			if(sv->exiting){
				pthread_mutex_unlock(&lock);
				return;
			}

		}
		int connfd = requests_array[out];
		if((in - out + sv->max_requests) % sv->max_requests == sv->max_requests - 1){
			pthread_cond_signal(&cv_nfull);
		}
		out = (out + 1) % sv->max_requests; 
		pthread_mutex_unlock(&lock);
		do_server_request(sv, connfd);

	}
}

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	//create LRU list
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	max_requests++;
	in =0;
	out =0;
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;
	sv->exiting = 0;
	pthread_mutex_init(&lock, NULL);
	pthread_mutex_init(&cache_lock, NULL);
	pthread_cond_init(&cv_nfull, NULL);
	pthread_cond_init(&cv_nempty, NULL);
	
	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
		if(max_requests <= 0){
			requests_array = NULL;
		}
		else{
			requests_array = (int*)malloc(max_requests * sizeof(int));
		}
		if(nr_threads <= 0){
			thread_array = NULL;
		}
		else{
			thread_array = (pthread_t*)malloc(nr_threads * sizeof(pthread_t));
			for(unsigned long i = 0; i< nr_threads ; i ++){
				pthread_create(&thread_array[i], NULL, (void *)&stub, sv);
			}
		}


		//might need to consider corner case where if max_requests  = 1;



	}

	return sv;
}

void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {

		pthread_mutex_lock(&lock);
		while((in - out + sv->max_requests) % sv->max_requests == sv->max_requests - 1){
			pthread_cond_wait(&cv_nfull, &lock);
		}
		requests_array[in] = connfd;
		if(in ==  out){
			pthread_cond_broadcast(&cv_nempty);
		}
		in = (in + 1)%sv->max_requests;
		pthread_mutex_unlock(&lock);
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
	}
}

struct cache_node *cache_lookup(struct server *sv, char *file_name){
	unsigned long index = hash(file_name);
	index = index % MAX_SIZE;
	if (cache_index[index] >  0){
		struct cache_node* cache_ptr = &cache_hash[index];
		while(cache_ptr != NULL){
			if(strcmp(cache_ptr->file->file_name, file_name)==0){
//					printf("lookup : found\n");
					return cache_ptr;
			}
			cache_ptr = cache_ptr->next;
		}

		return NULL;
	}
	else if(cache_index[index] == 0){//NEED TO ADD STUFF HERE (this is the case where the file was not cached)
		return NULL;
	}
	else  {
		printf("we shouldn't be coming here\n");

	}//this happens if the number of elements in that slot in the cache is negative
	

	return NULL; 

}

struct cache_node *cache_insert(struct server *sv, struct file_data *data){
	unsigned long index = hash(data->file_name);
	index = index % MAX_SIZE;
	if((sv->max_cache_size - current_cache_size) >= data->file_size){//we have space in the cache

		if(cache_index[index] == 0){
	//		printf("insert : no collisions : there is space\n");
			struct cache_node* cache_ptr = &cache_hash[index];
			cache_index[index]++;//decrement when evicting a node
			cache_hash[index].file = file_data_init();
			strcpy(cache_hash[index].file->file_name, data->file_name);
			cache_hash[index].file->file_size = data->file_size;
			cache_hash[index].file->file_buf = data->file_buf;
			cache_hash[index].next = NULL;
			cache_hash[index].num_of_readers = 0;
			cache_hash[index].index = index;
			current_cache_size = current_cache_size + data->file_size;
			return cache_ptr;
		}
		else if(cache_index[index] > 0){ //this is the case if there is a collision upon insertion
	//		printf("insert : collisions : there is space\n");
			struct cache_node* cache_ptr = &cache_hash[index];
			struct cache_node* cache_prev_ptr = cache_ptr;
			while(cache_ptr != NULL){//iterate through nodes stored in hash slot 
				cache_prev_ptr = cache_ptr;
				cache_ptr = cache_ptr->next;
			}	
			struct cache_node *cache_object = (struct cache_node*) malloc(sizeof(struct cache_node));
			cache_index[index]++;//decrement when evicting a node
			cache_ptr = cache_object;
			cache_prev_ptr->next = cache_ptr;
			cache_ptr->file = file_data_init();
			strcpy(cache_ptr->file->file_name, data->file_name);
			cache_ptr->file->file_size = data->file_size;
			cache_ptr->file->file_buf = data->file_buf;
			cache_ptr->next = NULL;
			cache_ptr->num_of_readers = 0;
			cache_ptr->index = index;
			current_cache_size = current_cache_size + data->file_size;
			return cache_ptr;
		}
		else{
			printf("again, we shouldn't be coming here (insert)\n");
		}
		//INCREMENT CURRENT_CACHE_SIZE by file->file_size
	}else{
		return NULL;
		}
	

	return NULL;
}

bool cache_evict(struct server *sv, int space_needed){
	//consider the case when all the cached files are in use (read from disk) also free the LRU node of deleted file
	//int potential_memory = 0;	//decrement the number of stored data in a single hash slot in cache_index[] using the index in the cache_node struct  
	//also decrement current_cache_size when removing a file
	//struct lru_node *lru_iterator = lru_head;
	while((sv->max_cache_size - current_cache_size) < space_needed){

	}
	return false;
}

void
server_exit(struct server *sv)
{
	/* when using one or more worker threads, use sv->exiting to indicate to
	 * these threads that the server is exiting. make sure to call
	 * pthread_join in this function so that the main server thread waits
	 * for all the worker threads to exit before exiting. */
	//in the while loop in the producer, after the sleeping, check if the exiting is on, unlock and call pthread_exit
	sv->exiting = 1;
	pthread_cond_broadcast(&cv_nfull);
	pthread_cond_broadcast(&cv_nempty);
	for(unsigned long i = 0; i < sv->nr_threads ; i++){
		pthread_join(thread_array[i], NULL);

	}

	/* make sure to free any allocated resources */
	free(requests_array);
	free(thread_array);
	free(sv);
}
