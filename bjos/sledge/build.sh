
gcc -g -o sledge --std=gnu99 -fsanitize=address sledge.c reader.c writer.c alloc.c blit.c strmap.c -lm

#  ./udis86/libudis86/decode.c ./udis86/libudis86/itab.c  ./udis86/libudis86/udis86.c 