// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<utils.c>>

#include "utils.h"

static inline void	_print_message(const char *fmt, va_list args, const char *sgr);

[[gnu::noreturn]] void	die(const char *msg) {
	printf(SGR_FATAL EXEC_NAME ": %s\n" SGR_RESET, msg);
	exit(1);
}

u8	error(const char *fmt, ...) {
	va_list	args;

	va_start(args);
	_print_message(fmt, args, SGR_ERROR);
	va_end(args);
	return 0;
}

u8	warn(const char *fmt, ...) {
	va_list	args;

	va_start(args);
	_print_message(fmt, args, SGR_WARN);
	va_end(args);
	return 0;
}

u8	info(const char *fmt, ...) {
	va_list	args;

	va_start(args);
	_print_message(fmt, args, SGR_INFO);
	va_end(args);
	return 0;
}

u8	debug(const char *fmt, ...) {
	va_list	args;

	va_start(args);
	_print_message(fmt, args, SGR_DEBUG);
	va_end(args);
	return 0;
}

static inline void	_print_message(const char *fmt, va_list args, const char *sgr) {
	fprintf(stderr, "%s", sgr);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n" SGR_RESET);
	fflush(stderr);
}
