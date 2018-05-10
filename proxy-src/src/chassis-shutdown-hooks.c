#include <stdlib.h>
#include <string.h>

#include "chassis-shutdown-hooks.h"
#include "glib-ext.h"

/**
 * SECTION:chassis-shutdown-hooks
 * @short_description: shutdown hooks
 *
 * called at shutdown of the chassis to free global structures once
 *
 * if two plugins use e.g. openssl only the last one unloading should free openssls global
 * structures
 */

static void
g_string_free_true(gpointer data) {
	g_string_free(data, TRUE);
}

/**
 * chassis_shutdown_hook_new:
 *
 * create a #chassis_shutdown_hook_t
 *
 * Returns: a #chassis_shutdown_hook_t
 */
chassis_shutdown_hook_t *
chassis_shutdown_hook_new() {
	chassis_shutdown_hook_t *hook;

	hook = g_slice_new0(chassis_shutdown_hook_t);
	hook->func = NULL;
	hook->udata = NULL;
	hook->is_called = FALSE;

	return hook;
}

/**
 * chassis_shutdown_hook_free:
 * @hook: a #chassis_shutdown_hook_t
 * 
 * free a #chassis_shutdown_hook_t
 */
void
chassis_shutdown_hook_free(chassis_shutdown_hook_t *hook) {
	g_slice_free(chassis_shutdown_hook_t, hook);
}

/**
 * chassis_shutdown_hooks_new:
 *
 * create a #chassis_shutdown_hooks_t
 *
 * Returns: a #chassis_shutdown_hooks_t
 */
chassis_shutdown_hooks_t *
chassis_shutdown_hooks_new() {
	chassis_shutdown_hooks_t *hooks;

	hooks = g_slice_new0(chassis_shutdown_hooks_t);
	hooks->hooks = g_hash_table_new_full(
			(GHashFunc)g_string_hash,
			(GEqualFunc)g_string_equal,
			g_string_free_true,
			(GDestroyNotify)chassis_shutdown_hook_free);
	hooks->mutex = g_mutex_new();

	return hooks;
}

/**
 * chassis_shutdown_hooks_free:
 * @hooks: a #chassis_shutdown_hooks_t
 *
 * free a #chassis_shutdown_hooks_t
 */
void
chassis_shutdown_hooks_free(chassis_shutdown_hooks_t *hooks) {
	g_hash_table_destroy(hooks->hooks);
	g_mutex_free(hooks->mutex);

	g_slice_free(chassis_shutdown_hooks_t, hooks);
}

/**
 * chassis_shutdown_hooks_lock:
 * @hooks: a #chassis_shutdown_hooks_t
 *
 * lock a @hooks
 */
void
chassis_shutdown_hooks_lock(chassis_shutdown_hooks_t *hooks) {
	g_mutex_lock(hooks->mutex);
}

/**
 * chassis_shutdown_hooks_unlock:
 * @hooks: a #chassis_shutdown_hooks_t
 *
 * unlock @hooks
 */
void
chassis_shutdown_hooks_unlock(chassis_shutdown_hooks_t *hooks) {
	g_mutex_unlock(hooks->mutex);
}

/**
 * chassis_shutdown_hooks_register:
 * @hooks: a #chassis_shutdown_hooks_t
 * @key: hook name
 * @key_len: length of @key
 * @hook: hook to register
 *
 * register a shutdown hook
 *
 * Returns: %TRUE if registering succeeded, %FALSE if already known
 */
gboolean
chassis_shutdown_hooks_register(chassis_shutdown_hooks_t *hooks,
		const char *key, gsize key_len,
		chassis_shutdown_hook_t *hook) {
	gboolean is_inserted = FALSE;

	chassis_shutdown_hooks_lock(hooks);
	if (NULL == g_hash_table_lookup_const(hooks->hooks, key, key_len)) {
		g_hash_table_insert(hooks->hooks, g_string_new_len(key, key_len), hook);
		is_inserted = TRUE;
	}
	chassis_shutdown_hooks_unlock(hooks);

	return is_inserted;
}

/**
 * chassis_shutdown_hooks_call:
 * @hooks: a #chassis_shutdown_hooks_t
 *
 * call all shutdown hooks
 */
void
chassis_shutdown_hooks_call(chassis_shutdown_hooks_t *hooks) {
	GHashTableIter iter;
	GString *key;
	chassis_shutdown_hook_t *hook;

	chassis_shutdown_hooks_lock(hooks);
	g_hash_table_iter_init(&iter, hooks->hooks);
	while (g_hash_table_iter_next(&iter, (void **)&key, (void **)&hook)) {
		if (hook->func && !hook->is_called) hook->func(hook->udata);
		hook->is_called = TRUE;
	}
	chassis_shutdown_hooks_unlock(hooks);
}

