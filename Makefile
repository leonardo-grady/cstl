CC = $(CROSS)gcc
CFLAGS += -fPIC -g -Wall $(GSFLAGS) -O0 -DLOG_TAG=\"libcstl\"

all: libs
libs: libcstl.a libcstl.so

libcstl.a: cstl.o
	$(AR) cr $@ $^

libcstl.so: cstl.o
	$(CC) -fPIC -shared -o $@ $^

clean:
	@rm -f *.o 
	@rm -rf *.a *.so 

.PHONY: clean libs
