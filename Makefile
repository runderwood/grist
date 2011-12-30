srcdir=./src/
builddir=./build/

CC=gcc
CFLAGS=-std=gnu99 -Wall

test:
	$(CC) -I. -I$(srcdir) -lm $(CFLAGS) $(srcdir)geom.c $(srcdir)grist.c $(srcdir)test.c -o $(builddir)grist_test

