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

#ifndef QUEUE_H
#define QUEUE_H


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define TRUE 	1
#define FALSE 	0


typedef struct
{
  const char *qname;
  void **buf;
  unsigned long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  unsigned long size;
} syncqueue_t;


// public queue.c functions
void *get (syncqueue_t * queue);
void put (syncqueue_t * queue, void *elem);
void enqueue (syncqueue_t * q, void *elem);
syncqueue_t *syncqueue_init (const char *qname, unsigned long queuesize);
void syncqueue_destroy (syncqueue_t * q);


#endif // QUEUE_H
