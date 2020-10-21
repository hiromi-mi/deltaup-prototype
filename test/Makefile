all:	1 2 5 7 8 6 3 4 9 nodiff
%:
	../diff $@.old $@.new > $@.diff
	../patch $@.old $@.diff > $@.result
	diff $@.new $@.result

.PHONY: all fail
