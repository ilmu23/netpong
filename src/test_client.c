// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<test_client.c>>

#include "netpong.h"
#include <termios.h>

static const struct addrinfo	hints = {
	.ai_family = AF_INET,
	.ai_socktype = SOCK_STREAM,
	.ai_flags = 0,
	.ai_protocol = 0,
	.ai_addrlen = 0,
	.ai_addr = NULL,
	.ai_canonname = NULL,
	.ai_next = NULL
};

static const char	*type_strs[] = {"Game init", "Game paused", "Game over", "State update"};

static struct termios	old_tios;
static struct termios	new_tios;

static i32	p1fd;
static i32	p2fd;
static u8	_pause;

static struct {
	pthread_mutex_t	lock;
	u8				updated;
	u8				p1;
	u8				p2;
}	score = {.lock = PTHREAD_MUTEX_INITIALIZER, .updated = 0, .p1 = 0, .p2 = 0};

#define get_paddle_direction_code(d)	((d == UP) ? MESSAGE_CLIENT_MOVE_PADDLE_UP : (d == DOWN) ? MESSAGE_CLIENT_MOVE_PADDLE_DOWN : MESSAGE_CLIENT_MOVE_PADDLE_STOP)

static inline void	_receive_msg(const i32 sfd, u16 ports[2]);
static inline void	_send_msg(const i32 sfd, const u8 msg_type, const u8 body[18]);
static inline i32	_connect_player(const char *addr, const u16 port);
static inline i32	_connect(const char *addr, const char *port);

static void	*_player_input([[gnu::unused]] void *arg);
static void	_restore_term_settings(void);

i32	main(i32 ac, char **av) {
	struct epoll_event	events[3];
	struct epoll_event	ev;
	pthread_t			input_thread;
	i32					ev_count;
	i32					efd;
	i32					sfd;
	i32					i;
	u16					ports[2];
	u8					c;

	if (ac != 3)
		return 1;
	if (tcgetattr(0, &old_tios) == -1 || atexit(_restore_term_settings) == -1)
		die(strerror(errno));
	new_tios = old_tios;
	new_tios.c_iflag &= ~(ICRNL | IXON);
	new_tios.c_lflag &= ~(ECHO | ICANON | IEXTEN);
	if (tcsetattr(0, TCSANOW, &new_tios) == -1)
		die(strerror(errno));
	sfd = _connect(av[1], av[2]);
	efd = epoll_create(1);
	if (efd == -1)
		die("epoll_create");
	ev = (struct epoll_event){
		.events = EPOLLIN,
		.data.fd = sfd
	};
	if (epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &ev) == -1)
		die("epoll_ctl");
	_receive_msg(sfd, ports);
	fprintf(stderr, SGR_DEBUG "Press return to connect player 1 " SGR_RESET);
	(void)read(0, &c, 1);
	p1fd = _connect_player(av[1], ports[0]);
	fprintf(stderr, SGR_DEBUG "\nPress return to connect player 2 " SGR_RESET);
	(void)read(0, &c, 1);
	p2fd = _connect_player(av[1], ports[1]);
	fprintf(stderr, SGR_DEBUG "\nPress return to unpause player 2 " SGR_RESET);
	(void)read(0, &c, 1);
	_send_msg(p2fd, MESSAGE_CLIENT_START, NULL);
	fprintf(stderr, SGR_DEBUG "\nPress return to unpause player 1 " SGR_RESET);
	(void)read(0, &c, 1);
	fputc('\n', stderr);
	_send_msg(p1fd, MESSAGE_CLIENT_START, NULL);
	if (pthread_create(&input_thread, NULL, _player_input, NULL) != 0)
		die("pthread_create failed");
	while (1) {
		ev_count = epoll_wait(efd, events, 3, -1);
		for (i = 0; i < ev_count; i++)
			_receive_msg(events[i].data.fd, NULL);
	}
	return 0;
}

static inline void	_receive_msg(const i32 sfd, u16 ports[2]) {
	union {
		msg_srv_init		*init;
		msg_srv_state		*state;
		msg_srv_game_over	*over;
	}		body;
	struct {
		u8	p1;
		u8	p2;
	}		_score;
	message	msg;
	ssize_t	rv;

	rv = recv(sfd, &msg, MESSAGE_HEADER_SIZE, MSG_WAITALL);
	if (rv != MESSAGE_HEADER_SIZE)
		die("message header");
	info("Received message\n\tProtocol version: %hhu\n\tMessage type: %s\n\tBody length: %hu", msg.version, type_strs[msg.type], msg.length);
	if (msg.length) {
		rv = recv(sfd, msg.body, msg.length, MSG_WAITALL);
		if (rv != msg.length)
			die("message body");
		switch (msg.type) {
			case MESSAGE_SERVER_GAME_INIT:
				body.init = (msg_srv_init *)msg.body;
				ports[0] = body.init->p1_port;
				ports[1] = body.init->p2_port;
				info("\tPlayer 1 port: %hu\n\tPlayer 2 port: %hu", body.init->p1_port, body.init->p2_port);
				break ;
			case MESSAGE_SERVER_GAME_OVER:
				body.over = (msg_srv_game_over *)msg.body;
				switch (body.over->finish_status) {
					case MESSAGE_SERVER_GAME_OVER_ACT_WON:
						info("\tPlayer %hhu won", body.over->actor_id);
						break ;
					case MESSAGE_SERVER_GAME_OVER_ACT_QUIT:
						info("\tPlayer %hhu quit", body.over->actor_id);
						break ;
					case MESSAGE_SERVER_GAME_OVER_SERVER_CLOSED:
						info("\tServer closed");
						exit(0);
				}
				pthread_mutex_lock(&score.lock);
				score.p1 = 0;
				score.p2 = 0;
				score.updated = 1;
				_pause = GAME_PLAYER_1 | GAME_PLAYER_2;
				pthread_mutex_unlock(&score.lock);
				break ;
			case MESSAGE_SERVER_STATE_UPDATE:
				body.state = (msg_srv_state *)msg.body;
				_score.p1 = body.state->score >> 8 & 0xFF;
				_score.p2 = body.state->score & 0xFF;
				pthread_mutex_lock(&score.lock);
				if (_score.p1 != score.p1 || _score.p2 != score.p2) {
					score.p1 = _score.p1;
					score.p2 = _score.p2;
					score.updated = 1;
				}
				pthread_mutex_unlock(&score.lock);
				info("\tPlayer 1 paddle position: %f\n\tPlayer 2 paddle position: %f", body.state->p1_paddle, body.state->p2_paddle);
				info("\tBall position: %f,%f", body.state->ball.x, body.state->ball.y);
				info("\tScore: %hhu - %hhu", body.state->score >> 8 & 0xFF, body.state->score & 0xFF);
				break ;
		}
	}
}

