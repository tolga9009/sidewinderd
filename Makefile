CC = gcc
CXXFLAGS = -Wall
LDFLAGS = -lusb-1.0
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
