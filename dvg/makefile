# ----------------------------
# Makefile Options
# ----------------------------

NAME = dvg
DESCRIPTION = "Digital Vector Generator (DVG) Interpreter"
COMPRESSED = NO
LDHAS_ARG_PROCESSING = 1

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

EXTRA_C_SOURCES = ../common/util.c

# ----------------------------

include $(shell cedev-config --makefile)

install: bin/$(NAME).bin
	cp bin/$(NAME).bin .
	rsync -rvu ./ ~/agon/sdcard_sync/cc/$(NAME)
