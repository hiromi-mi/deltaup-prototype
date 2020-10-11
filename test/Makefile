all:
	../diff 1.old 1.new > 1.diff
	../patch 1.old 1.diff > 1.result
	diff 1.diff 1.result
