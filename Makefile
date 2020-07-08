EXE:=hello.tap

Q=@
ifeq ($V,1)
Q=
endif

all:	${EXE}

${EXE}:	hello.c $(wildcard *.h)
	${Q}echo "[CC] " $<
	${Q}zcc +zx -lndos -create-app -O3 -o hello $< -lm
	${Q}rm -f hello hello_BANK_7.bin zcc_opt.def
	${Q}echo "[LD] " $@

run:	${EXE}
	fuse --speed 1000 $<

clean:
	${Q}echo "[CLEAN]"
	${Q}rm -rf ${EXE}
