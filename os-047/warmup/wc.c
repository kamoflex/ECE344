#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "wc.h"
#include <string.h>
#include <ctype.h>
#define MAX_SIZE 10000000 
struct node{
	int count;
	char *word;
	struct node *next;
};
struct wc {
	struct node *arr[MAX_SIZE];
	/* you can define this struct to have whatever fields you want. */
};

unsigned long 
hash (char *str){ //This code is djb2 from www.cse.yorku.ca/~oz/hash.html by Dan Bernstein
	unsigned long hash = 5381;
	int c;
	while((c=*str++)){
		hash = ((hash << 5) + hash) + c;
	}
	return hash;
}

struct wc *
wc_init(char *word_array, long size)
{
	struct wc *wc;

	wc = (struct wc *)malloc(sizeof(struct wc));
	assert(wc);
	long i =0;
	long counter =0;
	int word_found =0;
	//unsigned index =0;
	//unsigned word_size=0;


	while(i < size){
		//if(i==size){
		//	break;
		//}//
		while(isspace(word_array[i])!=0){//while char is whitespace
			i++;
		}
		
		while(isspace(word_array[i+counter])==0 && word_array[i+counter]!='\0'){//once you find a word it looks for the next backspace or EOS
			counter++;
		}
		if(word_array[i]=='\0'){
			break;
		}
		//word]_size = i + counter;//needed for when using malloc
		char *wordd=(char* ) malloc((counter+1)*sizeof(char));  
		strncpy(wordd, word_array + i, counter);	
		wordd[counter] = '\0';
		unsigned long index = hash(wordd);
		index = index%MAX_SIZE; //DECIDE TO KEEP OR NH
		if(wc->arr[index] == NULL){
			struct node *new_node = (struct node*) malloc(sizeof(struct node));
			new_node->word = wordd;//
			new_node->count=1;
			new_node->next =NULL;
			wc->arr[index] = new_node;
		}	
		else{
			struct node *current=wc->arr[index];
			struct node *prev=current;
			if(current!=NULL){
				//temp = current->next;
				while(current!=NULL){
					prev=current;
					if(strcmp(current->word,wordd)==0){
						current->count=current->count+1;
						word_found =1;
						free(wordd);//desperate mode only
						break;
					}	
					
				
					current=current->next;
					//temp=current->next;

				}
			}
			if(word_found ==0){
				struct node *new_node= (struct node*) malloc(sizeof(struct node));
				new_node->word = wordd;
				new_node->count=1;
				new_node->next =NULL;
				prev->next=new_node;
			}
			else if(word_found==1){
				word_found =0;
				//leave blank for now i guess??
			}
		}

		i=i+counter;//starts the counter at the end of the newly discovered word
		counter =0;
		word_found = 0;

	}

	return wc;
}

void
wc_output(struct wc *wc)
{
	struct node *current;
        //struct node *next;	
	for(unsigned i =0;i<MAX_SIZE;i++){
		current = wc->arr[i];
		while(current!=NULL){
			//next = current->next
			//;printf("index-%u \t %s:%d\n",i,current->word,current->count);

			printf("%s:%d\n",current->word,current->count);
			current = current->next;
		}
		
	}

	
}

void
wc_destroy(struct wc *wc)
{
	struct node *current;
        struct node *next;	
	for(unsigned i =0;i<MAX_SIZE;i++){
		current = wc->arr[i];
		while(current!=NULL){
			next = current->next;
			free(current);
			current = next;
		}
		//free(wc[i]) check whether you need this or not
	}
	free(wc);
}
