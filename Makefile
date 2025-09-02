CC = gcc
CFLAGS = -Wall -Wextra -Werror
SRC = main.c utils.c parse.c
OBJ = $(SRC:.c=.o)
EXEC = ft_traceroute

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(EXEC) -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(EXEC)

re: fclean all

.PHONY: all clean fclean re