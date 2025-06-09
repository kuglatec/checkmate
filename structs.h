struct Position {
char player; //The player to make his move. White = 0, Black = 1
char wcastle;// Castle rights for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
char bcastle;// same for black
char board[8][8];
};

struct Move {
    char start[2];
    char end[2];
};

struct Square {
    char x;
    char y;
};

struct Node {
    struct Position position;
    struct Move move;
    int nchildren;
    struct Node *children;
};
