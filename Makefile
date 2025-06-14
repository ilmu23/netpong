## ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
## ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
## █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
## ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
## ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
## ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
##
## <<Makefile>>

NAME		=	netpong_server
TEST_CLIENT	=	test_client

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
VECTDIR	=	vector

FILES	=	main.c \
			utils.c \
			$(PONGDIR)/pong.c \
			$(SERVDIR)/server.c \
			$(VECTDIR)/vector.c

SRCS	=	$(addprefix $(SRCDIR)/, $(FILES))
OBJS	=	$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRCS))

all: $(NAME) $(TEST_CLIENT)

$(NAME): $(OBJDIR) $(OBJS)
	@printf "\e[38;5;119;1mNETPONG >\e[m Compiling %s\n" $@
	@$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@
	@printf "\e[38;5;119;1mNETPONG >\e[m \e[1mDone!\e[m\n"

$(TEST_CLIENT): $(SRCDIR)/test_client.c $(SRCDIR)/utils.c
	@printf "\e[38;5;119;1mNETPONG >\e[m Compiling %s\n" $@
	@$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@
	@printf "\e[38;5;119;1mNETPONG >\e[m \e[1mDone!\e[m\n"

$(OBJDIR):
	@printf "\e[38;5;119;1mNETPONG >\e[m Creating objdirs\n"
	@mkdir -p $(OBJDIR)/$(PONGDIR)
	@mkdir -p $(OBJDIR)/$(SERVDIR)
	@mkdir -p $(OBJDIR)/$(VECTDIR)

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
