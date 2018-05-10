/* $%BEGINLICENSE%$
 Copyright (c) 2012, Oracle and/or its affiliates. All rights reserved.

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
 * SECTION:network-packet
 * @short_description: a seekable packet 
 *
 * a GString that knows where we stopped reading
 */


#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "network-packet.h"

/**
 * network_packet_new:
 *
 * create a new network packet
 *
 * Returns: a #network_packet
 */
network_packet *
network_packet_new(void) {
	network_packet *packet;

	packet = g_new0(network_packet, 1);

	return packet;
}

/**
 * network_packet_free:
 * @packet: a #network_packet
 *
 * free a #network_packet
 */
void
network_packet_free(network_packet *packet) {
	if (!packet) return;

	g_free(packet);
}

/**
 * network_packet_has_more_data:
 * @packet: a packet
 * @len: number of bytes to test for
 *
 * check if there is more data in the packet
 *
 * Returns: %TRUE if more data is in the packet, %FALSE if not
 */
gboolean
network_packet_has_more_data(network_packet *packet, gsize len) {
	if (packet->offset > packet->data->len) return FALSE; /* we are already out of bounds, shouldn't happen */
	if (len > packet->data->len - packet->offset) return FALSE;

	return TRUE;
}

/**
 * network_packet_skip:
 * @packet: a packet
 * @len: number of bytes to skip
 *
 * move read offset forward by @len bytes
 *
 * Returns: %TRUE if @len bytes could be skipped, %FALSE if not
 */
gboolean
network_packet_skip(network_packet *packet, gsize len) {
	if (!network_packet_has_more_data(packet, len)) {
		return FALSE;
	}

	packet->offset += len;
	return TRUE;
}

/**
 * network_packet_peek_data:
 * @packet: a packet
 * @dst: destination buffer of at least size @len
 * @len: number of bytes to copy
 *
 * copies @len bytes from @packet at position #network_packet.offset into @dst without moving #network_packet.offset forward
 *
 * Returns: %TRUE if @len byte could be copied into @dst
 * See: network_packet_get_data()
 */
gboolean
network_packet_peek_data(network_packet *packet, gpointer dst, gsize len) {
	if (!network_packet_has_more_data(packet, len)) return FALSE;

	memcpy(dst, packet->data->str + packet->offset, len);

	return TRUE;
}

/**
 * network_packet_get_data:
 * @packet: a packet
 * @dst: destination buffer of at least size @len
 * @len: number of bytes to copy
 *
 * copies @len bytes from @packet at position #network_packet.offset into @dst and moves #network_packet.offset forward by @len bytes
 *
 * Returns: %TRUE if @len byte could be copied into @dst
 * See: network_packet_peek_data()
 */
gboolean
network_packet_get_data(network_packet *packet, gpointer dst, gsize len) {
	if (!network_packet_peek_data(packet, dst, len)) {
		return FALSE;
	}

	packet->offset += len;

	return TRUE;
}

