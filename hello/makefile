# ----------------------------
# Makefile Options
# ----------------------------

NAME = hello
DESCRIPTION = "hello world"
COMPRESSED = NO
LDHAS_ARG_PROCESSING = 1

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

install: bin/$(NAME).bin
	cp bin/$(NAME).bin $(NAME)
	
