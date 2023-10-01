/**
 * This file implements the Logging API specified in logger.h.
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "logger.h"

static enum log_level_e current_log_level = LOG_INFO; ///< Global variable defining the current log level.

void set_log_level(const enum log_level_e log_level) {
	assert(log_level >= LOG_NONE);

	current_log_level = log_level;
}

void log_message(const enum log_level_e log_level, const char * const message) {
	assert(log_level > LOG_NONE);
	assert(message != NULL);

	if (log_level > current_log_level) {
		return;
	}

	(void) fprintf(stderr, "%s\n", message);
}

void log_printf(const enum log_level_e log_level, const char * const format, ...) {
	assert(log_level > LOG_NONE);
	assert(format != NULL);

	if (log_level > current_log_level) {
		return;
	}

	va_list ap;

	va_start(ap, format);
	(void) vfprintf(stderr, format, ap);
	va_end(ap);

	(void) fputs("\n", stderr);
}
