// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<test_client.c>>

#include "netpong.h"

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

static inline void	_receive_msg(const i32 sfd, u16 ports[2]);
static inline void	_send_msg(const i32 sfd, const u8 msg_type, const u8 body[18]);
static inline i32	_connect_player(const char *addr, const u16 port);
static inline i32	_connect(const char *addr, const char *port);

i32	main(i32 ac, char **av) {
	struct epoll_event	events[3];
	struct epoll_event	ev;
	i32					ev_count;
	i32					p1fd;
	i32					p2fd;
	i32					efd;
	i32					sfd;
	i32					i;
	u16					ports[2];
	u8					c;

	if (ac != 3)
		return 1;
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
	fprintf(stderr, SGR_DEBUG "Press return to connect player 2 " SGR_RESET);
	(void)read(0, &c, 1);
	p2fd = _connect_player(av[1], ports[1]);
	fprintf(stderr, SGR_DEBUG "Press return to unpause player 2 " SGR_RESET);
	(void)read(0, &c, 1);
	_send_msg(p2fd, MESSAGE_CLIENT_START, NULL);
	fprintf(stderr, SGR_DEBUG "Press return to unpause player 1 " SGR_RESET);
	(void)read(0, &c, 1);
	_send_msg(p1fd, MESSAGE_CLIENT_START, NULL);
	while (1) {
		ev_count = epoll_wait(efd, events, 3, -1);
		for (i = 0; i < ev_count; i++)
			_receive_msg(events[i].data.fd, NULL);
	}
	return 0;
}

static inline void	_receive_msg(const i32 sfd, u16 ports[2]) {
	union {
		msg_srv_init	*init;
		msg_srv_state	*state;
	}		body;
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
				info("\tWinner: Player %hhu", msg.body[0]);
				break ;
			case MESSAGE_SERVER_STATE_UPDATE:
				body.state = (msg_srv_state *)msg.body;
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
