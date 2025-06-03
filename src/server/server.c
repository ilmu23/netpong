// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<server.c>>

#include "pong/pong.h"
#include "server/server.h"

static pthread_t	t0;
static size_t		game_count;
static game			games[MAX_GAME_COUNT];
static const char	*address;

extern pthread_mutex_t	state_locks[MAX_GAME_COUNT];

static const struct addrinfo	hints = {
	.ai_family = AF_INET,
	.ai_socktype = SOCK_STREAM,
	.ai_flags = AI_PASSIVE,
	.ai_protocol = 0,
	.ai_canonname = NULL,
	.ai_addr = NULL,
	.ai_next = NULL
};

static const char	*server_message_type_strings[] = {"Game init", "Game paused", "Game over", "State update"};

#define set_non_blocking(fd)	(fcntl(fd, F_SETFL, O_NONBLOCK))

#define INET_N1(a)	((u8)(ntohl(a) >> 24 & 0xFF))
#define INET_N2(a)	((u8)(ntohl(a) >> 16 & 0xFF))
#define INET_N3(a)	((u8)(ntohl(a) >> 8 & 0xFF))
#define INET_N4(a)	((u8)(ntohl(a) & 0xFF))

static inline void	_host_game(const i32 cfd);
static inline void	_get_message(message *message_buf, const i32 fd);
static inline void	_assign_ports(const size_t game_id);
static inline i32	_open_socket(const char *port);
static inline i32	_connect_client(const i32 lfd);
static void			*_player_io(void *arg);
static void			*_run_game(void *arg);

[[gnu::noreturn]] void	run_server(const char *addr, const char *port) {
	i32					sfd;
	i32					cfd;

	address = addr;
	t0 = pthread_self();
	sfd = _open_socket(port);
	if (listen(sfd, MAX_LISTEN_BACKLOG) == -1)
		die(strerror(errno));
	info("Server initialized and listening on %s:%s", address, port);
	while (1) {
		cfd = _connect_client(sfd);
		if (cfd)
			_host_game(cfd);
		else if (errno == EAGAIN || errno == EWOULDBLOCK)
			errno = 0;
		else
			die(strerror(errno));
	}
	// CLEANUP GOES HERE
	exit(0);
}

void	send_message(const u8 game_id, const u8 message_type) {
	message	msg;
	state	*game;

	msg.version = PROTOCOL_VERSION;
	msg.type = message_type;
	debug("Game %zu: Sending message: %s", game_id, server_message_type_strings[message_type]);
	switch (message_type) {
		case MESSAGE_SERVER_GAME_INIT:
			msg.length = sizeof(msg_srv_init);
			*(msg_srv_init *)msg.body = _msg_srv_init(games[game_id].ports.p1, games[game_id].ports.p2);
			break ;
		case MESSAGE_SERVER_GAME_PAUSED:
			msg.length = 0;
			break ;
		case MESSAGE_SERVER_GAME_OVER:
			msg.length = 1;
			msg.body[0] = (games[game_id].state.score.p1 == GAME_SCORE_MAX) ? 1 : 2;
			info("Game %hhu: Game over, winner: Player %hhu", game_id, msg.body[0]);
			break ;
		case MESSAGE_SERVER_STATE_UPDATE:
			game = &games[game_id].state;
			msg.length = sizeof(msg_srv_state);
			*(msg_srv_state *)msg.body = _msg_srv_state(game->p1_paddle.pos, game->p2_paddle.pos, game->ball.x, game->ball.y, game->score);
	}
	if (send(games[game_id].sockets.state, &msg, MESSAGE_HEADER_SIZE + msg.length, 0) == -1)
		die(strerror(errno));
}

static inline void	_host_game(const i32 cfd) {
	size_t	i;

	if (game_count == MAX_GAME_COUNT) {
		close(cfd);
		return ;
	}
	for (i = 0; i < MAX_GAME_COUNT && games[i].threads.game_master; i++)
		;
	if (i == MAX_GAME_COUNT) {
		close(cfd);
		return ;
	}
	games[i].sockets.state = cfd;
	if (pthread_create(&games[i].threads.game_master, NULL, _run_game, (void *)(uintptr_t)i) != 0)
		die(strerror(errno));
}

static inline void	_get_message(message *message_buf, const i32 fd) {
	ssize_t	rv;

	rv = recv(fd, message_buf, MESSAGE_HEADER_SIZE, MSG_WAITALL);
	if (rv != MESSAGE_HEADER_SIZE)
		die(strerror(errno));
	if (message_buf->length) {
		rv = recv(fd, message_buf->body, message_buf->length, MSG_WAITALL);
		if (rv != message_buf->length)
			die(strerror(errno));
	}
}