static inline void	_send_msg(const i32 sfd, const u8 msg_type, const u8 body[18]) {
	message	msg;

	msg = (message){
		.version = PROTOCOL_VERSION,
		.type = msg_type
	};
	switch (msg.type) {
		case MESSAGE_CLIENT_START:
		case MESSAGE_CLIENT_PAUSE:
		case MESSAGE_CLIENT_QUIT:
			msg.length = 0;
			break ;
		case MESSAGE_CLIENT_MOVE_PADDLE:
			msg.length = 1;
			msg.body[0] = body[0];
	}
	if (send(sfd, &msg, MESSAGE_HEADER_SIZE + msg.length, 0) == -1)
		die(strerror(errno));
}

static inline i32	_connect_player(const char *addr, const u16 port) {
	size_t	i;
	char	buf[6];
	u16		_port;

	for (i = 1, _port = port; _port > 9; _port /= 10, i++)
		;
	buf[i] = '\0';
	for (_port = port; i; _port /= 10)
		buf[--i] = _port % 10 + '0';
	return _connect(addr, buf);
}

static inline i32	_connect(const char *addr, const char *port) {
	struct addrinfo	*addresses;
	struct addrinfo	*cur_address;
	i32				out;

	if (getaddrinfo(addr, port, &hints, &addresses) != 0)
		exit(1);
	for (cur_address = addresses; cur_address != NULL; cur_address = cur_address->ai_next) {
		out = socket(cur_address->ai_family, cur_address->ai_socktype, cur_address->ai_protocol);
		if (out) {
			if (connect(out, cur_address->ai_addr, cur_address->ai_addrlen) != -1)
				break ;
			close(out);
		}
	}
	freeaddrinfo(addresses);
	if (cur_address == NULL)
		exit(1);
	return out;
}

static void	*_player_input([[gnu::unused]] void *arg) {
	struct {
		direction	p1_direction;
		direction	p2_direction;
	}	state;
	i8	c;

	state.p1_direction = STOP;
	state.p2_direction = STOP;
	_pause = 0;
	while (1) {
		read(0, &c, sizeof(c));
		pthread_mutex_lock(&score.lock);
		if (score.updated) {
			state.p1_direction = STOP;
			state.p2_direction = STOP;
			score.updated = 0;
		}
		pthread_mutex_unlock(&score.lock);
		if (c == 'w' || c == 's') {
			if (c == 'w')
				state.p1_direction = (state.p1_direction != UP) ? UP : STOP;
			else
				state.p1_direction = (state.p1_direction != DOWN) ? DOWN : STOP;
			_send_msg(p1fd, MESSAGE_CLIENT_MOVE_PADDLE, &(u8){get_paddle_direction_code(state.p1_direction)});
		} else if (c == 'k' || c == 'j') {
			if (c == 'k')
				state.p2_direction = (state.p2_direction != UP) ? UP : STOP;
			else
				state.p2_direction = (state.p2_direction != DOWN) ? DOWN : STOP;
			_send_msg(p2fd, MESSAGE_CLIENT_MOVE_PADDLE, &(u8){get_paddle_direction_code(state.p2_direction)});
		} else if (c == 'p') {
			_pause ^= GAME_PLAYER_1;
			_send_msg(p1fd, (_pause & GAME_PLAYER_1) ? MESSAGE_CLIENT_PAUSE : MESSAGE_CLIENT_START, NULL);
		} else if (c == 'P') {
			_pause ^= GAME_PLAYER_2;
			_send_msg(p2fd, (_pause & GAME_PLAYER_2) ? MESSAGE_CLIENT_PAUSE : MESSAGE_CLIENT_START, NULL);
		} else if (c == 'q') {
			_send_msg(p1fd, MESSAGE_CLIENT_QUIT, NULL);
			break ;
		} else if (c == 'Q') {
			_send_msg(p2fd, MESSAGE_CLIENT_QUIT, NULL);
			break ;
		}
	}
	return NULL;
}

static void	_restore_term_settings(void) {
	tcsetattr(0, TCSANOW, &old_tios);
}
