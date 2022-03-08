# Copyright (C) 2015-2020 Davidson Francis <davidsondfgl@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>

CC      ?= gcc
WSDIR    = $(CURDIR)/lib/wsServer
INCLUDE  = -I $(WSDIR)/include
CFLAGS   =  -Wall -Wextra -O2
CFLAGS  +=  $(INCLUDE) -std=c99 -pthread -pedantic
LIB      =  $(WSDIR)/libws.a
BUILD_DIR= _build
RM	 = rm -rf

# Check if verbose examples
ifeq ($(VERBOSE_EXAMPLES), no)
	CFLAGS += -DDISABLE_VERBOSE
endif

.PHONY: all clean

# Examples
all: libws.a main

# Send receive
main: src/main.c $(LIB)
	[ -d $(BUILD_DIR) ] || mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) src/main.c -o $(BUILD_DIR)/main $(LIB)

run:
	@printf " --- Running executable ---\n\n"
	@cd _build && ./main
	@printf "\n\n --- DONE ---\n"

# Library
libws.a: $(OBJ)
	$(AR) $(ARFLAGS) $(LIB) $^

# Clean
clean:
	$(RM) _build
