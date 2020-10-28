CFLAGS=	-Wall -Wextra -g
all:	diff patch

diff:	common.h
patch:	common.h

test:
	$(MAKE) -C test
clean:
	$(RM) -f diff patch

.PHONY:	all test clean
