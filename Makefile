coomer: coomer.c
	cc -o coomer coomer.c -lX11 -Wall

install: coomer
	install ./coomer /usr/local/bin
