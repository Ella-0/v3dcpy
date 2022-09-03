.POSIX:

LIBS = -lvulkan
OBJS = v3dcpy.o
CFLAGS = -g -std=c11 -Wall

.c.o:
	$(CC) -o $@ -c $(CFLAGS) $<

v3dcpy: $(OBJS)
	$(CC) -o $@ $(LIBS) $(OBJS)

clean:
	rm -f v3dcpy $(OBJS)
