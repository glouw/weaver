CC = gcc

NAME = weaver

SRCS = main.c

ifdef ComSpec
	BIN = $(NAME).exe
else
	BIN = $(NAME)
endif

CFLAGS =
ifdef ComSpec
	CFLAGS += -I ../SDL2-2.0.7/i686-w64-mingw32/include
	CFLAGS += -I ../SDL2-2.0.7/i686-w64-mingw32/include/SDL2
	CFLAGS += -I ../SDL2_image-2.0.2/i686-w64-mingw32/include
	CFLAGS += -I ../SDL2_image-2.0.2/i686-w64-mingw32/include/SDL2
endif
CFLAGS += -std=c99
CFLAGS += -Wshadow -Wall -Wpedantic -Wextra -Wdouble-promotion
CFLAGS += -g
CFLAGS += -Ofast -march=pentium4
CFLAGS += -flto

LDFLAGS =
ifdef ComSpec
	LDFLAGS += -static-libgcc
	LDFLAGS += -L..\SDL2_image-2.0.2\i686-w64-mingw32\lib
	LDFLAGS += -L..\SDL2-2.0.7\i686-w64-mingw32\lib
	LDFLAGS += -lmingw32
	LDFLAGS += -lSDL2main
endif
LDFLAGS += -lSDL2 -lSDL2_image -lm

ifdef ComSpec
	# Windows.
	RM = del /F /Q
	MV = ren
else
	# Unix.
	RM = rm -f
	MV = mv -f
endif

# Link.
$(BIN): $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(SRCS:.c=.o) $(LDFLAGS) -o $(BIN)

# Compile.
%.o : %.c
	$(CC) $(CFLAGS) -MMD -MP -MT $@ -MF $*.td -c $<
	@$(RM) $*.d
	@$(MV) $*.td $*.d
%.d: ;
-include *.d

clean:
	$(RM) vgcore.*
	$(RM) cachegrind.out.*
	$(RM) callgrind.out.*
	$(RM) $(BIN)
	$(RM) $(SRCS:.c=.o)
	$(RM) $(SRCS:.c=.d)
