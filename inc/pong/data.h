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

typedef enum __direction {
	UP = 0,
	DOWN = 1,
	LEFT = 2,
	RIGHT = 3,
	STOP = 4
}	direction;

typedef struct __state {
	struct {
		f32			pos;
		direction	direction;
	}	p1_paddle;
	struct {
		f32			pos;
		direction	direction;
	}	p2_paddle;
	struct {
		f32			x;
		f32			y;
		f32			angle;
		direction	direction;
	}	ball;
	struct {
		u8	p1;
		u8	p2;
	}	score;
	u8	game_id;
	u8	started;
	u8	update;
	u8	pause;
	u8	quit;
}	state;
