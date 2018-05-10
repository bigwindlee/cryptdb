#ifndef _CHASSIS_STATS_PRIVATE_H_
#define _CHASSIS_STATS_PRIVATE_H_

#include <glib.h>
#include "chassis-exports.h"

CHASSIS_API chassis_stats_t *chassis_global_stats;

struct chassis_stats {
	/*< private >*/
	volatile gint lua_mem_alloc;
	volatile gint lua_mem_free;
	volatile gint lua_mem_bytes;
	volatile gint lua_mem_bytes_max;
};

/**
 * CHASSIS_STATS_ALLOC_INC_NAME:
 * @name: prefix of a #chassis_stats_t field
 *
 * increment the chassis_global_stats->$(name)_alloc if chassis_global_stats is already setup
 */
#define CHASSIS_STATS_ALLOC_INC_NAME(name) ((chassis_global_stats != NULL) ? g_atomic_int_inc(&(chassis_global_stats->name ## _alloc)) : (void)0)
/**
 * CHASSIS_STATS_FREE_INC_NAME:
 * @name: prefix of a #chassis_stats_t field
 *
 * increment the chassis_global_stats->$(name)_free if chassis_global_stats is already setup
 */
#define CHASSIS_STATS_FREE_INC_NAME(name) ((chassis_global_stats != NULL) ? g_atomic_int_inc(&(chassis_global_stats->name ## _free)) : (void)0)
/**
 * CHASSIS_STATS_ADD_NAME:
 * @name: prefix of a #chassis_stats_t field
 * @addme: increment
 *
 * add @addme to chassis_global_stats->$(name) if chassis_global_stats is already setup
 */
#define CHASSIS_STATS_ADD_NAME(name, addme) ((chassis_global_stats != NULL) ? g_atomic_int_add(&(chassis_global_stats->name), addme) : (void)0)
/**
 * CHASSIS_STATS_GET_NAME:
 * @name: prefix of a #chassis_stats_t field
 *
 * get the chassis_global_stats->$(name) if chassis_global_stats is already setup, 0 otherwise
 */
#define CHASSIS_STATS_GET_NAME(name) ((chassis_global_stats != NULL) ? g_atomic_int_get(&(chassis_global_stats->name)) : 0)
/**
 * CHASSIS_STATS_SET_NAME:
 * @name: prefix of a #chassis_stats_t field
 * @setme: value
 *
 * set chassis_global_stats->$(name) to @value if chassis_global_stats is already setup
 */
#define CHASSIS_STATS_SET_NAME(name, setme) ((chassis_global_stats != NULL) ? g_atomic_int_set(&(chassis_global_stats->name), setme) : (void)0)


#endif
