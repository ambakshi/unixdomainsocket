all: prog

prog: prog.c

.PHONY: clean

clean:
	$(RM) prog
