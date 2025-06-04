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

#define _SECOND_NS		1000000000U
#define _TICK_DURATION	_SECOND_NS / GAME_TICKRATE

#define paddle_boost(paddle)	((paddle.direction == UP) ? GAME_BALL_ANGLE_MOVE_BOOST : (paddle.direction == DOWN) ? -GAME_BALL_ANGLE_MOVE_BOOST : 0)

pthread_mutex_t	state_locks[MAX_GAME_COUNT];
static state	*games[MAX_GAME_COUNT];

static const char	*direction_strings[] = {
					"Up",
					"Down",
					"Left",
					"Right",
					"Stop"
};

static inline void	_move_paddles(state *game);
static inline void	_move_ball(state *game);
static inline void	_hit_ball(state *game, const u8 player);
static inline void	_end_game(state *game, const u8 winner);
static inline void	_tick(state *game);

void	*pong(void *arg) {
	struct timespec	start;
	u8				game_id;
	u8				quit;

	game_id =((state *)arg)->game_id;
	games[game_id] = arg;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
		die(strerror(errno));
	for(quit = 0; !quit; check_quit(game_id, games[game_id]->quit, quit)) {
		_tick(games[game_id]);
		start.tv_nsec += _TICK_DURATION;
		if (start.tv_nsec >= _SECOND_NS) {
			start.tv_nsec -= _SECOND_NS;
			start.tv_sec++;
		}
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &start, NULL);
	}
	return NULL;
}

void	unpause_game(const u8 game_id, const u8 player) {
	debug("Game %hhu: Player %hhu unpaused", game_id, player);
	lock_game(game_id);
	games[game_id]->pause &= ~player;
	if (!games[game_id]->pause) {
		games[game_id]->update ^= GAME_UPDATE_ALLOWED;
		if (!games[game_id]->started)
			games[game_id]->started = 1;
	}
	unlock_game(game_id);
}

void	pause_game(const u8 game_id, const u8 player) {
	debug("Game %hhu: Player %hhu paused", game_id, player);
	lock_game(game_id);
	games[game_id]->pause |= player;
	if (games[game_id]->update & GAME_UPDATE_ALLOWED) {
		games[game_id]->update ^= GAME_UPDATE_ALLOWED;
		send_message(game_id, MESSAGE_SERVER_GAME_PAUSED, 0);
	}
	unlock_game(game_id);
}

