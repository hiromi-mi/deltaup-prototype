CFLAGS=	-Wall -Wextra -g
all:	diff patch

diff:
patch:

test:
	$(MAKE) -C test
clean:
	$(RM) -f diff patch

.PHONY:	all test clean
