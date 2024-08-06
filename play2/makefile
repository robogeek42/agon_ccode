# ----------------------------
# Makefile Options
# ----------------------------

NAME = play
DESCRIPTION = "play sounds"
COMPRESSED = NO
LDHAS_ARG_PROCESSING = 1

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

install: bin/$(NAME).bin
	#srec_cat bin/$(NAME).bin -binary -offset 0x40000 -o bin/$(NAME).hex -intel
	cp bin/$(NAME).bin $(NAME)
	rsync -rvu ./ ~/agon/sdcard_sync/cc/$(NAME)
