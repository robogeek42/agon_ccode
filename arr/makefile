# ----------------------------
# Makefile Options
# ----------------------------

NAME = arr
DESCRIPTION = "Ag C array alloc demo"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# Heap size to 384k allowing a 64k program
INIT_LOC = 040000
BSSHEAP_LOW = 050000
BSSHEAP_HIGH = 0AFFFF

# ----------------------------

include $(shell cedev-config --makefile)

install: bin/$(NAME).bin
	srec_cat bin/$(NAME).bin -binary -offset 0x40000 -o bin/$(NAME).hex -intel
	cp bin/$(NAME).bin ~/agon/fab-agon-emulator/sdcard/cc/$(NAME)/
