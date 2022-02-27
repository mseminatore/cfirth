CC = gcc
CFLAGS = -I.
DEPS = firth.h firth_config.h firth_float.h
OBJS = main.o firth.o core.o firth_float.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cfirth: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o *~ cfirth
