#!/usr/bin/env dash

c_file="scheme.c"
x_file="scheme"

memcheck() {
  valgrind --tool=memcheck \
           --leak-check=yes \
           --show-reachable=yes \
           --num-callers=20 \
           --track-fds=yes \
  	   --track-origins=yes \
           --log-file=memlog.txt \
           "${x_file}" 
}

build () {
  copts="-std=c11 -Wall -Wextra -Wpedantic -Werror -Wshadow"
  #TODO remove these flags once you finish reimplementing builtin functions"
  passes="-Wno-unused-parameter"
  
  nix-shell -p gcc binutils \
            --command "gcc -g ${copts} ${passes} ${c_file} linenoise/linenoise.o mpc/mpc.o -lm -o ${x_file}"
}

format () {
  clang-format -style=Chromium "$c_file" > temp.c
  [ 0 = "$?" ] && mv temp.c "$c_file"
}

usage () {
  echo "usage: ./tool [build|memcheck|format]"
}

case "$@" in
  memcheck) memcheck;;
  build) build;;
  format) format;;
  *) usage;;
esac

