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
#include "utils/vector.h"

#define atomic _Atomic

static pthread_mutex_t	gamelist_lock = PTHREAD_MUTEX_INITIALIZER;
static const char		*server_message_type_strings[] = {"Game init", "Game paused", "Game over", "State update"};
static const char		*address;
static pthread_t		t0;
static socklen_t		addr_length;
static atomic u8		___terminate = 0;
static vector			active_games;
static size_t			game_count;

static const struct addrinfo	hints = {
	.ai_family = AF_INET,
	.ai_socktype = SOCK_STREAM,
	.ai_flags = AI_PASSIVE,
	.ai_protocol = 0,
	.ai_canonname = NULL,
	.ai_addr = NULL,
	.ai_next = NULL
};

#define Die()	(die(strerror(errno)))

#define set_non_blocking(fd)	((fd != -1) ? fcntl(fd, F_SETFL, O_NONBLOCK) : 1)

#define INET_N1(a)	((u8)(ntohl(a) >> 24 & 0xFF))
#define INET_N2(a)	((u8)(ntohl(a) >> 16 & 0xFF))
#define INET_N3(a)	((u8)(ntohl(a) >> 8 & 0xFF))
#define INET_N4(a)	((u8)(ntohl(a) & 0xFF))

static inline void	_sigint_handle([[gnu::unused]] const i32 signum);
static inline void	_terminate(const vector main_threads);
static inline void	_host_game(const i32 cfd, vector main_threads);
static inline void	_get_message(message *msg, const i32 fd);
static inline void	_assign_ports(game *game);
static inline i32	_open_socket(const char *port);
static inline i32	_connect_client(const i32 lfd);
static void			*_player_io(void *arg);
static void			*_run_game(void *arg);

[[gnu::noreturn]] void	run_server(const char *addr, const char *port) {
	struct sigaction	sigint;
	vector				main_threads;
	i32					lfd;
	i32					cfd;

	address = addr;
	t0 = pthread_self();
	lfd = _open_socket(port);
	active_games = vector(game *, 25, NULL);
	main_threads = vector(pthread_t, 25, NULL);
	if (!active_games || !main_threads)
		Die();
	if (listen(lfd, MAX_LISTEN_BACKLOG) == -1)
		Die();
	memset(&sigint, 0, sizeof(sigint));
	sigint.sa_handler = _sigint_handle;
	sigaction(SIGINT, &sigint, NULL);
	info("Server initialized and listening on %s:%s", address, port);
	while (!___terminate) {
		cfd = _connect_client(lfd);
		if (cfd != -1)
			_host_game(cfd, main_threads);
	}
	info("Server shutting down");
	_terminate(main_threads);
	vector_delete(active_games);
	vector_delete(main_threads);
	close(lfd);
	exit(0);
}

void	send_message(const game *game, const u8 message_type, const uintptr_t arg) {
	message	msg;

	msg = (message){.version = PROTOCOL_VERSION, .type = message_type};
	debug("Game %hhu: Sending message: %s", game->id, server_message_type_strings[message_type]);
	switch (message_type) {
		case MESSAGE_SERVER_GAME_INIT:
			msg.length = sizeof(msg_srv_init);
			*(msg_srv_init *)msg.body = _msg_srv_init(game->ports.p1, game->ports.p2);
			break ;
		case MESSAGE_SERVER_GAME_PAUSED:
			msg.length = 0;
			break ;
		case MESSAGE_SERVER_GAME_OVER:
#if PROTOCOL_VERSION == 0
			msg.length = 1;
			msg.body[0] = (u8)arg;
			info("Game %hhu: Game over, winner: Player %hhu", game->state.game_id, msg.body[0]);
#elif PROTOCOL_VERSION >= 1
			msg.length = sizeof(msg_srv_game_over);
			*(msg_srv_game_over *)msg.body = _msg_srv_game_over( (u8)(arg & 0xFF), (u8)(arg >> 8 & 0xFF), game->state.score);
#endif /* PROTOCOL_VERSION */
			break ;
		case MESSAGE_SERVER_STATE_UPDATE:
			msg.length = sizeof(msg_srv_state);
			*(msg_srv_state *)msg.body = _msg_srv_state(game->state.p1_paddle.pos, game->state.p2_paddle.pos,
					game->state.ball.x, game->state.ball.y, game->state.score);
	}
	if (send(game->sockets.state, &msg, MESSAGE_HEADER_SIZE + msg.length, 0) == -1)
		Die();
}

