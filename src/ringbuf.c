/*
 * ringbuf.c - Thread Safe ring buffer routines
 *
 *  Copyright (c) 2005 Claes M. Nyberg <pocpon@fuzzpoint.com>
 *  All rights reserved, all wrongs reversed.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. The name of author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 *  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 *  THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 *  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 *  OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 *  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: ringbuf.c,v 1.3 2005-05-26 15:16:28 cmn Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "print.h"
#include "str.h"
#include "ringbuf.h"


/*
 * Initialize a ring buffer for a maximum of size bytes.
 * Returns a ringbuf pointer on success, NULL on error.
 */
struct ringbuf *
ringbuf_init(size_t size)
{
	struct ringbuf *rbuf;

	if (size == 0) {
		err("ringbuf_init: Got bad size (0) of maximum buffer\n");
		return(NULL);
	}
	
	if ( (rbuf = calloc(1, sizeof(struct ringbuf))) == NULL) {
		err_errno("ringbuf_init: Failed to allocate ringbuf structure");
		return(NULL);
	}

	verbose(1, "Initiated buffer with %s bytes.\n", str_hsize(size));
	rbuf->size_max = size;
	return(rbuf);	
}


/*
 * Add an element to the ring buffer.
 * If the buffer reaches its maximum size, buffers are removed in
 * the insert order until sufficent size are available.
 * Returns0 on success, -1 on error.
 */
int
ringbuf_add(struct ringbuf *rbuf, void *elem, size_t size)
{
	void *tmp;
	struct r_list *new;
	
	if (rbuf == NULL) {
		err("ringbuf_add: Got NULL pointer as buffer\n");
		return(-1);
	}
	
	/* Element can never fit if it's bigger than the 
	 * maximum allowed size */
	if (size > ringbuf_maxsize(rbuf)) {
		err("ringbuf_add: Element size (%u) exceeds maximum "
			"possible value (%u)\n", size, ringbuf_maxsize(rbuf));
		return(-1);
	}

	while (ringbuf_sizeleft(rbuf) < size) {
		void *elem;
		verbose(4, "Buffer to small, %u bytes, need %u bytes. Removing element.\n",
			ringbuf_sizeleft(rbuf), size);
	
		if ( (elem = ringbuf_first(rbuf, NULL)) == NULL) {
			err("ringbuf_add: Failed to remove element\n");
			return(-1);
		}
		free(elem);
	}
	

	if ( (tmp = calloc(1, size + sizeof(struct r_list))) == NULL) {
		err_errno("ringbuf_add: Failed to allocate memory");
		return(-1);
	}

	/* Save a call to calloc now, and free later on by using 
	 * the same chunk for both list information and data */
	new = (struct r_list *)((char *)tmp + size);
	new->elem = tmp;
	new->size = size;
	memcpy(new->elem, elem, size);
	
	/* Count the data to be added */	
	rbuf->num_elems++;
	rbuf->size_curr += size;
	
	
	/* We are always at the end */
	new->next = NULL;
	
	/* First element */	
	if (ringbuf_elements(rbuf) == 1) {
		new->prev = NULL;
		rbuf->first = new;
		rbuf->last = new;
	}
	/* Second element */
	else if (ringbuf_elements(rbuf) == 2) {
		rbuf->first->next = new;
		rbuf->last = new;
		new->prev = rbuf->first;
	}
	
	/* n:th element */
	else {
		new->prev = rbuf->last;
		rbuf->last->next = new;
		rbuf->last = new;
	}
	
	verbose(3, "Added element number %u of size %s bytes\n", 
		ringbuf_elements(rbuf), str_hsize(size));
	verbose(2, "Ring buffer uses %s [%u] bytes\n", 
		str_hsize(ringbuf_currsize(rbuf)), ringbuf_currsize(rbuf));
	return(0);
}



/*
 * Resize buffer.
 * If the new size is less than the current size, 
 * elements are removed in the order they were inserted
 * until the new limit is reached. 
 * Returns the number of elements removed, or -1 on error.
 */
