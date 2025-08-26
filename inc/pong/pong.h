// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<pong.h>>

#pragma once

#include "data.h"
#include "server/server.h"

#include <math.h>
#include <time.h>
#include <pthread.h>

#define GAME_PLAYER_1	0x1U
#define GAME_PLAYER_2	0x2U

#define GAME_TICKRATE	1000U

#define GAME_FIELD_WIDTH	40.0f
#define GAME_FIELD_HEIGHT	20.0f

#define	GAME_BALL_RADIUS	0.5f
#define GAME_PADDLE_HEIGHT	1.5f

#define GAME_BALL_ANGLE_MAX			40.0f
#define GAME_BALL_ANGLE_MOVE_BOOST	5.0f
#define GAME_BALL_ANGLE_MULTIPLIER	(GAME_BALL_ANGLE_MAX / GAME_PADDLE_HEIGHT)
#define GAME_BALL_ANGLE_ABS_MAX		(GAME_BALL_ANGLE_MAX + GAME_BALL_ANGLE_MOVE_BOOST)

#define GAME_BALL_SPEED		0.025f
#define GAME_PADDLE_SPEED	0.0125f

#define GAME_SCORE_MAX	11

#define GAME_UPDATE_ALLOWED			0x1
#define GAME_UPDATE_SCORE_PENDING	0x2

void	*pong(void *arg);

void	unpause_game(game *game, const u8 player);
void	pause_game(game *game, const u8 player);

void	move_paddle(game *game, const u8 player, const direction direction);

void	quit_game(game *game, const u8 player);
