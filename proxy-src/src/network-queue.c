/* $%BEGINLICENSE%$
 Copyright (c) 2009, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; version 2 of the
 License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA

 $%ENDLICENSE%$ */
 
/**
 * SECTION:network-queue
 * @short_description: a queue of packets
 *
 * a queue of #GString
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "network-queue.h"

/**
 * network_queue_init:
 *
 * create a #network_queue
 *
 * Returns: a #network_queue
 * Deprecated: 0.7.0: use network_queue_new() instead
 */
network_queue *
network_queue_init() {
	return network_queue_new();
}

/**
 * network_queue_new:
 *
 * create a #network_queue
 *
 * Returns: a #network_queue
 */
network_queue *
network_queue_new() {
	network_queue *queue;

	queue = g_new0(network_queue, 1);

	queue->chunks = g_queue_new();
	
	return queue;
}

/**
 * network_queue_free:
 * @queue: a #network_queue
 *
 * free a #network_queue
 */
void
network_queue_free(network_queue *queue) {
	GString *packet;

	if (!queue) return;

	while ((packet = g_queue_pop_head(queue->chunks))) g_string_free(packet, TRUE);

	g_queue_free(queue->chunks);

	g_free(queue);
}

/**
 * network_queue_append:
 * @queue: a queue
 * @chunk: a packet
 *
 * append a @chunk to the @queue
 *
 * Returns: 0 on success
 */
int
network_queue_append(network_queue *queue, GString *chunk) {
	queue->len += chunk->len;

	g_queue_push_tail(queue->chunks, chunk);

	return 0;
}

/**
 * network_queue_peek_string:
 * @queue:     the queue to read from
 * @peek_len:  bytes to collect
 * @dest:      GString to write it. If NULL, we allow a new one and return it
 *
 * get a string from the head of the queue and leave the queue unchanged 
 *
 * Returns: NULL if not enough data, if dest is not NULL, dest, otherwise a new GString containing the data
 */
GString *
network_queue_peek_string(network_queue *queue, gsize peek_len, GString *dest) {
	gsize we_want = peek_len;
	GList *node;

/* TODO: convert to DTrace probe
	g_debug("[%s] looking for %d bytes, queue has %d", G_STRLOC, peek_len, queue->len); */
	if (queue->len < peek_len) {
		return NULL;
	}

	if (!dest) {
		/* no define */
		dest = g_string_sized_new(peek_len);
	}

	g_assert_cmpint(dest->allocated_len, >, peek_len);

	for (node = queue->chunks->head; node && we_want; node = node->next) {
		GString *chunk = node->data;

		if (node == queue->chunks->head) {
			gsize we_have = we_want < (chunk->len - queue->offset) ? we_want : (chunk->len - queue->offset);

			g_string_append_len(dest, chunk->str + queue->offset, we_have);
			
			we_want -= we_have;
		} else {
			gsize we_have = we_want < chunk->len ? we_want : chunk->len;
			
			g_string_append_len(dest, chunk->str, we_have);

			we_want -= we_have;
		}
	}

	return dest;
}

/**
 * network_queue_pop_string:
 * @queue: a network queue
 * @steal_len: number of bytes to take from the queue
 * @dest: destination
 *
 * get a string from the head of the queue and remove the chunks from the queue 
 *
 * Returns: NULL if not enough data is available, @dest otherwise
 */
GString *network_queue_pop_string(network_queue *queue, gsize steal_len, GString *dest) {
	gsize we_want = steal_len;
	GString *chunk;

	if (queue->len < steal_len) {
		return NULL;
	}

	while ((chunk = g_queue_peek_head(queue->chunks))) {
		gsize we_have = we_want < (chunk->len - queue->offset) ? we_want : (chunk->len - queue->offset);

		if (!dest && (queue->offset == 0) && (chunk->len == steal_len)) {
			/* optimize the common case that we want to have to full chunk
			 *
			 * if dest is null, we can remove the GString from the queue and return it directly without
			 * copying it
			 */
			dest = g_queue_pop_head(queue->chunks);
			queue->len -= we_have;
			return dest;
		}

		if (!dest) {
			/* if we don't have a dest-buffer yet, create one */
			dest = g_string_sized_new(steal_len);
		}
		g_string_append_len(dest, chunk->str + queue->offset, we_have);

		queue->offset += we_have;
		queue->len    -= we_have;
		we_want -= we_have;

		if (chunk->len == queue->offset) {
			/* the chunk is done, remove it */
			g_string_free(g_queue_pop_head(queue->chunks), TRUE);
			queue->offset = 0;
		} else {
			break;
		}
	}

	return dest;
}


