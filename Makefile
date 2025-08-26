## ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
## ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
## █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
## ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
## ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
## ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
##
## <<Makefile>>

NAME		=	netpong_server

BUILD	=	fsan
ifeq ("$(BUILD)", "normal")
	LOG_LEVEL	=	3
else
	LOG_LEVEL	=	4
endif

CC				=	gcc
cflags.common	=	-Wall -Wextra -Werror -Wpedantic -pedantic-errors -std=gnu2x -DLOG_LEVEL=$(LOG_LEVEL) -I$(INCDIR)
cflags.debug	=	-g
cflags.fsan		=	$(cflags.debug) -fsanitize=address,undefined
cflags.normal	=	-s -O1
cflags.extra	=	
CFLAGS			=	$(cflags.common) $(cflags.$(BUILD)) $(cflags.extra)

LDFLAGS	=	-lpthread -lm

SRCDIR	=	src
OBJDIR	=	obj
INCDIR	=	inc

PONGDIR	=	pong
SERVDIR	=	server
UTILDIR	=	utils

FILES	=	main.c \
			$(PONGDIR)/pong.c \
			$(SERVDIR)/server.c \
			$(UTILDIR)/misc.c \
			$(UTILDIR)/vector.c

SRCS	=	$(addprefix $(SRCDIR)/, $(FILES))
OBJS	=	$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

all: $(NAME)

$(NAME): $(OBJDIR) $(OBJS)
	@printf "\e[38;5;119;1mNETPONG >\e[m Compiling %s\n" $@
	@$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	@printf "\e[38;5;119;1mNETPONG >\e[m \e[1mDone!\e[m\n"

$(OBJDIR):
	@printf "\e[38;5;119;1mNETPONG >\e[m Creating objdirs\n"
	@mkdir -p $(OBJDIR)/$(PONGDIR)
	@mkdir -p $(OBJDIR)/$(SERVDIR)
	@mkdir -p $(OBJDIR)/$(UTILDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@printf "\e[38;5;119;1mNETPONG >\e[m Compiling %s\n" $<
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -f $(OBJS) $(ASMOBJS)

fclean: clean
	@rm -rf $(OBJDIR)
	@rm -f $(TEST_CLIENT)
	@rm -f $(NAME)

re: fclean all

db:
	@printf "\e[38;5;119;1mNETPONG >\e[m Creating compilation command database\n"
	@compiledb make --no-print-directory BUILD=$(BUILD) cflags.extra=$(cflags.extra) | sed -E '/^##.*\.\.\.$$|^[[:space:]]*$$/d'
	@printf "\e[38;5;119;1mNETPONG >\e[m \e[1mDone!\e[m\n"

.PHONY: all clean fclean re db
