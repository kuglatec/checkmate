#ifndef CHESS2_STRUCTS_H
#define CHESS2_STRUCTS_H

#include <stdint.h>
struct Square {
    char x;
    char y;
};

struct enPassant {
    int valid; 
    struct Square square; 
};




struct polyglotEntry
{
    uint64_t hash;
    uint16_t move;
    uint16_t weight;
};

struct Position {
    char wcastled;
    char bcastled;
    int player; //The player to make his move. Black = 1, White = 0
    int wcastle; // Castle rights for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
    int bcastle; // same for black
    char board[8][8];
    struct enPassant enpassant; //e.p. capture square
};

enum {
    WHITE_PAWN = 0, WHITE_ROOK = 1, WHITE_KNIGHT = 2,
    WHITE_BISHOP = 3, WHITE_QUEEN = 4, WHITE_KING = 5,
    BLACK_PAWN = 6, BLACK_ROOK = 7, BLACK_KNIGHT = 8,
    BLACK_BISHOP = 9, BLACK_QUEEN = 10, BLACK_KING = 11
};

struct MoveBB {
    uint16_t move;
};



struct PositionWBitboard
{
    char player;
    char wcastled;
    char bcastled;
    int wcastle; // Castle rights for white. 0 = no castling, 1 = kingside, 2 = queenside 3 = both.
    int bcastle; // same for black
    int enpassant; //en passant: index for EP square, -1 if none
    uint64_t board[12]; //bitboards for each piece
};

struct SquareState {
    struct Square square;
    char piece; 
};

struct hashSet {
    uint64_t* hashes;
    size_t size;
    size_t capacity;
};

struct Move {
    struct Square start;
    struct Square end;
    int promotes; // 1 = yes, 0 = no
    char promotion; // Defines what the piece promotes to by char
};
struct moveReturn {
    size_t len; // The number of squares in the path
    struct SquareState states[4];
    struct enPassant enpassant; // original en pHashassant struct
    char wcastled;
    char bcastled;
};
struct HashEntry {
    uint64_t hash;          // Zobrist hash of the position
    int depth;              // Search depth when this was stored
    int score;              // Evaluation score
    int type;               // HASH_EXACT, HASH_LOWER_BOUND, or HASH_UPPER_BOUND
    struct Move bestmove;   // Best move found for this position
    int age;                // Age counter to manage replacement
};

struct HashTable {
    struct HashEntry* entries;  // Array of hash entries
    size_t len;                 // Number of entries (should be power of 2)
    int current_age;            // Current search age
};

struct Path {
    size_t len;
    struct Square* squares;
};

struct KillerTable
{
    struct Move killers[64][2];
};

struct Game {
    struct Position position;
    struct Move moves[1024];
    int move_count;
};

#endif //CHESS2_STRUCTS_H
