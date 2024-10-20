SERVER = server

SERVER_SRC = server.c

CLIENT = client

CLIENT_SRC = client.c

FIFO_LOCATION = /tmp/FIFO_*

RM = rm -rf 


all: $(SERVER) $(CLIENT)

$(SERVER): $(SERVER_SRC)
	$(CC) -o $(SERVER) $(SERVER_SRC)

$(CLIENT): $(CLIENT_SRC)
	$(CC) -o $(CLIENT) $(CLIENT_SRC)

fclean:
	$(RM) $(FIFO_LOCATION) $(SERVER) $(CLIENT)

re: fclean all