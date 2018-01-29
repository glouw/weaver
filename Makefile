# Portable aliases.
# CompSec defined if in Windows.
ifdef ComSpec
	RM = del /F /Q
	MV = ren
else
	RM = rm -f
	MV = mv -f
endif

# Compiler and standard.
CC = gcc -std=c99

# Project name.
PROJ = weaver

# Source files.
SRCS = main.c

# Warnings flags.
CFLAGS = -Wshadow -Wall -Wpedantic -Wextra -Wdouble-promotion

# Debugging flags.
CFLAGS+= -g

# Optimization flags.
CFLAGS+= -Ofast -march=native

# Linker flags.
ifdef ComSpec
	# Windows requires additional SDL2main library.
	#  in the exe folder.
	LDFLAGS = -Isystem. -L. -lm -lSDL2main -lSDL2 -lSDL2_image
else
	LDFLAGS = -lm -lSDL2 -lSDL2_image
endif

# Linker.
$(PROJ): $(SRCS:.c=.o)
	$(CC) $(CFLAGS) $(SRCS:.c=.o) $(LDFLAGS) -o $(PROJ)

# Compiler template; generates dependency targets.
%.o : %.c
	$(CC) $(CFLAGS) -MMD -MP -MT $@ -MF $*.td -c $<
	$(MV) $*.td $*.d
	
# All dependency targets.
%.d: ;
-include *.d

clean:
	$(RM) vgcore.*
	$(RM) cachegrind.out.*
	$(RM) callgrind.out.*
	$(RM) $(PROJ)
	$(RM) $(SRCS:.c=.o)
	$(RM) $(SRCS:.c=.d)