void	move_paddle(const u8 game_id, const u8 player, const direction direction) {
	debug("Game %hhu: Player %hhu changed paddle direction: %s", game_id, player, direction_strings[direction]);
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

void	quit_game(const u8 game_id, const u8 player) {
	debug("Game %hhu: Player %hhu quit", game_id, player);
	lock_game(game_id);
	if (!player)
		send_message(game_id, MESSAGE_SERVER_GAME_OVER, MESSAGE_SERVER_GAME_OVER_SERVER_CLOSED << 8 | 0);
	else if (games[game_id]->started)
		send_message(game_id, MESSAGE_SERVER_GAME_OVER, (MESSAGE_SERVER_GAME_OVER_ACT_QUIT << 8) | player);
	games[game_id]->quit = 1;
	unlock_game(game_id);
}

static inline void	_move_paddles(state *game) {
	switch (game->p1_paddle.direction) {
		case UP:
			game->p1_paddle.pos += GAME_PADDLE_SPEED;
			if (game->p1_paddle.pos + GAME_PADDLE_HEIGHT >= GAME_FIELD_HEIGHT) {
				game->p1_paddle.pos = GAME_FIELD_HEIGHT - GAME_PADDLE_HEIGHT;
				game->p1_paddle.direction = STOP;
			}
			break ;
		case DOWN:
			game->p1_paddle.pos -= GAME_PADDLE_SPEED;
			if (game->p1_paddle.pos - GAME_PADDLE_HEIGHT <= 0.0f) {
				game->p1_paddle.pos = GAME_PADDLE_HEIGHT;
				game->p1_paddle.direction = STOP;
			}
			break ;
		default:
			break ;
	}
	switch (game->p2_paddle.direction) {
		case UP:
			game->p2_paddle.pos += GAME_PADDLE_SPEED;
			if (game->p2_paddle.pos + GAME_PADDLE_HEIGHT >= GAME_FIELD_HEIGHT) {
				game->p2_paddle.pos = GAME_FIELD_HEIGHT - GAME_PADDLE_HEIGHT;
				game->p2_paddle.direction = STOP;
			}
			break ;
		case DOWN:
			game->p2_paddle.pos -= GAME_PADDLE_SPEED;
			if (game->p2_paddle.pos - GAME_PADDLE_HEIGHT <= 0.0f) {
				game->p2_paddle.pos = GAME_PADDLE_HEIGHT;
				game->p2_paddle.direction = STOP;
			}
			break ;
		default:
			break ;
	}
}

static inline void	_move_ball(state *game) {
	struct {
		struct {
			f32	a;
			f32	b;
			f32	c;
		}	sides;
		struct {
			f32	a;
			f32	b;
			f32	c;
		}	angles;
	}	triangle;
	f32	dx;
	f32	dy;

	if (game->ball.angle >= 0.1 || game->ball.angle <= -0.1) {
		triangle.angles.c = 90.0f;
		triangle.angles.b = fabsf(game->ball.angle);
		triangle.angles.a = 90.0f - triangle.angles.b;
		triangle.sides.c = GAME_BALL_SPEED;
		triangle.sides.b = triangle.sides.c * (sin(triangle.angles.b) / sin(triangle.angles.c));
		triangle.sides.a = triangle.sides.c * (sin(triangle.angles.a) / sin(triangle.angles.c));
		dx = (game->ball.direction == LEFT) ? -triangle.sides.a : triangle.sides.a;
		dy = (game->ball.angle < 0.0f) ? -triangle.sides.b : triangle.sides.b;
	} else {
		dx = (game->ball.direction == LEFT) ? -GAME_BALL_SPEED : GAME_BALL_SPEED;
		dy = 0;
	}
	game->ball.x += dx;
	game->ball.y += dy;
	if (game->ball.y + GAME_BALL_RADIUS >= GAME_FIELD_HEIGHT) {
		game->ball.y = GAME_FIELD_HEIGHT - GAME_BALL_RADIUS;
		game->ball.angle = -game->ball.angle;
	} else if (game->ball.y - GAME_BALL_RADIUS <= 0.0f) {
		game->ball.y = GAME_BALL_RADIUS;
		game->ball.angle = -game->ball.angle;
	}
	if (game->ball.x + GAME_BALL_RADIUS >= GAME_FIELD_WIDTH)
		_hit_ball(game, GAME_PLAYER_2);
	else if (game->ball.x - GAME_BALL_RADIUS <= 0.0f)
		_hit_ball(game, GAME_PLAYER_1);
}

static inline void	_hit_ball(state *game, const u8 player) {
	f32	paddle_pos;

	paddle_pos = (player == GAME_PLAYER_1) ? game->p1_paddle.pos : game->p2_paddle.pos;
	if (game->ball.y >= paddle_pos - GAME_PADDLE_HEIGHT && game->ball.y <= paddle_pos + GAME_PADDLE_HEIGHT) {
		switch (player) {
			case GAME_PLAYER_1:
				game->ball.x = GAME_BALL_RADIUS;
				game->ball.direction = RIGHT;
				game->ball.angle = fabsf(game->ball.y - paddle_pos) * GAME_BALL_ANGLE_MULTIPLIER + paddle_boost(game->p1_paddle);
				break ;
			case GAME_PLAYER_2:
				game->ball.x = GAME_FIELD_WIDTH - GAME_BALL_RADIUS;
				game->ball.direction = LEFT;
				game->ball.angle = fabsf(game->ball.y - paddle_pos) * GAME_BALL_ANGLE_MULTIPLIER + paddle_boost(game->p1_paddle);
		}
	} else {
		game->update ^= GAME_UPDATE_SCORE_PENDING;
		game->ball.x = GAME_FIELD_WIDTH / 2;
		game->ball.y = GAME_FIELD_HEIGHT / 2;
		game->ball.angle = 0.0f;
		switch (player) {
			case GAME_PLAYER_1:
				game->ball.direction = LEFT;
				if (++game->score.p2 == GAME_SCORE_MAX)
					_end_game(game, GAME_PLAYER_2);
				break ;
			case GAME_PLAYER_2:
				game->ball.direction = RIGHT;
				if (++game->score.p1 == GAME_SCORE_MAX)
					_end_game(game, GAME_PLAYER_1);
		}
	}
}

static inline void	_end_game(state *game, const u8 winner) {
	send_message(game->game_id, MESSAGE_SERVER_STATE_UPDATE, 0);
	send_message(game->game_id, MESSAGE_SERVER_GAME_OVER, (MESSAGE_SERVER_GAME_OVER_SERVER_CLOSED << 8) | winner);
	*game = (state){
		.p1_paddle.pos = GAME_FIELD_HEIGHT / 2,
		.p2_paddle.pos = GAME_FIELD_HEIGHT / 2,
		.p1_paddle.direction = STOP,
		.p2_paddle.direction = STOP,
		.ball.x = GAME_FIELD_WIDTH / 2,
		.ball.y = GAME_FIELD_HEIGHT / 2,
		.ball.angle = 0.0f,
		.ball.direction = (rand() & 1) ? LEFT : RIGHT,
		.score.p1 = 0,
		.score.p2 = 0,
		.game_id = game->game_id,
		.started = 0,
		.update = 0,
		.pause = GAME_PLAYER_1 | GAME_PLAYER_2,
		.quit = 0
	};
}

static inline void	_tick(state *game) {
	lock_game(game->game_id);
	if (game->update & GAME_UPDATE_ALLOWED) {
		if (!(game->update & GAME_UPDATE_SCORE_PENDING)) {
			_move_paddles(game);
			_move_ball(game);
		} else
			debug("Game %hhu: waiting to send score update", game->game_id);
	}
	unlock_game(game->game_id);
}
