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

EXE:=statue.tap
EXE_C:=statue_C.tap

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

${EXE_C}:	statue.c $(wildcard *.h)
	${Q}echo "[CC] " $<
	${Q}zcc +zx -lndos -create-app -DIN_C -O2 --opt-code-speed=all -Cc-unsigned -o statue_C $< -lm
	${Q}rm -f statue_C *.bin zcc_opt.def
	${Q}echo "[LD] " $@

run:	${EXE}
	fuse $<

run_C:	${EXE_C}
	fuse $<

clean:
	${Q}echo "[CLEAN]"
	${Q}rm -rf ${EXE} ${EXE_C} *.bin

# This apparently messes up the FPS counting (i.e. the "clock()" calls).
# It does appear to be a bit faster than "optimize1", 6.1 fps maybe.
optimized_C:	statue.c $(wildcard *.h)
	${Q}echo "[CC] " $<
	${Q}zcc +zx -compiler=sdcc -SO3 -DIN_C -lndos -create-app -O2 --opt-code-speed=all -Cc-unsigned -o statue_C statue.c -lmath48
	${Q}rm -f statue_C *.bin zcc_opt.def
	${Q}echo "[LD] " $@
