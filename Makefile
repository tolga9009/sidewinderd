CC = gcc
CXXFLAGS = -Wall -Wextra
LDFLAGS = -ludev
INCLUDES =

HDRS =
SRCS = sidewinderd.c
OBJS = $(SRCS:.c=.o)
EXEC = sidewinderd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(INCLUDES) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
