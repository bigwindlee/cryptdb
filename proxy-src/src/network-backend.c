/* $%BEGINLICENSE%$
 Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.

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
 * SECTION:network-backend
 * @short_description: backends
 *
 * backends that a socket can connect to
 */

#include <string.h>

#include <glib.h>

#include "network-backend.h"
#include "chassis-plugin.h"
#include "glib-ext.h"

#define C(x) x, sizeof(x) - 1
#define S(x) x->str, x->len

/**
 * backend_init:
 *
 * create a #network_backend_t
 *
 * Deprecated: 0.8.0: use network_backend_new() instead
 */
network_backend_t *
backend_init(void) {
	return network_backend_new();
}

/**
 * network_backend_new:
 *
 * create a new #network_backend_t
 *
 * Returns: a new #network_backend_t
 */
network_backend_t *
network_backend_new(void) {
	network_backend_t *b;

	b = g_new0(network_backend_t, 1);

	b->pool = network_connection_pool_new();
	b->uuid = g_string_new(NULL);
	b->addr = network_address_new();

	return b;
}

/**
 * backend_free:
 * @b: backend
 *
 * free a backend
 *
 * Deprecated: 0.8.0: network_backend_free()
 */
void
backend_free(network_backend_t *b) {
	network_backend_free(b);
}

/**
 * network_backend_free:
 * @b: backend
 *
 * free a backend
 */
void
network_backend_free(network_backend_t *b) {
	if (!b) return;

	network_connection_pool_free(b->pool);

	if (b->addr)     network_address_free(b->addr);
	if (b->uuid)     g_string_free(b->uuid, TRUE);

	g_free(b);
}

/**
 * network_backends_new:
 *
 * create a #network_backends_t
 *
 * Returns: a #network_backends_t
 */
network_backends_t *
network_backends_new() {
	network_backends_t *bs;

	bs = g_new0(network_backends_t, 1);

	bs->backends = g_ptr_array_new();
	bs->backends_mutex = g_mutex_new();

	return bs;
}

/**
 * network_backends_free:
 * @bs: a #network_backends_t
 *
 * free a #network_backends_t
 */
void
network_backends_free(network_backends_t *bs) {
	gsize i;

	if (!bs) return;

	g_mutex_lock(bs->backends_mutex);
	for (i = 0; i < bs->backends->len; i++) {
		network_backend_t *backend = bs->backends->pdata[i];
		
		network_backend_free(backend);
	}
	g_mutex_unlock(bs->backends_mutex);

	g_ptr_array_free(bs->backends, TRUE);
	g_mutex_free(bs->backends_mutex);

	g_free(bs);
}

/**
 * network_backends_add:
 * @bs: a #network_backends_t
 * @address: address
 * @type: backend type
 *
 * FIXME: 1) remove _set_address, make this function callable with result of same
 *        2) differentiate between reasons for "we didn't add" (now -1 in all cases)
 */
int
network_backends_add(network_backends_t *bs, /* const */ gchar *address, backend_type_t type) {
	network_backend_t *new_backend;
	guint i;

	new_backend = network_backend_new();
	new_backend->type = type;

	if (0 != network_address_set_address(new_backend->addr, address)) {
		network_backend_free(new_backend);
		return -1;
	}

	/* check if this backend is already known */
	g_mutex_lock(bs->backends_mutex);
	for (i = 0; i < bs->backends->len; i++) {
		network_backend_t *old_backend = bs->backends->pdata[i];

		if (strleq(S(old_backend->addr->name), S(new_backend->addr->name))) {
			network_backend_free(new_backend);

			g_mutex_unlock(bs->backends_mutex);
			g_critical("backend %s is already known!", address);
			return -1;
		}
	}


	g_ptr_array_add(bs->backends, new_backend);
	g_mutex_unlock(bs->backends_mutex);

	g_message("added %s backend: %s", (type == BACKEND_TYPE_RW) ?
			"read/write" : "read-only", address);

	return 0;
}

/**
 * network_backends_check:
 * @bs: a #network_backends_t
 *
 * update the _DOWN state to _UNKNOWN if the backends were down for at least 4 seconds
 *
 * we only check once a second to reduce the overhead on connection setup
 *
 * Returns:  number of updated backends
 */
int
network_backends_check(network_backends_t *bs) {
	GTimeVal now;
	guint i;
	int backends_woken_up = 0;
	gint64	t_diff;

	g_get_current_time(&now);
	ge_gtimeval_diff(&bs->backend_last_check, &now, &t_diff);

	/* check max(once a second) */
	/* this also covers the "time went backards" case */
	if (t_diff < G_USEC_PER_SEC) {
		if (t_diff < 0) {
			g_message("%s: time went backwards (%"G_GINT64_FORMAT" usec)!",
				G_STRLOC, t_diff);
			bs->backend_last_check.tv_usec = 0;
			bs->backend_last_check.tv_sec = 0;
		}
		return 0;
	}
	
	/* check once a second if we have to wakeup a connection */
	g_mutex_lock(bs->backends_mutex);

	bs->backend_last_check = now;

	for (i = 0; i < bs->backends->len; i++) {
		network_backend_t *cur = bs->backends->pdata[i];

		if (cur->state != BACKEND_STATE_DOWN) continue;

		/* check if a backend is marked as down for more than 4 sec */
		if (now.tv_sec - cur->state_since.tv_sec > 4) {
			g_debug("%s.%d: backend %s was down for more than 4 sec, waking it up", 
					__FILE__, __LINE__,
					cur->addr->name->str);

			cur->state = BACKEND_STATE_UNKNOWN;
			cur->state_since = now;
			backends_woken_up++;
		}
	}
	g_mutex_unlock(bs->backends_mutex);

	return backends_woken_up;
}

/**
 * network_backends_get:
 * @bs: a #network_backends_t
 * @ndx: index 
 *
 * get a backend 
 *
 * Returns: a network_backend_t
 */
network_backend_t *
network_backends_get(network_backends_t *bs, guint ndx) {
	if (ndx >= network_backends_count(bs)) return NULL;

	/* FIXME: shouldn't we copy the backend or add ref-counting ? */	
	return bs->backends->pdata[ndx];
}

/**
 * network_backends_count:
 * @bs: a #network_backends_t
 *
 * get number of backends
 *
 * Returns: number of backends
 */
guint
network_backends_count(network_backends_t *bs) {
	guint len;

	g_mutex_lock(bs->backends_mutex);
	len = bs->backends->len;
	g_mutex_unlock(bs->backends_mutex);

	return len;
}

