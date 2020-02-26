#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sbuffer.h"
#include <pthread.h>

pthread_mutex_t local_mutex=PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t barrier;
/*
 *
 * All data that can be stored in the sbuffer should be encapsulated in a
 * structure, this structure can then also hold extra info needed for your implementation
 */
int count=0;    //count counts for the number of threads arrived the barrier

struct sbuffer_data {
    sensor_data_t data;
};

typedef struct sbuffer_node {
    struct sbuffer_node * next;
    sbuffer_data_t element;
} sbuffer_node_t;

struct sbuffer {
    sbuffer_node_t * head;
    sbuffer_node_t * tail;
};



int sbuffer_init(sbuffer_t ** buffer)
{
    ///////////////////
  *buffer = malloc(sizeof(sbuffer_t));
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;
    pthread_barrier_init(&barrier, NULL, 2);
  return SBUFFER_SUCCESS; 
}


int sbuffer_free(sbuffer_t ** buffer)
{
    sbuffer_node_t * dummy;
  if ((buffer==NULL) || (*buffer==NULL)) 
  {
    return SBUFFER_FAILURE;
  } 
  while ( (*buffer)->head )
  {
    dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;
   pthread_mutex_destroy(&local_mutex);
   pthread_barrier_destroy(&barrier);
  return SBUFFER_SUCCESS;		
}


int sbuffer_remove(sbuffer_t * buffer,sensor_data_t * data)
{
    sbuffer_node_t * dummy;
    pthread_barrier_wait(&barrier);     //barrier


    if(pthread_mutex_lock(&local_mutex)!=0)  //mutex
    {
        perror("pthread_mutex_lock\n");
        exit(EXIT_FAILURE);
    }



  if (buffer == NULL)
  {
      pthread_mutex_unlock(&local_mutex);
      return SBUFFER_FAILURE;
  }

  if (buffer->head == NULL)
  {
      pthread_mutex_unlock(&local_mutex);
      return SBUFFER_NO_DATA;
  }


    *data = buffer->head->element.data;   //if only the faster thread arrived, only read it

    dummy = buffer->head;
    count++;
    if(count==2) {                      //if slower thread arrived the barrier,delete the head,reset the count

      if (buffer->head == buffer->tail) // buffer has only one node
      {
          buffer->head = buffer->tail = NULL;
      } else                            // buffer has many nodes empty
      {
          buffer->head = buffer->head->next;
      }
        count=0;
      free(dummy);
  }


    if(pthread_mutex_unlock(&local_mutex)!=0)
    {
        perror("pthread_mutex_unlock\n");
        exit(EXIT_FAILURE);
    }
    return SBUFFER_SUCCESS;
}


int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data)
{
    sbuffer_node_t * dummy;
    if(pthread_mutex_lock(&local_mutex)!=0)
    {
        perror("pthread_mutex_lock\n");
        exit(EXIT_FAILURE);
    }

  if (buffer == NULL)
  {
      pthread_mutex_unlock(&local_mutex);
      return SBUFFER_FAILURE;
  }
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL)
  {
      pthread_mutex_unlock(&local_mutex);
      return SBUFFER_FAILURE;
  }
  dummy->element.data = *data;
  dummy->next = NULL;
  if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
  {
    buffer->head = buffer->tail = dummy;
  } 
  else // buffer not empty
  {
    buffer->tail->next = dummy;
    buffer->tail = buffer->tail->next; 
  }

  pthread_mutex_unlock(&local_mutex);

  return SBUFFER_SUCCESS;
}