int
ringbuf_resize(struct ringbuf *rbuf, size_t new_size)
{
	size_t deleted = 0;

	verbose(3, "Resizing buffer to %u bytes\n", new_size);

    if (rbuf == NULL) {
		err("ringbuf_resize: Got NULL pointer as buffer\n");
		return(-1);
	}

	if (new_size == 0) {
		err("ringbuf_resize: Got a bad size (0)\n");
		return(-1);
	}

	/* Nothing to fix, simply change the maximum size */
	if (ringbuf_currsize(rbuf) < new_size) {
		rbuf->size_max = new_size;
		return(0);
	}

	/* Remove elements until the current size fits the new size */
	while (ringbuf_currsize(rbuf) > new_size) {
		void *elem;
		
		if ( (elem = ringbuf_first(rbuf, NULL)) == NULL) {
			err("ringbuf_resize: Failed to remove element from buffer\n");
			return(0);
		}
		deleted++;
		free(elem);
	}

	return(deleted);	
}


/*
 * Remove and return the element at the end of the list, 
 * returns NULL if the buffer is empty.
 * Note that you have to free the returned element using free(3),
 * once you have finished using it.
 */
void *
ringbuf_last(struct ringbuf *rbuf, size_t *elem_size)
{
	struct r_list *elem;

	verbose(4, "Removing last element\n");
	
    if (rbuf == NULL) {
		err("ringbuf_last: Got NULL pointer as buffer\n");
		return(NULL);
	}
	
	/* No elements left */
	if (ringbuf_elements(rbuf) == 0)	
		return(NULL);

	elem = rbuf->last;
	rbuf->num_elems--;
	rbuf->size_curr -= elem->size;
	

	/* Last element */
	if (ringbuf_elements(rbuf) == 0) {
		rbuf->last = NULL;
		rbuf->first = NULL;
	}
	
	/* Next last element */
	else if (ringbuf_elements(rbuf) == 0) {
		rbuf->last = rbuf->first;
		rbuf->first->next = NULL;
		rbuf->first->prev = NULL;
	}
	/* N:th element */
	else {
		rbuf->last = elem->prev;
		elem->prev->next = NULL;
	}

	if (elem_size != NULL)
		*elem_size = elem->size;
	
	verbose(3, "Removed element of size %s\n", str_hsize(elem->size));
	return(elem->elem);
}

/*
 * Peek at latest entry in the list.
 */
const void *
ringbuf_peek_last(struct ringbuf *rbuf)
{
	return(rbuf->last->elem);
}


/*
 * Remove and return the element that has spent the longest time 
 * in the buffer, or NULL if the buffer is empty.
 * Note that you have to free the returned element using free(3)
 * once you are finished with it.
 */
void *
ringbuf_first(struct ringbuf *rbuf, size_t *elem_size)
{
	struct r_list *elem;
		
	verbose(4, "Removing first element\n");

    if (rbuf == NULL) {
		err("ringbuf_first: Got NULL pointer as buffer\n");
		return(NULL);
	}

	/* No elements left */
	if (ringbuf_elements(rbuf) == 0)
		return(NULL);

	elem = rbuf->first;
	rbuf->num_elems--;
	rbuf->size_curr -= elem->size;


	/* Last element */
	if (ringbuf_elements(rbuf) == 0) {
		rbuf->first = NULL;
		rbuf->last = NULL;	
	}
	/* Next last element */	
	else if (ringbuf_elements(rbuf) == 1) {
		rbuf->first = rbuf->last;
		rbuf->first->prev = NULL;
		rbuf->first->next = NULL;
		
	}
	/* N:th element */
	else {
		rbuf->first = elem->next;
		rbuf->first->prev = NULL;
	}

	if (elem_size != NULL) 
		*elem_size = elem->size;
	verbose(3, "Removed element of size %s\n", str_hsize(elem->size));
	return(elem->elem);	
}


/*
 * Peek at first entry in the list
 */
const void *
ringbuf_peek_first(struct ringbuf *rbuf)
{
	return(rbuf->first->elem);
}
