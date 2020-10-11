CFLAGS=	-Wall -Wextra -g
all:	diff patch

diff:
patch:

test:
	$(MAKE) -C test

.PHONY:	all test
