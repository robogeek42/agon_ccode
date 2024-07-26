# ----------------------------
# Makefile Options
# ----------------------------

NAME = keys
DESCRIPTION = "keys"
COMPRESSED = NO
LDHAS_ARG_PROCESSING = 1

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

install: src/main.c bin/$(NAME).bin
	cp bin/$(NAME).bin $(NAME)
