struct Position {
char player; //The player to make his move. White = 0, Black = 1
char wcastle;// Castle right for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
char bcastle;// same but for black
char board[8][8];
};