// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<misc.c>>

#include "utils/misc.h"

static inline void	_print_message(const char *fmt, va_list args, const char *sgr);

static pthread_mutex_t	log_lock = PTHREAD_MUTEX_INITIALIZER;

[[gnu::noreturn]] void	die(const char *msg) {
	printf(SGR_FATAL EXEC_NAME ": %s\n" SGR_RESET, msg);
	exit(1);
}

u8	error([[maybe_unused]] const char *fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_ERROR
	va_list	args;

	va_start(args, fmt);
	_print_message(fmt, args, SGR_ERROR);
	va_end(args);
#endif
	return 0;
}

u8	warn([[maybe_unused]] const char *fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_WARN
	va_list	args;

	va_start(args, fmt);
	_print_message(fmt, args, SGR_WARN);
	va_end(args);
#endif
	return 0;
}

u8	info([[maybe_unused]] const char *fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_INFO
	va_list	args;

	va_start(args, fmt);
	_print_message(fmt, args, SGR_INFO);
	va_end(args);
#endif
	return 0;
}

u8	debug([[maybe_unused]] const char *fmt, ...) {
#if LOG_LEVEL >= LOG_LEVEL_DEBUG
	va_list	args;

	va_start(args, fmt);
	_print_message(fmt, args, SGR_DEBUG);
	va_end(args);
#endif
	return 0;
}

static inline void	_print_message(const char *fmt, va_list args, const char *sgr) {
	pthread_mutex_lock(&log_lock);
	fprintf(stderr, "%s", sgr);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n" SGR_RESET);
	fflush(stderr);
	pthread_mutex_unlock(&log_lock);
}
