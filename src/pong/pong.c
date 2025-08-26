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

static const char	*direction_strings[] = {"Up", "Down", "Left", "Right", "Stop"};

static inline void	_move_paddles(game *game);
static inline void	_move_ball(game *game);
static inline void	_hit_ball(game *game, const u8 player);
static inline void	_end_game(game *game, const u8 winner);
static inline void	_tick(game *game);

void	*pong(void *arg) {
	struct timespec	start;
	game			*_game;
	u8				quit;

	_game = (game *)arg;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
		die(strerror(errno));
	for (quit = 0; !quit; check_quit(_game->state, quit)) {
		_tick(_game);
		start.tv_nsec += _TICK_DURATION;
		if (start.tv_nsec >= _SECOND_NS) {
			start.tv_nsec -= _SECOND_NS;
			start.tv_sec++;
		}
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &start, NULL);
	}
	return NULL;
}

void	unpause_game(game *game, const u8 player) {
	debug("Game %hhu: Player %hhu unpaused", game->id, player);
	lock_game(game->state);
	game->state.pause &= ~player;
	if (!game->state.pause) {
		game->state.update ^= GAME_UPDATE_ALLOWED;
		if (!game->state.started)
			game->state.started = 1;
	}
	unlock_game(game->state);
}

void	pause_game(game *game, const u8 player) {
	debug("Game %hhu: Player %hhu paused", game->id, player);
	lock_game(game->state);
	game->state.pause |= player;
	if (game->state.update & GAME_UPDATE_ALLOWED) {
		game->state.update ^= GAME_UPDATE_ALLOWED;
		send_message(game, MESSAGE_SERVER_GAME_PAUSED, 0);
	}
	unlock_game(game->state);
}

void	move_paddle(game *game, const u8 player, const direction direction) {
	debug("Game %hhu: Player %hhu changed paddle direction: %s", game->id, player, direction_strings[direction]);
	lock_game(game->state);
	switch (player) {
		case GAME_PLAYER_1:
			game->state.p1_paddle.direction = direction;
			break ;
		case GAME_PLAYER_2:
			game->state.p2_paddle.direction = direction;
			break ;
	}
	unlock_game(game->state);
}

void	quit_game(game *game, const u8 player) {
	debug("Game %hhu: Player %hhu quit", game->id, player);
	lock_game(game->state);
	if (!player)
		send_message(game, MESSAGE_SERVER_GAME_OVER, (MESSAGE_SERVER_GAME_OVER_SERVER_CLOSED << 8) | 0);
	else if (game->state.started)
		send_message(game, MESSAGE_SERVER_GAME_OVER, (MESSAGE_SERVER_GAME_OVER_ACT_QUIT << 8) | player);
	game->state.quit = 1;
	unlock_game(game->state);
}

static inline void	_move_paddles(game *game) {
	switch (game->state.p1_paddle.direction) {
		case UP:
			game->state.p1_paddle.pos += GAME_PADDLE_SPEED;
			if (game->state.p1_paddle.pos + GAME_PADDLE_HEIGHT >= GAME_FIELD_HEIGHT) {
				game->state.p1_paddle.pos = GAME_FIELD_HEIGHT - GAME_PADDLE_HEIGHT;
				game->state.p1_paddle.direction = STOP;
			}
			break ;
		case DOWN:
			game->state.p1_paddle.pos -= GAME_PADDLE_SPEED;
			if (game->state.p1_paddle.pos - GAME_PADDLE_HEIGHT <= 0.0f) {
				game->state.p1_paddle.pos = GAME_PADDLE_HEIGHT;
				game->state.p1_paddle.direction = STOP;
			}
			break ;
		default:
			break ;
	}
	switch (game->state.p2_paddle.direction) {
		case UP:
			game->state.p2_paddle.pos += GAME_PADDLE_SPEED;
			if (game->state.p2_paddle.pos + GAME_PADDLE_HEIGHT >= GAME_FIELD_HEIGHT) {
				game->state.p2_paddle.pos = GAME_FIELD_HEIGHT - GAME_PADDLE_HEIGHT;
				game->state.p2_paddle.direction = STOP;
			}
			break ;
		case DOWN:
			game->state.p2_paddle.pos -= GAME_PADDLE_SPEED;
			if (game->state.p2_paddle.pos - GAME_PADDLE_HEIGHT <= 0.0f) {
				game->state.p2_paddle.pos = GAME_PADDLE_HEIGHT;
				game->state.p2_paddle.direction = STOP;
			}
			break ;
		default:
			break ;
	}
}

