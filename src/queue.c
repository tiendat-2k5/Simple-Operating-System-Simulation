#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: put a new process to queue [q] */
        if(q==NULL || proc == NULL) return;
        
        if(q->size >= MAX_QUEUE_SIZE){
          printf("Queue is full, Cannot enqueue %d\n", proc->pid);
          return;
        }
        
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
    if(empty(q)) return NULL;
    
    // THAY VÌ tìm highest priority, lấy process THEO THỨ TỰ
    struct pcb_t *proc = q->proc[0];  // Lấy process ĐẦU TIÊN
    
    // Dịch tất cả process lên
    for(int i = 0; i < q->size - 1; i++){
        q->proc[i] = q->proc[i + 1];
    }
    q->size--;
    
    return proc;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */
         
        if(empty(q) || proc == NULL) return NULL;
        
        for(int i=0;i<q->size;i++){
          if (q->proc[i] == proc) {
            struct pcb_t *remove_proc = q->proc[i];
            for(int j=i;j<q->size - 1;j++){
              q->proc[j] = q->proc[j+1];
            }
            q->size--;
            q->proc[q->size] = NULL;
            return remove_proc;
            }
          }
        return NULL;
}