static inline void	_assign_ports(const size_t game_id) {
	struct epoll_event	events[2];
	struct sockaddr_in	addr;
	socklen_t			addr_length;
	i32					event_count;
	i32					p1_sfd;
	i32					p2_sfd;
	i32					i;

	debug("Game %zu: Starting player port assignment", game_id);
	p1_sfd = _open_socket(NULL);
	p2_sfd = _open_socket(NULL);
	if (listen(p1_sfd, MAX_LISTEN_BACKLOG) == -1 || listen(p2_sfd, MAX_LISTEN_BACKLOG) ||
		set_non_blocking(p1_sfd) == -1 || set_non_blocking(p2_sfd) == -1)
		die(strerror(errno));
	addr_length = sizeof(addr);
	if (getsockname(p1_sfd, (struct sockaddr *)&addr, &addr_length) == -1)
		die(strerror(errno));
	games[game_id].ports.p1 = ntohs(addr.sin_port);
	addr_length = sizeof(addr);
	if (getsockname(p2_sfd, (struct sockaddr *)&addr, &addr_length) == -1)
		die(strerror(errno));
	games[game_id].ports.p2 = ntohs(addr.sin_port);
	events[0] = (struct epoll_event){
		.events = EPOLLIN,
		.data.fd = p1_sfd
	};
	if (epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_ADD, p1_sfd, &events[0]) == -1)
		die(strerror(errno));
	events[0].data.fd = p2_sfd;
	if (epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_ADD, p2_sfd, &events[0]) == -1)
		die(strerror(errno));
	info("Game %zu: Assigning ports for players:\n\tplayer 1: %hu\n\tplayer 2: %hu", game_id, games[game_id].ports.p1, games[game_id].ports.p2);
	send_message(game_id, MESSAGE_SERVER_GAME_INIT);
	games[game_id].sockets.p1 = -1;
	games[game_id].sockets.p2 = -1;
	while (games[game_id].sockets.p1 == -1 || games[game_id].sockets.p2 == -1) {
		event_count = epoll_wait(games[game_id].epoll_instance, events, MAX_EVENTS, -1);
		for (i = 0; i < event_count; i++) {
			if (events[i].data.fd == p1_sfd && games[game_id].sockets.p1 == -1) {
				games[game_id].sockets.p1 = _connect_client(p1_sfd);
				if (games[game_id].sockets.p1 != -1)
					info("Game %zu: Player 1 connected", game_id);
			}
			else if (events[i].data.fd == p2_sfd && games[game_id].sockets.p2 == -1) {
				games[game_id].sockets.p2 = _connect_client(p2_sfd);
				if (games[game_id].sockets.p2 != -1)
					info("Game %zu: Player 2 connected", game_id);
			}
		}
	}
	epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_DEL, p1_sfd, NULL);
	epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_DEL, p2_sfd, NULL);
	close(p1_sfd);
	close(p2_sfd);
	if (set_non_blocking(games[game_id].sockets.p1) == -1 ||
		set_non_blocking(games[game_id].sockets.p2) == -1)
		die(strerror(errno));
}

static inline i32	_open_socket(const char *port) {
	struct sockaddr_in	*addr;
	struct addrinfo		*addresses;
	struct addrinfo		*cur_address;
	i32					out;

	debug("Opening socket on %s:%s", address, port);
	if (getaddrinfo(address, port, &hints, &addresses) != 0)
		die(strerror(errno));
	for (cur_address = addresses; cur_address != NULL; cur_address = cur_address->ai_next) {
		out = socket(cur_address->ai_family, cur_address->ai_socktype, cur_address->ai_protocol);
		if (out != -1) {
			if (setsockopt(out, SOL_SOCKET, SO_REUSEADDR, &(i32){1}, sizeof(i32)) == -1)
				die(strerror(errno));
			if (bind(out, cur_address->ai_addr, cur_address->ai_addrlen) == 0) {
				addr = (struct sockaddr_in *)cur_address->ai_addr;
				debug("Socket opened and bound to %hhu.%hhu.%hhu.%hhu:%hu",
					INET_N1(addr->sin_addr.s_addr), INET_N2(addr->sin_addr.s_addr),
					INET_N3(addr->sin_addr.s_addr), INET_N4(addr->sin_addr.s_addr),
					ntohs(addr->sin_port));
				break ;
			}
			close(out);
		}
	}
	freeaddrinfo(addresses);
	if (!cur_address)
		die("unable to bind");
	return out;
}

