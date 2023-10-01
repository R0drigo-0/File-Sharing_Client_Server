/**
 * A Logging API.
 *
 * Usage:
 *
 *   set_log_level(LOG_INFO);
 *
 *   log_message(LOG_DEBUG, "This message will not be shown.");
 *   log_message(LOG_INFO, "This message will be shown.");
 *   log_message(LOG_INFO, "And this one too.");
 *
 *   log_printf(LOG_INFO, "This message will be shown %s.", "using printf() syntax");
 *
 *   size_t this_is_likely_a_64_bit_integer = 3;
 *   log_printf(LOG_INFO, "This is a 32-bit integer: %d.", (int) 3);
 */

#ifndef LOGGER_H_
#define LOGGER_H_

/**
 * Enumeration of the various log levels.
 */
enum log_level_e {
	LOG_NONE = 0, //!< No messages displayed. It shall not be used when calling log_message and log_printf.
	LOG_INFO = 1, //!< Informative message displayed.
	LOG_DEBUG = 2 //!< Truckloads of information displayed.
};

/**
 * Changes the log level. Messages of current level or below will be displayed.
 * @param log_level is the new log level.
 */
void set_log_level(const enum log_level_e log_level);

/**
 * Log a message.
 * @param log_level is the log level associated with this message.
 * @param message is the message.
 */
void log_message(const enum log_level_e log_level, const char * const message);

/**
 * Log a message using printf() syntax.
 * @param log_level is the log level associated with this message.
 * @param format is the message formatting string.
 */
void log_printf(const enum log_level_e log_level, const char * const format, ...);

#endif // LOGGER_H_
