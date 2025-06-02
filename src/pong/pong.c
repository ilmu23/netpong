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

#define _SECOND_NS		1000000000
#define _TICK_DURATION	_SECOND_NS / GAME_TICKRATE

pthread_mutex_t	state_locks[MAX_GAME_COUNT];
static state	*games[MAX_GAME_COUNT];

typedef struct timespec	tspec;

static inline void	_get_sleep_time(const tspec *start, const tspec *end, tspec *wait);
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
	games[game_id]->pause &= ~player;
	if (!games[game_id]->pause)
		games[game_id]->update = GAME_UPDATE_ALLOWED;
	unlock_game(game_id);
}

void	pause_game(const u8 game_id, const u8 player) {
	lock_game(game_id);
	games[game_id]->pause |= player;
	if (games[game_id]->update != GAME_UPDATE_FORBIDDEN) {
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

static inline void	_get_sleep_time(const tspec *start, const tspec *end, tspec *wait) {
	tspec	diff;

	diff = (tspec){
		.tv_sec = end->tv_sec - start->tv_sec,
		.tv_nsec = end->tv_nsec - start->tv_nsec
	};
	*wait = (tspec){
		.tv_sec = start->tv_sec + diff.tv_sec,
		.tv_nsec = start->tv_sec + _TICK_DURATION - diff.tv_nsec
	};
	while (wait->tv_nsec < 0) {
		wait->tv_nsec += _SECOND_NS;
		wait->tv_sec--;
	}
	while (wait->tv_nsec >= _SECOND_NS) {
		wait->tv_nsec -= _SECOND_NS;
		wait->tv_sec++;
	}
}

static inline u8	_tick(state *game) {
	tspec	start;
	tspec	wait;
	tspec	end;

	timespec_get(&start, TIME_UTC);
	lock_game(game->game_id);
	// simulated work
	usleep(rand_range(12, 780));
	unlock_game(game->game_id);
	timespec_get(&end, TIME_UTC);
	_get_sleep_time(&start, &end, &wait);
	debug("Game %hhu: Tick done, sleeping until %lld:%lld", game->game_id, wait.tv_sec, wait.tv_sec);
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wait, NULL);
	return 1;
}