static inline i32	_connect_client(const i32 lfd) {
	struct sockaddr_in	peer;
	socklen_t			peer_size;
	i32					out;

	peer_size = sizeof(peer);
	if (pthread_self() == t0)
		debug("Listening for new clients");
	else
		debug("Attempting to accept a client");
	out = accept(lfd, (struct sockaddr *)&peer, &peer_size);
	if (out == -1) {
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			die(strerror(errno));
		errno = 0;
	} else {
		info("New connection received from %hhu.%hhu.%hhu.%hhu:%hu",
			INET_N1(peer.sin_addr.s_addr), INET_N2(peer.sin_addr.s_addr),
			INET_N3(peer.sin_addr.s_addr), INET_N4(peer.sin_addr.s_addr),
			ntohs(peer.sin_port));
	}
	return out;
}

static void	*_player_io(void *arg) {
	struct epoll_event	events[MAX_EVENTS];
	message				messages[2];
	message				*message;
	game				*game;
	i32					event_count;
	i32					i;
	u8					player;

	game = arg;
	while (1) {
		event_count = epoll_wait(game->epoll_instance, events, MAX_EVENTS, -1);
		for (i = 0; i < event_count; i++) {
			player = (events[i].data.fd == game->sockets.p1) ? GAME_PLAYER_1 : GAME_PLAYER_2;
			debug("Game %hhu: New message received from player %hhu", game->state.game_id, player);
			message = &messages[(player == GAME_PLAYER_1) ? 0 : 1];
			_get_message(message, events[i].data.fd);
			switch (message->type) {
				case MESSAGE_CLIENT_START:
					unpause_game(game->state.game_id, player);
					break ;
				case MESSAGE_CLIENT_PAUSE:
					pause_game(game->state.game_id, player);
					break ;
				case MESSAGE_CLIENT_MOVE_PADDLE:
					switch (*message->body) {
						case MESSAGE_CLIENT_MOVE_PADDLE_UP:
							move_paddle(game->state.game_id, player, UP);
							break ;
						case MESSAGE_CLIENT_MOVE_PADDLE_DOWN:
							move_paddle(game->state.game_id, player, DOWN);
							break ;
						case MESSAGE_CLIENT_MOVE_PADDLE_STOP:
							move_paddle(game->state.game_id, player, STOP);
							break ;
						default: // INVALID MESSAGE BODY
							;
					}
				default: // INVALID MESSAGE TYPE
					;
			}
		}
	}
	return NULL;
}

static void	*_run_game(void *arg) {
	struct epoll_event	ev;
	size_t				game_id;

	game_id = (size_t)(uintptr_t)arg;
	debug("Game %zu: Starting init", game_id);
	games[game_id].state = (state){
		.p1_paddle.pos = GAME_FIELD_HEIGHT / 2 + 1.0f, // remove once we have a client capable of moving paddles
		.p2_paddle.pos = GAME_FIELD_HEIGHT / 2,
		.p1_paddle.direction = STOP,
		.p2_paddle.direction = STOP,
		.ball.x = GAME_FIELD_WIDTH / 2,
		.ball.y = GAME_FIELD_HEIGHT / 2,
		.ball.angle = 0.0f,
		.ball.direction = (rand() & 1) ? LEFT : RIGHT,
		.score.p1 = 0,
		.score.p2 = 0,
		.game_id = game_id,
		.update = 0,
		.pause = GAME_PLAYER_1 | GAME_PLAYER_2
	};
	if (!games[game_id].epoll_instance) {
		debug("Game %zu: Setting up epoll", game_id);
		games[game_id].epoll_instance = epoll_create(1);
		if (games[game_id].epoll_instance == -1)
			die(strerror(errno));
	}
	_assign_ports(game_id);
	ev = (struct epoll_event){
		.events = EPOLLIN,
		.data.fd = games[game_id].sockets.p1
	};
	if (epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_ADD, games[game_id].sockets.p1, &ev) == -1)
		die(strerror(errno));
	ev.data.fd = games[game_id].sockets.p2;
	if (epoll_ctl(games[game_id].epoll_instance, EPOLL_CTL_ADD, games[game_id].sockets.p2, &ev) == -1)
		die(strerror(errno));
	debug("Game %zu: Creating threads", game_id);
	pthread_mutex_init(&state_locks[game_id], NULL);
	if (pthread_create(&games[game_id].threads.game_loop, NULL, pong, (void *)&games[game_id].state) == -1 ||
		pthread_create(&games[game_id].threads.player_io, NULL, _player_io, (void *)&games[game_id]) == -1)
		die(strerror(errno));
	while (1) {
		lock_game(game_id);
		if (games[game_id].state.update & GAME_UPDATE_ALLOWED) {
			send_message(game_id, MESSAGE_SERVER_STATE_UPDATE);
			games[game_id].state.update &= ~GAME_UPDATE_SCORE_PENDING;
		}
		unlock_game(game_id);
		usleep(16666);
	}
	// STATE UPDATE LOOP GOES HERE
	// CLEANUP GOES HERE
	return NULL;
}
