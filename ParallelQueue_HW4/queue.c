#include "queue.h"
#include <stdlib.h>
#include <threads.h>

typedef struct Item{
    void* Item_data;
    struct Item* nextItem;
}Item;

typedef struct Queue
{
    struct Item* firstItem;
    struct Item* lastItem;
    size_t size_of_queue;
    mtx_t mutex;
    cnd_t cond;/*will use it to send signals when we insert items to the queue*/
    size_t num_of_waiting_threads;
    size_t num_of_passed_items;
}Queue;

struct Queue* q;

void initQueue(){
    q=malloc(sizeof(struct Queue));
    q->firstItem=NULL;
    q->lastItem=NULL;
    q->size_of_queue=0;
    q->num_of_passed_items=0;
    q->num_of_waiting_threads=0;
    mtx_init(&(q->mutex),mtx_plain);
    cnd_init(&(q->cond));
}

void destroyQueue(){
    /*first we lock the mtx to safely destroy the queue*/
    mtx_lock(&(q->mutex));

    /*then we free the items in the queue*/
    struct Item* temp1=q->firstItem;
    struct Item* temp2;
    while(temp1!=NULL){
        temp2=temp1->nextItem;
        free(temp1);
        temp1=temp2;
    }

    q->firstItem=NULL;
    q->lastItem=NULL;
    q->size_of_queue=0;
    q->num_of_passed_items=0;
    q->num_of_waiting_threads=0;
    /*we unlock the mtx before destroying it*/
    mtx_unlock(&(q->mutex));
    mtx_destroy(&(q->mutex));
    cnd_destroy(&(q->cond));
    /*at last we free the queue and nullify it,this is done at last because above 
    we needed to get to some attributes of the queue(mutex,cond...)  */
    free(q);
    q=NULL;
}

void enqueue(void* item_data){
    struct Item* item=malloc(sizeof(struct Item));
    item->Item_data=item_data;
    item->nextItem=NULL;

    /*now we lock the mtx to ensure safe(exclusive) usage of the queue*/
    mtx_lock(&(q->mutex));

    if(q->firstItem==NULL){
        q->firstItem=item;
        q->lastItem=item;
        q->size_of_queue=1;
    }
    else{
        q->lastItem->nextItem=item;
        q->lastItem=item;
        q->size_of_queue+=1;
    }
    /*signal to any waiting threads that a new item has been added to the queue*/
    cnd_signal(&(q->cond));

    mtx_unlock(&(q->mutex));
}

void* dequeue(){
    /* We lock the mtx to ensure safe(exclusive) usage of the queue*/
    mtx_lock(&(q->mutex));

    q->num_of_waiting_threads+=1;
    /*We were asked to block if the queue is empty, so we wait while the queue is empty*/
    while(q->size_of_queue==0){
        cnd_wait(&(q->cond),&(q->mutex));
    }
    q->num_of_waiting_threads-=1;
    /* if we got here then we are sure that there is at least one item in the queue,
    and to satisfy FIFO we should return the first item in the queue*/
    struct Item* item=q->firstItem;
    void* res=item->Item_data;
    q->firstItem=item->nextItem;

    q->size_of_queue-=1;
    q->num_of_passed_items+=1;
    if(q->size_of_queue==0){
        q->lastItem=NULL;
    }

    free(item);

    mtx_unlock(&(q->mutex));

    return res;

}

bool tryDequeue(void** x){
    mtx_lock(&(q->mutex));

    if(q->size_of_queue==0){
        mtx_unlock(&(q->mutex));
        return false;
    }

    struct Item* item=q->firstItem;
    *x=item->Item_data;
    q->firstItem=item->nextItem;

    q->size_of_queue-=1;
    q->num_of_passed_items+=1;
    if(q->size_of_queue==0){
        q->lastItem=NULL;
    }

    free(item);

    mtx_unlock(&(q->mutex));

    return true;
}

size_t size(){

    mtx_lock(&(q->mutex));
    size_t res=q->size_of_queue;
    mtx_unlock(&(q->mutex));
    return res;

}

size_t waiting(){
    return q->num_of_waiting_threads;
}
size_t visited(){
    return q->num_of_passed_items;
}


