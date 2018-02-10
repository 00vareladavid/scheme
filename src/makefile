CC = gcc
# should I add -Wpedantic?
CFLAGS = -g -std=c11 -Wall -Wextra -Werror -Wshadow
OUTPUT_OPTION = -MMD -MP -o $@

SOURCE = $(wildcard *.c)
OBJS = $(SOURCE:.c=.o)
DEPS = $(SOURCE:.c=.d)
-include ${DEPS}

scheme: ${OBJS}
	${CC} -o scheme ${OBJS}

clean:
	rm ${OBJS}
help:
	-@echo "make scheme: make the whole enchilada"
        -@echo "make help: view usage"
