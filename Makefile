CC        = $(CROSS_COMPILE)g++
STRIP     = $(CROSS_COMPILE)strip

TARGET    = ctw.so

HASH_SRCS = hashcodes/crypto_hash.cpp hashcodes/sha1.cpp hashcodes/md5.cpp
HTTP_SRCS = http/httpclient.cpp http/httpcurl.cpp
AUTH_SRCS = oauth/base64.cpp oauth/oauth.cpp
TWCR_SRCS = twitter/twitter_client.cpp
UI_SRCS   = ui/simple_ui.cpp ui/simple_terminal.cpp ui/bot.cpp
SRCS	  = main.cpp
OBJS	  = $(HASH_SRCS:%.cpp=%.o) $(HTTP_SRCS:%.cpp=%.o) $(AUTH_SRCS:%.cpp=%.o) $(TWCR_SRCS:%.cpp=%.o) $(UI_SRCS:%.cpp=%.o) $(SRCS:%.cpp=%.o) cmd.o
DEPS	  = $(HASH_SRCS:%.cpp=%.d) $(HTTP_SRCS:%.cpp=%.d) $(AUTH_SRCS:%.cpp=%.d) $(TWCR_SRCS:%.cpp=%.d) $(UI_SRCS:%.cpp=%.d) $(SRCS:%.cpp=%.d)
KEYS	  = keys/apikeys.hpp
INCLUDE_DIRS = ./hashcodes ./http ./oauth ./twitter ./ui ./include .

CFLAGS    = -Wall -O2 $(addprefix -I,$(INCLUDE_DIRS)) -fPIC
LDFLAGS   = -lcurl -shared

CMD_INCLUDES=-I../../../../modules/lua/src \
	-I$(lastword $(wildcard ../../../../modules/ncurses/build/*/include)) \
	-I$(lastword $(wildcard ../../../../modules/libedit/build/*/src))
CMD_LFLAGS=-L../../../../modules/lua/src \
	-L$(lastword $(wildcard ../../../../modules/ncurses/build/*/lib)) \
	-L$(lastword $(wildcard ../../../../modules/libedit/build/*/src/.libs))
CMD_LIBS=-ledit -ltinfo -llua
CMD_CC=gcc
CMD_CFLAGS=-O2 -g -Wall -std=gnu99 -fPIC

ifndef CONSUMER_KEY
$(error CONSUMER_KEY can not find from enviroment! you must define CONSUMER_KEY)
endif
ifndef CONSUMER_SECRET
$(error CONSUMER_SECRET can not find from enviroment! you must define CONSUMER_SECRET)
endif


all: $(KEYS) $(TARGET)

-include $(DEPS)

clean: 
	rm -f $(OBJS) $(DEPS) $(TARGET) *~ *.scc *.vcproj *.vspscc $(KEYS)

$(KEYS):
	-@ ./write_apikeys.sh

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(DEBUGS) -o $@ $^ $(LDFLAGS) $(CMD_LFLAGS) $(CMD_LIBS)

.cpp.o:
	$(CC) -c -MMD -MP -o $@ $(CFLAGS) $(DEBUGS) $<

cmd.o: cmd.c
	$(CMD_CC) -c -MMD -MP -o $@ $(CMD_CFLAGS) $(CMD_INCLUDES) $<
