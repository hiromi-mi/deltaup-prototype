all:	1 2 3
1:
	../diff 1.old 1.new > 1.diff
	../patch 1.old 1.diff > 1.result
	diff 1.new 1.result
2:
	../diff 2.old 2.new > 2.diff
	../patch 2.old 2.diff > 2.result
	diff 2.new 2.result
3:
	../diff 3.old 3.new > 3.diff
	../patch 3.old 3.diff > 3.result
	diff 3.new 3.result
