// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<pong.c>>

#include "pong/pong.h"
#include "server/server.h"

pthread_mutex_t	state_locks[MAX_GAME_COUNT];
static state	*games[MAX_GAME_COUNT];

static inline u8	_tick(state *game);

void	*pong(void *arg) {
	u8	game_id;

	game_id =((state *)arg)->game_id;
	games[game_id] = arg;
	while (_tick(games[game_id]))
		;
	return NULL;
}

void	unpause_game(const u8 game_id, const u8 player) {
	lock_game(game_id);
	switch (player) {
		case GAME_PLAYER_1:
		case GAME_PLAYER_2:
			games[game_id]->pause &= ~player;
	}
	if (!games[game_id]->pause)
		games[game_id]->update = GAME_UPDATE_ALLOWED;
	unlock_game(game_id);
}

void	pause_game(const u8 game_id, const u8 player) {
	lock_game(game_id);
	switch (player) {
		case GAME_PLAYER_1:
		case GAME_PLAYER_2:
			games[game_id]->pause |= player;
	}
	if (games[game_id]->pause && games[game_id]->update != GAME_UPDATE_FORBIDDEN) {
		games[game_id]->update = GAME_UPDATE_FORBIDDEN;
		send_message(game_id, MESSAGE_SERVER_GAME_PAUSED);
	}
	unlock_game(game_id);
}

void	move_paddle(const u8 game_id, const u8 player, const direction direction) {
	lock_game(game_id);
	switch (player) {
		case GAME_PLAYER_1:
			games[game_id]->p1_paddle.direction = direction;
			break ;
		case GAME_PLAYER_2:
			games[game_id]->p2_paddle.direction = direction;
			break ;
	}
	unlock_game(game_id);
}

static inline u8	_tick(state *game) {
	// GET START TIMESTAMP
	lock_game(game->game_id);
	// UPDATE STATE IF GAME_UPDATE_ALLOWED
	unlock_game(game->game_id);
	// GET END TIMESTAMP
	// SLEEP FOR 1S / TICKRATE - (END TIMESTAMP - START TIMESTAMP)
	return 1;
}
