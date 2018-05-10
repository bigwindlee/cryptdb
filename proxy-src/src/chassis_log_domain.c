#include <glib.h>

#include "chassis_log_domain.h"

/**
 * SECTION:chassis_log_domain
 * @short_description: log domains
 *
 * filter messages across several domains
 */

/**
 * chassis_log_domain_new:
 * @domain_name: name of the domain
 * @min_level: minimun log-level
 * @backend: backend
 *
 * create a new log-domain
 *
 * Returns: a new #chassis_log_domain_t
 */
chassis_log_domain_t *
chassis_log_domain_new(const gchar *domain_name, GLogLevelFlags min_level, chassis_log_backend_t *backend) {
	chassis_log_domain_t *domain;

	g_return_val_if_fail(domain_name, NULL);

	domain = g_slice_new0(chassis_log_domain_t);

	domain->name = g_strdup(domain_name);
	domain->min_level = min_level;
	domain->backend = backend;
	domain->is_autocreated = FALSE;
	domain->parent = NULL;
	domain->children = g_ptr_array_new();

	return domain;
}

/**
 * chassis_log_domain_free:
 * @domain: a domain
 *
 * free a chassis_log_domain
 */
void
chassis_log_domain_free(chassis_log_domain_t* domain) {
	if (NULL == domain) return;

	if (NULL != domain->name) g_free(domain->name);
	if (NULL != domain->children) g_ptr_array_free(domain->children, TRUE);

	g_slice_free(chassis_log_domain_t, domain);
}

/**
 * chassis_log_domain_log:
 * @domain: the logger to validate the level against
 * @level: the log level of the message
 * @message: the string to log
 *
 * Conditionally logs a message to a logger's backend.
 * 
 * This function performs the checking of the effective log level against the message's log level.
 * It does not modify the message in any way.
 */
void
chassis_log_domain_log(chassis_log_domain_t* domain, GLogLevelFlags level, const gchar *message) {
	if (level != CHASSIS_LOG_LEVEL_BROADCAST &&  /* _BROADCAST is logged always */
	    domain->effective_level < level) {
		return;
	}
	chassis_log_backend_log(domain->backend, domain->name, level, message);
}


