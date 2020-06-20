hello.tap:	hello.c $(wildcard *.h)
	zcc +zx -lndos -create-app -o hello $< -lm

run:	hello.tap
	fuse $<
