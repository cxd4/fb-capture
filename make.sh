src="."
obj="$src/obj"

mkdir -p $obj

C_FLAGS="\
    -DWIDTH=1024 -DHEIGHT=768 \
    -Os \
    -march=native \
    -ansi \
    -Wall \
    -pedantic"

echo Compiling C source code...
cc -S $C_FLAGS -o $obj/main.s    $src/main.c

echo Assembling compiled sources...
as -o $obj/main.o $obj/main.s

echo Linking assembled object files...
gcc -o $obj/fbcapt -s $obj/main.o
