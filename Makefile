EXE:=hello.tap

all:	${EXE}

${EXE}:	hello.c $(wildcard *.h)
	@echo "[CC] " $<
	@zcc +zx -lndos -create-app -O3 -o hello $< -lm
	@rm -f hello hello_BANK_7.bin zcc_opt.def
	@echo "[LD] " $@

run:	${EXE}
	fuse $<

clean:
	@echo "[CLEAN]"
	@rm -rf ${EXE}
