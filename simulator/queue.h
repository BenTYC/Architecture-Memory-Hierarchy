#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifndef QUEUE_H_
#define QUEUE_H_

struct listnode {
	int data;
	struct listnode *next;
};
typedef struct listnode ListNode;

struct queue{
	ListNode *front;
	ListNode *rear;
};
typedef struct queue Queue;

void init(Queue *q){
	q->front = NULL;
	q->rear = NULL;
}

int empty(Queue *q){
	return (q->front == NULL);
}

void print_queue(Queue *q)
{
	if( empty(q) ){
		printf("queue is empty\n");
		return;
	}
	
	ListNode *current = q->front; 
	
	while(current != q->rear){
		printf("%d->", current->data);
		current = current->next;
	}
	printf("%d\n", current->data);
}

void enqueue(Queue *q, int data)
{
	ListNode *current = (ListNode *)malloc(sizeof(ListNode));
	assert(current != NULL);
	current->data = data;
	current->next = NULL;
	if( empty(q) ){
		q->front=current;
		q->rear=current;
	}else{
	    q->rear->next=current;
	    q->rear=current;
	}
}

int dequeue(Queue *q)
{
	ListNode *current;
	int n;
	if(empty(q)){
		return (-1);
	}else if(q->front == q->rear){
		current = q->front;
		q->front = NULL;
		q->rear = NULL;
		n = current->data;
	    free(current);
	    return n;
	}else{
		current = q->front;
	    q->front = q->front->next;
	    n = current->data;
	    free(current);
	    return n;
	}	
}

int pop_queue(Queue *q, int n)
{	
	if( empty(q) ){
		return -1;
	} 
	
	ListNode *current = q->front;
	ListNode *current_pre;
	
	if( n == q->front->data ){
		q->front = q->front->next;
		free(current);
		return n;
	}	
	
	while(current != q->rear){
		if(n == current->data){
			current_pre->next = current->next;
			free(current);
			return n;
		}		
		current_pre = current;
		current = current->next;
	}
	
	if(current == q->rear && n == q->rear->data){
		q->rear = current_pre;
		free(current);
		return n;	
	}
	
	return -1;	
}

#endif

