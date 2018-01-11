#!/bin/sh

nix-shell -p gcc binutils \
          --command 'gcc -std=c99 -Wall parsing.c linenoise/linenoise.c mpc/mpc.c -lm -o parsing'
