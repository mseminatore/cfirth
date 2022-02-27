CC = gcc
CFLAGS = -I.
DEPS = firth.h firth_config.h firth_float.h
OBJS = main.o firth.o core.o firth_float.o
LIBS = -lm

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

firth: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f *.o *~ firth

check:
	./firth ./test/core-tests.fth

#distcheck: