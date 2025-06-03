// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<defs.h>>

#pragma once

#include <stdint.h>

typedef	int8_t		i8;
typedef	int16_t		i16;
typedef	int32_t		i32;
typedef	int64_t		i64;

typedef	uint8_t		u8;
typedef	uint16_t	u16;
typedef	uint32_t	u32;
typedef	uint64_t	u64;

typedef float		f32;
typedef double		f64;

#define lock_game(game_id)		(pthread_mutex_lock(&state_locks[game_id]))
#define unlock_game(game_id)	(pthread_mutex_unlock(&state_locks[game_id]))

#define	rand_range(min, max)	(rand() % (max - min + 1) + min)

#ifndef EXEC_NAME
# define EXEC_NAME "netpong_server"
#endif

#define LOG_LEVEL_FATAL	0
#define LOG_LEVEL_ERROR	1
#define LOG_LEVEL_WARN	2
#define LOG_LEVEL_INFO	3
#define LOG_LEVEL_DEBUG	4

#ifndef LOG_LEVEL
# define LOG_LEVEL LOG_LEVEL_INFO
#endif

#ifndef SGR_FATAL
# define SGR_FATAL	"\x1b[1;38;5;124m"
#endif

#ifndef SGR_ERROR
# define SGR_ERROR	"\x1b[1;38;5;196m"
#endif

#ifndef SGR_WARN
# define SGR_WARN	"\x1b[1;38;5;202m"
#endif

#ifndef SGR_INFO
# define SGR_INFO	"\x1b[1;38;5;99m"
#endif

#ifndef SGR_DEBUG
# define SGR_DEBUG	"\x1b[1;38;5;85m"
#endif

#define SGR_RESET	"\x1b[m"

#define PROTOCOL_VERSION	0

#define MAX_GAME_COUNT	100

#define MESSAGE_CLIENT_START		0
#define MESSAGE_CLIENT_PAUSE		1
#define MESSAGE_CLIENT_MOVE_PADDLE	2

#define MESSAGE_SERVER_GAME_INIT		0
#define MESSAGE_SERVER_GAME_PAUSED		1
#define MESSAGE_SERVER_GAME_OVER		2
#define MESSAGE_SERVER_STATE_UPDATE		3
