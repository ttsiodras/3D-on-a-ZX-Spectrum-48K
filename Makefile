# This build depends on z88dk.
# If it's not packaged in your distribution, do this:
#
# mkdir -p ~/Github/
# cd ~/Github/
# git clone https://github.com/z88dk/z88dk/
# cd z88dk
# git submodule init
# git submodule update
# ./build.sh
#
# You can now use the cross compiler - just put these in your
# enviroment (e.g. in your .profile):
#
# export PATH=$HOME/Github/z88dk/bin:$PATH
# export ZCCCFG=$HOME/Github/z88dk/lib/config

ASM_TAP:=tap/statue.tap
C_TAP:=tap/statue_C.tap
SCALING_FACTOR?=8960

Q=@
ifeq ($V,1)
Q=
endif

MASK_BIN:=codegen/mask.bin
SINCOS_BIN:=codegen/sincos.bin
SCROFS_BIN:=codegen/scr_ofs.bin
POINTS_BIN:=codegen/points.bin
POINTS_CNT_BIN:=codegen/points_count.bin
RECIP_BIN:=codegen/recip.bin

ALL_TABLES:=codegen/tables.asm
ALL_DEPS:=${ALL_TABLES} ${MASK_BIN} ${SCROFS_BIN} ${POINTS_BIN} ${POINTS_CNT_BIN} ${SINCOS_BIN} ${RECIP_BIN}

ASM_FLAGS:=+zx -lndos -create-app -O3 
C_FLAGS:=+zx -lndos -create-app -DIN_C -O2 --opt-code-speed=all -Cc-unsigned 
LD_FLAGS:=-lm -m --list

all:	${ASM_TAP}

${MASK_BIN} ${SINCOS_BIN} ${SCROFS_BIN}: codegen/tables_gen.py
	${Q}echo "[CODEGEN] " $@
	${Q}cd codegen ; python3 $(notdir $<)

${POINTS_BIN} ${POINTS_CNT_BIN}: codegen/points_gen.py codegen/statue_data.py
	${Q}echo "[CODEGEN] " $@
	${Q}cd codegen ; python3 $(notdir $<) ${SCALING_FACTOR}

${RECIP_BIN}: codegen/recip_gen.py
	${Q}echo "[CODEGEN] " $@
	${Q}cd codegen ; python3 $(notdir $<)

${ASM_TAP}:	src/statue.c $(wildcard src/*h) ${ALL_DEPS}
	${Q}echo "[CC] " $<
	${Q}mkdir -p tap
	${Q}zcc ${ASM_FLAGS} -o $(basename $@) $< ${ALL_TABLES} ${LD_FLAGS}
	${Q}echo "[LD] " $@


${C_TAP}:	src/statue.c $(wildcard src/*.h) ${ALL_DEPS}
	${Q}echo "[CC] " $<
	${Q}mkdir -p tap
	${Q}zcc ${C_FLAGS} -o $(basename $@) $< ${ALL_TABLES} ${LD_FLAGS}
	${Q}echo "[LD] " $@

tap/benchy.tap: src/benchy.c ${RECIP_BIN}
	${Q}echo "[CC] " $<
	${Q}zcc ${ASM_FLAGS} -o $(basename $@) $< ${ALL_TABLES} ${LD_FLAGS}
	${Q}echo "[LD] " $@

run:	${ASM_TAP}
	fuse -g tv3x $<

run_C:	${C_TAP}
	fuse -g tv3x $<

run_benchy:	benchy
	fuse -g tv3x $<.tap

clean:
	${Q}echo "[CLEAN]"
	${Q}rm -rf tap codegen/*.bin codegen/*lis src/*lis
	${Q}rm -f mask.bin scr_ofs.bin points.bin points_count.bin sincos.bin recip.bin
	${Q}rm -rf codegen/__pycache__

# This apparently messes up the FPS counting (i.e. the "clock()" calls).
# It does appear to be a bit faster than "optimize1", 6.1 fps maybe.
optimized_C:	src/statue.c $(wildcard src/*.h)
	${Q}echo "[CC] " $<
	${Q}zcc +zx -compiler=sdcc -SO3 -DIN_C -lndos -create-app -O2 --opt-code-speed=all -Cc-unsigned -o statue_C $< -lmath48
	${Q}rm -f statue_C *.bin zcc_opt.def
	${Q}echo "[LD] " $@
