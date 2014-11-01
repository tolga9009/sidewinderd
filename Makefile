CXX = clang++
CXXFLAGS = -Wall -Wextra
LDFLAGS = -ludev -lconfig++

HDRS =
SRCS = main.cpp keyboard.cpp tinyxml2.cpp
OBJS = $(SRCS:.cpp=.o)
EXEC = sidewinderd

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o $(EXEC)

clean:
	rm -f *.o $(EXEC)
