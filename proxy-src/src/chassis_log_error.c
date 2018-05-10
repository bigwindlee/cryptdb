#include <glib.h>

#include "chassis_log_error.h"

/**
 * SECTION:chassis_log_error
 * @short_description: error domain for #chassis_log_t
 *
 * error domain for #chassis_log_t
 */

/**
 * chassis_log_error:
 *
 * chassis-log error-domain
 *
 * Returns: #GQuark of the chassis-log error-domain
 */
GQuark
chassis_log_error(void) {
	return g_quark_from_static_string("chassis_log_error");
}

