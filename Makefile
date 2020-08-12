EXE:=statue.tap

Q=@
ifeq ($V,1)
Q=
endif

all:	${EXE}

${EXE}:	statue.c $(wildcard *.h)
	${Q}echo "[CC] " $<
	${Q}zcc +zx -lndos -create-app -O3 -o statue $< -lm
	${Q}rm -f statue *.bin zcc_opt.def
	${Q}echo "[LD] " $@

run:	${EXE}
	fuse $<

runfast:	${EXE}
	fuse --speed 250 $<

clean:
	${Q}echo "[CLEAN]"
	${Q}rm -rf ${EXE}
