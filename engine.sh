clear
gcc main.c -O3 -march=native -flto -DNDEBUG -funroll-loops -finline-functions -o engine
time ./engine