static inline void	_sigint_handle([[gnu::unused]] const i32 signum) {
	___terminate = 1;
}

static inline void	_terminate(const vector main_threads) {
	size_t	i;

	pthread_mutex_lock(&gamelist_lock);
	for (i = 0; i < vector_size(active_games); i++) {
			quit_game(*(game **)vector_get(active_games, i), 0);
	}
	pthread_mutex_unlock(&gamelist_lock);
	for (i = 0; i < vector_size(main_threads); i++)
		pthread_join(*(pthread_t *)vector_get(main_threads, i), NULL);
}

static inline void	_host_game(const i32 cfd, vector main_threads) {
	pthread_t	thread;

	if (game_count == MAX_GAME_COUNT) {
		close(cfd);
		return ;
	}
	if (pthread_create(&thread, NULL, _run_game, (void *)(uintptr_t)cfd) != 0)
		Die();
	if (!vector_push(main_threads, thread))
		Die();
}

static inline void	_get_message(message *msg, const i32 fd) {
	ssize_t	rv;
	u8		_terminate;

	rv = recv(fd, msg, MESSAGE_HEADER_SIZE, MSG_WAITALL);
	_terminate = ___terminate;
	if (_terminate)
		pthread_exit(NULL);
	if (rv == 0) {
		*msg = (message){
			.version = PROTOCOL_VERSION,
			.type = MESSAGE_CLIENT_QUIT,
			.length = 0,
		};
		return ;
	}
	if (rv != MESSAGE_HEADER_SIZE)
		Die();
	if (msg->length) {
		rv = recv(fd, msg->body, (msg->length <= sizeof(msg->body)) ? msg->length : sizeof(msg->body), MSG_WAITALL);
		if (rv != msg->length)
			Die();
	}
}

static inline void	_assign_ports(game *game) {
	struct epoll_event	events[MAX_EVENTS];
	struct sockaddr_in	addr;
	i32					event_count;
	i32					p1_sfd;
	i32					p2_sfd;
	i32					i;

	debug("Game %hhu: Starting player port assignment", game->id);
	p1_sfd = _open_socket(NULL);
	p2_sfd = _open_socket(NULL);
	if (listen(p1_sfd, MAX_LISTEN_BACKLOG) == -1 || listen(p2_sfd, MAX_LISTEN_BACKLOG) ||
		set_non_blocking(p1_sfd) == -1 || set_non_blocking(p2_sfd) == -1)
		Die();
	addr_length = sizeof(addr);
	if (getsockname(p1_sfd, (struct sockaddr *)&addr, &addr_length) == -1)
		Die();
	game->ports.p1 = ntohs(addr.sin_port);
	addr_length = sizeof(addr);
	if (getsockname(p2_sfd, (struct sockaddr *)&addr, &addr_length) == -1)
		Die();
	game->ports.p2 = ntohs(addr.sin_port);
	events[0] = (struct epoll_event){
		.events = EPOLLIN,
		.data.fd = p1_sfd
	};
	if (epoll_ctl(game->epoll_instance, EPOLL_CTL_ADD, p1_sfd, &events[0]) == -1)
		Die();
	events[0].data.fd = p2_sfd;
	if (epoll_ctl(game->epoll_instance, EPOLL_CTL_ADD, p2_sfd, &events[0]) == -1)
		Die();
	info("Game %hhu: Assigning ports for players:", game->id);
	info("\tPlayer 1: %hu\n\tPlayer 2: %hu", game->ports.p1, game->ports.p2);
	send_message(game, MESSAGE_SERVER_GAME_INIT, 0);
	game->sockets.p1 = -1;
	game->sockets.p2 = -1;
	while (game->sockets.p1 == -1 || game->sockets.p2 == -1) {
		event_count = epoll_wait(game->epoll_instance, events, MAX_EVENTS, PLAYER_CONNECT_TIMEOUT);
		if (event_count == 0) {
			warn("Game %hhu: Timed out waiting for players", game->id);
			game->state.quit = 1;
			break ;
		}
		for (i = 0; i < event_count; i++) {
			if (events[i].data.fd == p1_sfd && game->sockets.p1 == -1) {
				game->sockets.p1 = _connect_client(p1_sfd);
				if (game->sockets.p1 != -1)
					info("Game %hhu: Player 1 connected", game->id);
			}
			else if (events[i].data.fd == p2_sfd && game->sockets.p2 == -1) {
				game->sockets.p2 = _connect_client(p2_sfd);
				if (game->sockets.p2 != -1)
					info("Game %hhu: Player 2 connected", game->id);
			}
		}
	}
	epoll_ctl(game->epoll_instance, EPOLL_CTL_DEL, p1_sfd, NULL);
	epoll_ctl(game->epoll_instance, EPOLL_CTL_DEL, p2_sfd, NULL);
	close(p1_sfd);
	close(p2_sfd);
	if (set_non_blocking(game->sockets.p1) == -1 ||
		set_non_blocking(game->sockets.p2) == -1)
		Die();
}