static inline void	_move_ball(game *game) {
	f32	angle_rad;
	f32	dx;
	f32	dy;

	if (game->state.ball.angle >= 0.1 || game->state.ball.angle <= -0.1) {
		angle_rad = fabsf(game->state.ball.angle) * M_PI * 2 / 360;
		dx = cosf(angle_rad) * GAME_BALL_SPEED;
		dy = sinf(angle_rad) * GAME_BALL_SPEED;
		if (game->state.ball.direction == LEFT)
			dx = -dx;
		if (game->state.ball.angle < 0.0f)
			dy = -dy;
	} else {
		dx = (game->state.ball.direction == LEFT) ? -GAME_BALL_SPEED : GAME_BALL_SPEED;
		dy = 0;
	}
	game->state.ball.x += dx;
	game->state.ball.y += dy;
	if (game->state.ball.y + GAME_BALL_RADIUS >= GAME_FIELD_HEIGHT) {
		game->state.ball.y = GAME_FIELD_HEIGHT - GAME_BALL_RADIUS;
		game->state.ball.angle = -game->state.ball.angle;
	} else if (game->state.ball.y - GAME_BALL_RADIUS <= 0.0f) {
		game->state.ball.y = GAME_BALL_RADIUS;
		game->state.ball.angle = -game->state.ball.angle;
	}
	if (game->state.ball.x + GAME_BALL_RADIUS >= GAME_FIELD_WIDTH)
		_hit_ball(game, GAME_PLAYER_2);
	else if (game->state.ball.x - GAME_BALL_RADIUS <= 0.0f)
		_hit_ball(game, GAME_PLAYER_1);
}

static inline void	_hit_ball(game *game, const u8 player) {
	f32	paddle_pos;

	paddle_pos = (player == GAME_PLAYER_1) ? game->state.p1_paddle.pos : game->state.p2_paddle.pos;
	if (game->state.ball.y >= paddle_pos - GAME_PADDLE_HEIGHT && game->state.ball.y <= paddle_pos + GAME_PADDLE_HEIGHT) {
		switch (player) {
			case GAME_PLAYER_1:
				game->state.ball.x = GAME_BALL_RADIUS;
				game->state.ball.direction = RIGHT;
				game->state.ball.angle = (game->state.ball.y - paddle_pos) * GAME_BALL_ANGLE_MULTIPLIER + paddle_boost(game->state.p1_paddle);
				break ;
			case GAME_PLAYER_2:
				game->state.ball.x = GAME_FIELD_WIDTH - GAME_BALL_RADIUS;
				game->state.ball.direction = LEFT;
				game->state.ball.angle = (game->state.ball.y - paddle_pos) * GAME_BALL_ANGLE_MULTIPLIER + paddle_boost(game->state.p2_paddle);
		}
	} else {
		game->state.update ^= GAME_UPDATE_SCORE_PENDING;
		game->state.p1_paddle.pos = GAME_FIELD_HEIGHT / 2;
		game->state.p2_paddle.pos = GAME_FIELD_HEIGHT / 2;
		game->state.ball.x = GAME_FIELD_WIDTH / 2;
		game->state.ball.y = GAME_FIELD_HEIGHT / 2;
		game->state.ball.angle = 0.0f;
		switch (player) {
			case GAME_PLAYER_1:
				game->state.ball.direction = LEFT;
				if (++game->state.score.p2 == GAME_SCORE_MAX)
					_end_game(game, GAME_PLAYER_2);
				break ;
			case GAME_PLAYER_2:
				game->state.ball.direction = RIGHT;
				if (++game->state.score.p1 == GAME_SCORE_MAX)
					_end_game(game, GAME_PLAYER_1);
		}
	}
}

static inline void	_end_game(game *game, const u8 winner) {
	send_message(game, MESSAGE_SERVER_STATE_UPDATE, 0);
	send_message(game, MESSAGE_SERVER_GAME_OVER, (MESSAGE_SERVER_GAME_OVER_ACT_WON << 8) | winner);
	game->state.p1_paddle.pos = GAME_FIELD_HEIGHT / 2;
	game->state.p2_paddle.pos = GAME_FIELD_HEIGHT / 2;
	game->state.p1_paddle.direction = STOP;
	game->state.p2_paddle.direction = STOP;
	game->state.ball.x = GAME_FIELD_WIDTH / 2;
	game->state.ball.y = GAME_FIELD_HEIGHT / 2;
	game->state.ball.angle = 0.0f;
	game->state.ball.direction = (rand() & 1) ? LEFT : RIGHT;
	game->state.score.p1 = 0;
	game->state.score.p2 = 0;
	game->state.started = 0;
	game->state.update = 0;
	game->state.pause = GAME_PLAYER_1 | GAME_PLAYER_2;
	game->state.quit = 0;
}

static inline void	_tick(game *game) {
	lock_game(game->state);
	if (game->state.update & GAME_UPDATE_ALLOWED) {
		if (!(game->state.update & GAME_UPDATE_SCORE_PENDING)) {
			_move_paddles(game);
			_move_ball(game);
		} else
			debug("Game %hhu: waiting to send score update", game->id);
	}
	unlock_game(game->state);
}
