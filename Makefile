CFLAGS=	-Wall -Wextra -g -ldivsufsort
all:	diff patch view

diff:	common.h common.o
patch:	common.h common.o
view:	common.h common.o

test:
	$(MAKE) -C test
clean:
	$(RM) -f diff patch

.PHONY:	all test clean