static inline i32	_open_socket(const char *port) {
	struct sockaddr_in	addr;
	struct addrinfo		*addresses;
	struct addrinfo		*cur_address;
	i32					out;

	debug("Opening socket on %s:%s", address, port);
	if (getaddrinfo(address, port, &hints, &addresses) != 0)
		Die();
	for (cur_address = addresses; cur_address != NULL; cur_address = cur_address->ai_next) {
		out = socket(cur_address->ai_family, cur_address->ai_socktype, cur_address->ai_protocol);
		if (out != -1) {
			if (setsockopt(out, SOL_SOCKET, SO_REUSEADDR, &(i32){1}, sizeof(i32)) == -1)
				Die();
			if (bind(out, cur_address->ai_addr, cur_address->ai_addrlen) == 0) {
				addr_length = sizeof(addr);
				getsockname(out, (struct sockaddr *)&addr, &addr_length);
				debug("Socket opened and bound to %hhu.%hhu.%hhu.%hhu:%hu",
					INET_N1(addr.sin_addr.s_addr), INET_N2(addr.sin_addr.s_addr),
					INET_N3(addr.sin_addr.s_addr), INET_N4(addr.sin_addr.s_addr),
					ntohs(addr.sin_port));
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
		if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)
			Die();
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
			debug("Game %hhu: New message received from player %hhu", game->id, player);
			message = &messages[(player == GAME_PLAYER_1) ? 0 : 1];
			_get_message(message, events[i].data.fd);
			if (message->version != PROTOCOL_VERSION) {
				quit_game(game, 0);
				return NULL;
			}
			switch (message->type) {
				case MESSAGE_CLIENT_START:
					unpause_game(game, player);
					break ;
				case MESSAGE_CLIENT_PAUSE:
					pause_game(game, player);
					break ;
				case MESSAGE_CLIENT_MOVE_PADDLE:
					switch (*message->body) {
						case MESSAGE_CLIENT_MOVE_PADDLE_UP:
							move_paddle(game, player, UP);
							break ;
						case MESSAGE_CLIENT_MOVE_PADDLE_DOWN:
							move_paddle(game, player, DOWN);
							break ;
						case MESSAGE_CLIENT_MOVE_PADDLE_STOP:
							move_paddle(game, player, STOP);
							break ;
						default: // INVALID MESSAGE BODY
							;
					}
					break ;
				case MESSAGE_CLIENT_QUIT:
					quit_game(game, player);
					return NULL;
				default: // INVALID MESSAGE TYPE
					;
			}
		}
	}
	return NULL;
}

