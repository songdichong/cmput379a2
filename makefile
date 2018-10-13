all: server client

server: server.c
	@gcc server.c -std=c99 -pthread -o gameserver379

client: client.c
	@gcc client.c -std=c99 -pthread -lncurses -o gameclient379

clean:
	@rm -f gameserver379
	@rm -f gameclient379

