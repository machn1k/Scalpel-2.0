// Scalpel Copyright (C) 2005-11 by Golden G. Richard III and 
// 2007-11 by Vico Marziale.
// Written by Golden G. Richard III and Vico Marziale.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//
// Thanks to Kris Kendall, Jesse Kornblum, et al for their work 
// on Foremost.  Foremost 0.69 was used as the starting point for 
// Scalpel, in 2005.

// A fifo queue with synchronization support.

#include "syncqueue.h"

static void *dequeue(syncqueue_t * q);


// synchronized wrapper for getting out of queue.
void *get(syncqueue_t * queue) {

  void *elem;
  pthread_mutex_lock(queue->mut);
  while (queue->empty) {
//              printf("queue %s EMPTY.\n", queue->qname);
    pthread_cond_wait(queue->notEmpty, queue->mut);
  }
  elem = dequeue(queue);
  pthread_mutex_unlock(queue->mut);
  pthread_cond_signal(queue->notFull);
  return elem;
}


// synchronized wrapper for putting into queue.
void put(syncqueue_t * queue, void *elem) {

  pthread_mutex_lock(queue->mut);
  while (queue->full) {
//              printf ("queue %s FULL.\n", queue->qname);
    pthread_cond_wait(queue->notFull, queue->mut);
  }
  enqueue(queue, elem);
  pthread_mutex_unlock(queue->mut);
  pthread_cond_signal(queue->notEmpty);
}


// create a new initialized queue
syncqueue_t *syncqueue_init(const char *qname, unsigned long size) {

  syncqueue_t *q;

  q = (syncqueue_t *) calloc(1, sizeof(syncqueue_t));
  if(q == NULL) {
    printf("Couldn't create queue! Aborting.");
    exit(1);
  }

  q->buf = (void **)calloc(size, sizeof(void *));
  q->qname = qname;
  q->empty = TRUE;
  q->full = FALSE;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);
  q->size = size;

  return (q);
}


// destroy a queue and reclaim memory
void syncqueue_destroy(syncqueue_t * q) {

  // watch for memory leaks here!!!!
//      free(q->qname);
  pthread_mutex_destroy(q->mut);
  free(q->mut);
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}


// add an element to end of q
void enqueue(syncqueue_t * q, void *elem) {

  q->buf[q->tail] = elem;
  q->tail++;
  if(q->tail == q->size)
    q->tail = 0;
  if(q->tail == q->head)
    q->full = TRUE;
  q->empty = FALSE;
}


// remove and return the oldest packet_t in the queue
static void *dequeue(syncqueue_t * q) {

  void *next = q->buf[q->head];

  q->head++;
  if(q->head == q->size)
    q->head = 0;
  if(q->head == q->tail)
    q->empty = TRUE;
  q->full = FALSE;

  return next;
}
