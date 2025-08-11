// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<data.h>>

#pragma once

#include "defs.h"
#include "pong/data.h"

#include <pthread.h>

typedef struct __game {
	state	state;
	struct {
		pthread_t	game_master;
		pthread_t	game_loop;
		pthread_t	player_io;
	}	threads;
	struct {
		i32	state;
		i32	p1;
		i32	p2;
	}	sockets;
	struct {
		u16	p1;
		u16	p2;
	}	ports;
	i32	epoll_instance;
	u8	id;
}	game;

#if PROTOCOL_VERSION >= 0

#define MESSAGE_HEADER_SIZE 4

typedef struct [[gnu::packed]] __message {
	u8	version;
	u8	type;
	u16	length;
	u8	body[18];
}	message;

typedef struct [[gnu::packed]] __msg_srv_init {
	u16	p1_port;
	u16	p2_port;
}	msg_srv_init;

#define _msg_srv_init(_p1, _p2)	((msg_srv_init){.p1_port = _p1, .p2_port = _p2})

typedef struct [[gnu::packed]] __msg_srv_state {
	f32	p1_paddle;
	f32	p2_paddle;
	struct {
		f32	x;
		f32	y;
	}	ball;
	u16	score;
}	msg_srv_state;

#define _msg_srv_state(_p1, _p2, _x, _y, _sc)	((msg_srv_state){.p1_paddle = _p1, .p2_paddle = _p2, .ball.x = _x, .ball.y = _y, .score = (_sc.p1 << 8) | _sc.p2})

#if PROTOCOL_VERSION >= 1

typedef struct [[gnu::packed]] __msg_srv_game_over {
	u8	actor_id;
	u8	finish_status;
	u16	score;
}	msg_srv_game_over;

#define _msg_srv_game_over(_aid, _fs, _sc)	((msg_srv_game_over){.actor_id = _aid, .finish_status = _fs, .score = (_sc.p1 << 8) | _sc.p2})

#endif /* PROTOCOL_VERSION >= 1 */

#endif /* PROTOCOL_VERSION >= 0 */
