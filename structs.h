#include <stddef.h>
struct Position {
int player; //The player to make his move. White = 0, Black = 1
int wcastle; // Castle rights for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
int bcastle; // same for black
char board[8][8];
};


struct Square {
    char x;
    char y;
};

struct Move {
    struct Square start;
    struct Square end;
    char promotion; // Defines what the piece promotes to 
};

struct Node {
    struct Position position;
    struct Move move;
    int nchildren;
    struct Node *children;
};

struct Path {
    size_t len;
    struct Square* squares;
};

