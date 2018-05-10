/* $%BEGINLICENSE%$
 Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.

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

#include <glib.h>

#include "chassis-timings.h"
#include "glib-ext.h"

/**
 * SECTION:chassis-timings
 * @short_description: trace-points 
 *
 * trace-points in the executing with source locations and names
 *
 * TODO: replace with dtrace and its linux equivs
 */

#define MICROS_IN_SEC 1000000

chassis_timestamps_global_t *chassis_timestamps_global = NULL;

/**
 * chassis_timestamp_new:
 *
 * create a #chassis_timestamp_t
 *
 * Returns: a #chassis_timestamp_t
 */
chassis_timestamp_t *
chassis_timestamp_new(void) {
	chassis_timestamp_t *ts;

	ts = g_new0(chassis_timestamp_t, 1);

	return ts;
}

/**
 * chassis_timestamp_init_now:
 * @ts: a #chassis_timestamp_t
 * @name: name of the timestamp (a static string)
 * @filename: __FILE__
 * @line: __LINE__
 *
 * init @ts with the current time information
 */
void
chassis_timestamp_init_now(chassis_timestamp_t *ts,
		const char *name,
		const char *filename,
		gint line) {

	ts->name = name;
	ts->filename = filename;
	ts->line = line;
	ts->usec = my_timer_microseconds();
	ts->cycles = my_timer_cycles();
	ts->ticks = my_timer_ticks();
}

/**
 * chassis_timestamp_free:
 * @ts: a #chassis_timestamp_t
 *
 * free a #chassis_timestamp_t
 */
void
chassis_timestamp_free(chassis_timestamp_t *ts) {
	g_free(ts);
}

/**
 * chassis_timestamps_new:
 *
 * create a #chassis_timestamps_t
 *
 * Returns: a #chassis_timestamps_t
 */
chassis_timestamps_t *
chassis_timestamps_new(void) {
	chassis_timestamps_t *ts;

	ts = g_new0(chassis_timestamps_t, 1);
	ts->timestamps = g_queue_new();

	return ts;
}

/**
 * chassis_timestamps_free:
 * @ts: #chassis_timestamps_t
 *
 * free #chassis_timestamps_t
 */
void
chassis_timestamps_free(chassis_timestamps_t *ts) {
	chassis_timestamp_t *t;

	while ((t = g_queue_pop_head(ts->timestamps))) chassis_timestamp_free(t);
	g_queue_free(ts->timestamps);
	g_free(ts);
}

/**
 * chassis_timestamps_add:
 * @ts: a #chassis_timestamps_t
 * @name: name of the timestamp
 * @filename: filename
 * @line: line in @filename
 *
 * add a current timestamp named @name for @filename and @line to @ts
 */
void
chassis_timestamps_add(chassis_timestamps_t *ts,
		const char *name,
		const char *filename,
		gint line) {
	chassis_timestamp_t *t;

	t = chassis_timestamp_new();
	chassis_timestamp_init_now(t, name, filename, line);

	g_queue_push_tail(ts->timestamps, t);
}

/**
 * chassis_get_rel_milliseconds:
 *
 * Retrieve a timestamp with a millisecond resolution.
 *
 * @note The return value must not be assumed to be based on any specific epoch, it is only to be used as a relative measure.
 * Returns: the current timestamp
 */
guint64
chassis_get_rel_milliseconds() {
	return my_timer_milliseconds();
}

/**
 * chassis_get_rel_microseconds:
 *
 * Retrieve a timestamp with a microsecond resolution.
 *
 * @note The return value must not be assumed to be based on any specific epoch, it is only to be used as a relative measure.
 * Returns: the current timestamp
 */
guint64
chassis_get_rel_microseconds() {
	return my_timer_microseconds();
}

/**
 * chassis_calc_rel_microseconds:
 * @start: the start time
 * @stop: the end time
 *
 * Calculate the difference between two relative microsecond readings, taking into account a potential timer frequency.
 *
 * @note This is especially necessary for Windows, do _not_ simply subtract the relative readings, those are _not_ in microseconds!
 * Returns: the difference of stop and start, adjusted to microseconds
 */
guint64
chassis_calc_rel_microseconds(guint64 start, guint64 stop) {
#ifdef WIN32
	guint64 frequency;
	g_assert(chassis_timestamps_global != NULL);
	frequency = chassis_timestamps_global->microseconds_frequency;
	if (0 == frequency) {
		g_critical("High resolution counter QueryPerformanceCounter not available on this system. All timer values will be meaningless.");
		return stop - start;
	}
	return (guint64) ((stop - start) * (1.0 / frequency) * MICROS_IN_SEC);
#else
	return stop - start;
#endif
}

/**
 * chassis_get_rel_nanoseconds:
 *
 * Retrieve a timestamp with a nanosecond resolution.
 *
 * @note The return value must not be assumed to be based on any specific epoch, it is only to be used as a relative measure.
 * Returns: the current timestamp
 */
guint64
chassis_get_rel_nanoseconds() {
	return my_timer_nanoseconds();
}


/**
 * chassis_timestamps_global_init:
 * @gl: (allow-none): the new timer or %NULL to initialize the global timer base
 *
 * Creates a new timer base, which will calibrate itself during creation.
 *
 * @note This function is not threadsafe.
 */
void
chassis_timestamps_global_init(chassis_timestamps_global_t *gl) {
	chassis_timestamps_global_t *timestamps = gl;

	if (NULL == gl) {
		if (NULL != chassis_timestamps_global) {
			g_warning("%s: invalid attempt to reinitialize the global chassis timer info, ignoring call, still using %p",
					G_STRLOC, (void*)chassis_timestamps_global);
			return;
		} else {
			chassis_timestamps_global = g_new0(chassis_timestamps_global_t, 1);
		}
		timestamps = chassis_timestamps_global;
		g_debug("%s: created new global chassis timer info at %p", G_STRLOC, (void*)chassis_timestamps_global);
	}
	my_timer_init(timestamps);
}

/**
 * chassis_timestamps_global_free:
 * @gl: (allow-none): the timer or %NULL to free the global timer base
 *
 * free the global timer base
 */
void
chassis_timestamps_global_free(chassis_timestamps_t *gl) {
	if (NULL == gl) {
		g_free(chassis_timestamps_global);
	} else {
		g_free(gl);
	}
}
