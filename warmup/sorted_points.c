#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"
#include <math.h>

/* this structure should store all the points in a list in sorted order. */
struct sorted_points {
	/* you can define this struct to have whatever fields you want. */
	struct individual_points* head; 
};

struct individual_points {
	double x;
	double y;
	double distance;
	struct individual_points* next; 

};

/* think about where you are going to store a pointer to the next element of the
 * linked list? if needed, you may define other structures. */

struct sorted_points *
sp_init()
{
	struct sorted_points *sp;

	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);

	sp->head = NULL; 

	return sp;
}

void//DISCOVER THE PROBLEM AND FIX IT
sp_destroy(struct sorted_points *sp)
{
	struct individual_points *current=sp->head;
	struct individual_points *next;
	while(current!=NULL){
		next = current->next;
		free(current);
		current = next;
	
	}
	free(sp);
	return;
}

int//COMPLETED 
sp_add_point(struct sorted_points *sp, double x, double y)
{
	struct individual_points* current; 
	struct individual_points* temp;
	struct  individual_points* node = (struct individual_points *)malloc(sizeof(struct individual_points));
	if (node == NULL){
		return 0;
	}
	else{
		node->x = x;
		node->y = y;
		node->distance = sqrt(((node->x)*(node->x))+((node->y)*(node->y)));
		if(sp->head == NULL){ //it is an empty linked list
			sp->head= node;
			node->next = NULL;
			return 1;
		}
		else if(sp->head!=NULL){ //not empty list
			current = sp->head; //where the current counter begins
			//*****************************************FOR ONLY ONR NODE PRESENT (HEAD) *****************************************
			if(current->next==NULL && current->distance < node->distance){ //if theres only head but node is larger than head
				current->next =node;
				node->next=NULL;	
				return 1;
			}
			else if(current->next==NULL && current->distance > node->distance){//if there is only head but node is smallern CHECK THIS CASE **
			//	temp = current;
				node->next = current;
			   	current->next=NULL;
				sp->head=node;	
				return 1;
			}
			else if(current->next==NULL && current->distance == node->distance){//if there is only one node but equal distance to second one
				if(current->x > node->x){//if theres a difference in x
					temp = current;
					sp->head =node;
					node->next= temp;
					current->next = NULL;
					return 1;
				}
				else if(current->x < node->x){
					current->next = node;
					node->next = NULL;
					return 1;					
				}
				else if(current->x == node->x){
					if(current->y > node->y){
						temp = current;
						sp->head = node;
						node->next= temp;
						current->next = NULL;
						return 1;
					}
					else if(current->y < node->y){
						current->next = node;
						node->next = NULL;	
						return 1;
					}
					else if(current->y == node->y){
					
		
						node->next= NULL;
						current->next =node;
						return 1;
					}
				}
			}
			//*************************************************** IF THERE EXISTS MORE THAN 2 ELEMENTSS ***********************	
			else{// if there are more than only 1 input
				if(current->distance>node->distance){
					sp->head = node;
					node->next = current;
					return 1;
				}
				while(current->next!= NULL &&(current->next->distance<node->distance || (current->next->distance == node->distance && current->next->x < node->x)||(current->next->distance == node->distance && current->next->x < node->x && current->next->y < node->y )  )){
					current = current->next;
				}
				node->next = current->next;
				current->next = node;
				return 1;
		//		while(current->next!=NULL){//TRY PUTTING ALL THE IF STATEMENTS IN THE LOOP HEADER AND STOP THE LOOP WHEN ITS TIME TO INSERT
		//			if(current->next->distance < node->distance){
		//				current=current->next;
		///			}
		//			else if(current->next->distance > node->distance){
		//				temp = current->next;
		//				current->next = node;
		//				node->next= temp;
		//				return 1;
		//			}
		//			else if(current->next->distance == node->distance){
		//				if(current->next->x > node->x){
		//					temp = current->next;
		//					current->next = node;
		//					node->next= temp;
		//					return 1;
		//				}
		//				else if(current->next->x < node->x){
		//					current=current->next;					
		//				}
		//				else if(current->next->x == node->x){
		//					if(current->next->y > node->y){
		//						temp = current->next;
		//						current->next = node;
		//						node->next= temp;
		//						return 1;
		//					}
		//					else if(current->next->y < node->y){
		//						current=current->next;
		//					}
		//					else if(current->next->y == node->y){
		//						temp = current->next;
		//						current->next = node;
		//						node->next= temp;
		//						return 1;
		//					}
		//				}
		//			}
		//		}
			}
	

		
		}
		return 1;
	}

}

int//COMPLETED
sp_remove_first(struct sorted_points *sp, struct point *ret)
{
	if(sp->head == NULL){
		return 0;
	}
	else{
		struct individual_points* temp = sp->head->next;
		ret->x = sp->head->x;
		ret->y = sp->head->y;
		free(sp->head);
		sp->head = temp;
	return 1;
	}

}

int//COMPLETED
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	if(sp->head == NULL){
		return 0;
	}
	else{
		if(sp->head->next == NULL){
			ret->x = sp->head->x;
			ret->y = sp->head->y;
			free(sp->head);
			sp->head = NULL;
			return 1; 			
		}
		struct individual_points* current = sp->head;
		struct individual_points* temp; 	
		while(current->next!=NULL){
			temp = current;
			current = current->next;			
		}
		ret->x = temp->next->x;
		ret->y = temp->next->y; 
		free(temp->next);
		temp->next=NULL;
		return 1;
	}
}

int//COMPLETED
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	struct individual_points* current=sp->head;
	struct individual_points* temp;
	if (index<0){
		return 0;
	}    
	else if(index==0){
		if(current ==  NULL){
			return 0;
		}
		else {
			temp = current->next;
			ret->x=current->x;
			ret->y=current->y;
			free(current);
			sp->head=temp;
			return 1;
		}
	}
	else if(index > 0){	
		if(current == NULL){
			return 0;		
		}
		else{
			for(unsigned i =0; i<index ;i++){
				if(current==NULL||current->next==NULL){
					return 0;
				}
				else{//finish this
					temp=current;
					current = current->next;
				}
			}
			ret->x = current->x;
			ret->y = current->y;
			temp->next = current->next;
			free(current);
			return 1;
		}	
	}
	return 1;	
}

int//COMPLETED
sp_delete_duplicates(struct sorted_points *sp)
{
	int duplicates=0;
	struct individual_points* current=sp->head;
	struct individual_points* temp = current->next;
	if(current == NULL || current->next==NULL){
		return 0;
	} 
	else{
		while(current->next!=NULL){
			temp = current->next;
			if(current->x == temp->x && current->y == temp->y){
				duplicates++;
				current->next = temp->next;
				free(temp);
			//	current=current->next;
			}
			else{
				current=current->next;
			}
		}		
	}
	return duplicates;
}









