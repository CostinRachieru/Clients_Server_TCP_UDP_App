# Protocoale de comunicatii:
# Makefile

CFLAGS = -Wall -g 

all: server subscriber

# Compileaza server.cpp
server: server.cpp

# Compileaza subscriber.cpp
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${ID} ${IP_SERVER} ${PORT}

clean:
	rm -f server subscriber
