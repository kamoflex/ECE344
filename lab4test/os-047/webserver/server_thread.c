#include "request.h"
#include "server_thread.h"
#include "common.h"
#include <pthread.h>

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	int exiting;

};

/* static functions */
pthread_cond_t cv_nfull;
pthread_cond_t cv_nempty;
pthread_mutex_t lock;
int in, out;
pthread_t *thread_array;
int *requests_array;
//global variables
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
	ret = request_readfile(rq);
	if (ret == 0) { /* couldn't read file */
		goto out;
	}
	/* send file to client */
	request_sendfile(rq);
out:
	request_destroy(rq);
	file_data_free(data);
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

	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

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
