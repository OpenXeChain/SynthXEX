CC := cc
CFLAGS := 

build: src/*.c src/*/*.c src/*/*.h
	${CC} src/*.c src/*/*.c
