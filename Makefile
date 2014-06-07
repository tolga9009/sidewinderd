CC = gcc
CFLAGS += $(INCLUDES)
CXXFLAGS = -Wall -Wextra
LDFLAGS = -ludev -lconfig $(shell xml2-config --libs)
INCLUDES = $(shell xml2-config --cflags)

HDRS =
SRCS = sidewinderd.c
OBJS = $(SRCS:.c=.o)
EXEC = sidewinderd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(INCLUDES) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
