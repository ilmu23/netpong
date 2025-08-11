// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<server.h>>

#include "data.h"
#include "utils/misc.h"

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#define MAX_EVENTS	2

#define MAX_LISTEN_BACKLOG	64

#define MESSAGE_CLIENT_MOVE_PADDLE_UP	0x1U
#define MESSAGE_CLIENT_MOVE_PADDLE_DOWN	0x2U
#define MESSAGE_CLIENT_MOVE_PADDLE_STOP	0x0U

#define MESSAGE_SERVER_GAME_OVER_ACT_WON		0x1U
#define MESSAGE_SERVER_GAME_OVER_ACT_QUIT		0x2U
#define MESSAGE_SERVER_GAME_OVER_SERVER_CLOSED	0x3U

[[gnu::noreturn]] void	run_server(const char *address, const char *port);

void	send_message(const game *game, const u8 message_type, const uintptr_t arg);
