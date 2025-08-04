#include <stddef.h>

struct Square {
    char x;
    char y;
};

struct enPassant {
    int valid; 
    struct Square square; 
};

struct Position {
int player; //The player to make his move. Black = 0, White = 1
int wcastle; // Castle rights for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
int bcastle; // same for black
char board[8][8];
struct enPassant enpassant; //e.p. capture square
};


struct SquareState {
    struct Square square;
    char piece; 
};

struct moveReturn {
    size_t len; // The number of squares in the path
    struct SquareState* states; // The state of the square after the move
};

struct Move {
    struct Square start;
    struct Square end;
    int promotes; // 1 = yes, 0 = no
    char promotion; // Defines what the piece promotes to by char
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

