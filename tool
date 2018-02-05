#!/usr/bin/env dash

c_file="scheme.c"
x_file="scheme"
err_file="err.ascii"

memcheck() {
  valgrind --tool=memcheck \
           --leak-check=yes \
           --show-reachable=yes \
           --num-callers=20 \
           --track-fds=yes \
  	   --track-origins=yes \
           --log-file=memlog.txt \
           "./${x_file}" 
}

build () {
  other_opts="-fdiagnostics-color=always"
  copts="-std=c11 -Wall -Wextra -Wpedantic -Werror -Wshadow"
  #TODO remove these flags once you finish reimplementing builtin functions"
  passes="-Wno-unused-parameter"
  input_files="${c_file} linenoise/linenoise.o mpc/mpc.o"
  
  nix-shell -p gcc binutils \
            --command "gcc -g ${other_opts} ${copts} ${passes} ${input_files} -lm -o ${x_file}" \
    >"$err_file" 2>&1 
}

run () {
  if build; then
    memcheck
    less memlog.txt
  else
    less -r "$err_file"
  fi
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
  run) run;;
  *) usage;;
esac
