#!/bin/bash

# gcc -Wall -Wextra -Werror -Wshadow -ggdb main.c -DSOL_DEF -DDEBUG -o a
gcc -Wall -Wextra -Werror -Wshadow -ggdb -mfsgsbase cw.c -DSOL_DEF -DDEBUG -o a
