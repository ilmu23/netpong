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

#ifndef EXEC_NAME
# define EXEC_NAME "netpong"
#endif

#define SGR_FATAL	"\x1b[1;38;5;124m"
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
