clear
gcc src/main.c -Ofast -march=native -flto -DNDEBUG -funroll-loops -finline-functions -o engine
./engine
