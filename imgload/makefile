# ----------------------------
# Makefile Options
# ----------------------------

NAME = imgload
DESCRIPTION = "image load test"
COMPRESSED = NO
LDHAS_ARG_PROCESSING = 1

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)

install: bin/$(NAME).bin
	srec_cat bin/$(NAME).bin -binary -offset 0x40000 -o bin/$(NAME).hex -intel
	cp bin/$(NAME).bin ~/agon/fab-agon-emulator/sdcard/c/
	cp bin/$(NAME).bin ~/agon/sdcard_sync/c/
