#!/usr/bin/env dash

c_file="src/main.c"
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
  echo "Building"
  cd src
  nix-shell -p gcc binutils gnumake --run "make scheme"

  echo "Finishing"
  cd ..
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