static void	*_run_game(void *arg) {
	struct epoll_event	ev;
	sigset_t			sigs;
	size_t				size;
	size_t				i;
	game				_game;
	u8					tmp;

	sigemptyset(&sigs);
	sigaddset(&sigs, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &sigs, NULL);
	pthread_mutex_lock(&gamelist_lock);
	_game = (game){
		.state = (state){
			.lock = PTHREAD_MUTEX_INITIALIZER,
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
			.started = 0,
			.update = 0,
			.pause = GAME_PLAYER_1 | GAME_PLAYER_2,
			.quit = 0
		},
		.threads.game_master = pthread_self(),
		.threads.game_loop = -1,
		.threads.player_io = -1,
		.sockets.state = (i32)(uintptr_t)arg
	};
	for (i = 0, size = vector_size(active_games); i < size; i++)
		if ((*(game **)vector_get(active_games, i))->id > i)
			break ;
	if (!vector_insert(active_games, i, (game *){&_game}))
		Die();
	_game.id = i;
	pthread_mutex_unlock(&gamelist_lock);
	debug("Game %hhu: Starting init", _game.id);
	debug("Game %hhu: Creating epoll instance", _game.id);
	_game.epoll_instance = epoll_create(1);
	if (_game.epoll_instance == -1)
		Die();
	_assign_ports(&_game);
	if (!_game.state.quit) {
		ev = (struct epoll_event){
			.events = EPOLLIN,
			.data.fd = _game.sockets.p1
		};
		if (epoll_ctl(_game.epoll_instance, EPOLL_CTL_ADD, _game.sockets.p1, &ev) == -1)
			Die();
		ev.data.fd = _game.sockets.p2;
		if (epoll_ctl(_game.epoll_instance, EPOLL_CTL_ADD, _game.sockets.p2, &ev) == -1)
			Die();
		debug("Game %hhu: Creating threads", _game.id);
		if (pthread_create(&_game.threads.game_loop, NULL, pong, &_game) == -1 ||
			pthread_create(&_game.threads.player_io, NULL, _player_io, &_game) == -1)
			Die();
	}
	for (check_quit(_game.state, tmp); !tmp; check_quit(_game.state, tmp)) {
		lock_game(_game.state);
		if (_game.state.update & GAME_UPDATE_ALLOWED) {
			send_message(&_game, MESSAGE_SERVER_STATE_UPDATE, 0);
			_game.state.update &= ~GAME_UPDATE_SCORE_PENDING;
		}
		unlock_game(_game.state);
		usleep(16666);
	}
	lock_game(_game.state);
	debug("Game %1$hhu: Starting cleanup\nGame %1$hhu: Joining player IO thread", _game.id);
	if (!pthread_equal(_game.threads.player_io, -1)) {
		pthread_cancel(_game.threads.player_io);
		pthread_join(_game.threads.player_io, NULL);
	}
	debug("Game %hhu: Joining game thread", _game.id);
	if (!pthread_equal(_game.threads.game_loop, -1)) {
		pthread_cancel(_game.threads.game_loop);
		pthread_join(_game.threads.game_loop, NULL);
	}
	unlock_game(_game.state);
	pthread_mutex_destroy(&_game.state.lock);
	debug("Game %hhu: Destroying epoll instance", _game.id);
	close(_game.epoll_instance);
	debug("Game %hhu: Closing sockets", _game.id);
	close(_game.sockets.state);
	if (_game.sockets.p2 != -1)
		close(_game.sockets.p2);
	if (_game.sockets.p1 != -1)
		close(_game.sockets.p1);
	pthread_mutex_lock(&gamelist_lock);
	for (i = 0; i < vector_size(active_games); i++) {
		if ((*(game **)vector_get(active_games, i))->id == _game.id) {
			vector_erase(active_games, i);
			break ;
		}
	}
	pthread_mutex_unlock(&gamelist_lock);
	return NULL;
}
