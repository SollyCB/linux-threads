#!/bin/bash

gcc -Wall -Wextra -Werror -Wshadow -ggdb -mfsgsbase main.c -DSOL_DEF -DDEBUG -o a
