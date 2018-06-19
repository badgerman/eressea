.PHONY: clean tags test

all:
	s/build

tags:
	ctags -R src scripts

test:
	s/runtests

clean:
	@rm -f *.log.*
	@find . -name "*~" | xargs rm -f
