/*
 * ringbuf.h - Ring buffer header file
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
 * $Id: ringbuf.h,v 1.2 2005-05-26 04:38:36 cmn Exp $
 */

#ifndef _RINGBUF_H
#define _RINGBUF_H

#include <sys/types.h>

/* Get current size of buffer */
#define ringbuf_currsize(r)	((r)->size_curr)

/* Get maximum allowed buffer size */
#define ringbuf_maxsize(r)	((r)->size_max)

/* Get the number of elements in the buffer */
#define ringbuf_elements(r)	((r)->num_elems)

/* Get the amount of bytes left in the buffer */
#define ringbuf_sizeleft(r)	(ringbuf_maxsize(r) - ringbuf_currsize(r))


struct ringbuf {
	size_t size_max;	/* Maximum size allowed */
	size_t size_curr;	/* Current size */
	size_t num_elems;	/* Number of elements in buffer */

	struct r_list {
		void *elem;
		size_t size;
		struct r_list *prev;
		struct r_list *next;
	} *first;

	struct r_list *last;
};


/* ringbuf.c */
extern void *ringbuf_last(struct ringbuf *, size_t *);
extern int ringbuf_resize(struct ringbuf *, size_t);
extern void *ringbuf_first(struct ringbuf *, size_t *);
extern int ringbuf_add(struct ringbuf *, void *, size_t);
extern struct ringbuf *ringbuf_init(size_t);
extern const void *ringbuf_peek_last(struct ringbuf *);
extern const void *ringbuf_peek_first(struct ringbuf *);

#endif /* _RINGBUF_H */
