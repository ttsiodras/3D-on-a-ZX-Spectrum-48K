hello.tap:	hello.c $(wildcard *.h)
	zcc +zx -lndos -create-app -O3 -o hello $< -lm

run:	hello.tap
	fuse $<
