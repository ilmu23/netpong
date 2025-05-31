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

#include <pthread.h>

#define GAME_PLAYER_1	0x1U
#define GAME_PLAYER_2	0x2U

#define GAME_FIELD_WIDTH	80.0f
#define GAME_FIELD_HEIGHT	20.0f

#define GAME_UPDATE_ALLOWED		1
#define GAME_UPDATE_FORBIDDEN	0

void	*pong(void *arg);

void	unpause_game(const u8 game_id, const u8 player);
void	pause_game(const u8 game_id, const u8 player);

void	move_paddle(const u8 game_id, const u8 player, const direction direction);
