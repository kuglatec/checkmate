#include <stdint.h>
#include <time.h>
static uint64_t piece_keys[8][8][12];  // [file][rank][piece_type]
static uint64_t castle_keys[16];       //castling
static uint64_t enpassant_keys[8];     //ep
static uint64_t player_key;            //player
static int zobrist_initialized = 0;

int piece_to_index(char piece) {
    switch (piece) {
    case 'P': return 0;
    case 'N': return 1;
    case 'B': return 2;
    case 'R': return 3;
    case 'Q': return 4;
    case 'K': return 5;
    case 'p': return 6;
    case 'n': return 7;
    case 'b': return 8;
    case 'r': return 9;
    case 'q': return 10;
    case 'k': return 11;
    default: return -1;
    }
}


void init_zobrist() {
    if (zobrist_initialized) return;
    
    srand((unsigned int)time(NULL));
    

    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int piece = 0; piece < 12; piece++) {
                piece_keys[file][rank][piece] = 
                    ((uint64_t)rand() << 32) | (uint64_t)rand();
            }
        }
    }
    
    for (int i = 0; i < 16; i++) {
        castle_keys[i] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    
    for (int file = 0; file < 8; file++) {
        enpassant_keys[file] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    }
    
    player_key = ((uint64_t)rand() << 32) | (uint64_t)rand();
    
    zobrist_initialized = 1;
}


uint64_t hash_position(const struct Position* pos) {
    if (!zobrist_initialized) {
        init_zobrist();
    }

    uint64_t hash = 0;

//board
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            char piece = pos->board[file][rank];
            if (piece != '.') {
                int piece_index = piece_to_index(piece);
                if (piece_index >= 0) {
                    hash ^= piece_keys[file][rank][piece_index];
                }
            }
        }
    }

//castling
    int castle_state = (pos->wcastle & 3) | ((pos->bcastle & 3) << 2);
    hash ^= castle_keys[castle_state];

//ep
    if (pos->enpassant.valid &&
        pos->enpassant.square.x >= 0 && pos->enpassant.square.x < 8) {
        hash ^= enpassant_keys[pos->enpassant.square.x];
        }

    //player
    if (pos->player == 1) {
        hash ^= player_key;
    }

    return hash;
}
