#include <stdio.h>
#include <stdint.h>
uint64_t zobrist_pieces[64][12];
uint64_t zobrist_castling[16];    
uint64_t zobrist_enpassant[64];   
uint64_t zobrist_side;

int pieceToIndex(const char piece) {
  switch (piece) {
    case 'P': return 0;  case 'p': return 1;
    case 'N': return 2;  case 'n': return 3;
    case 'B': return 4;  case 'b': return 5;
    case 'R': return 6;  case 'r': return 7;
    case 'Q': return 8;  case 'q': return 9;
    case 'K': return 10; case 'k': return 11;
    default: return -1;
  }
}

void initZobrist() {
    uint64_t seed = 1070372;
    
    for (int sq = 0; sq < 64; sq++) {
        for (int piece = 0; piece < 12; piece++) {
            seed = seed * 1103515245 + 12345;
            zobrist_pieces[sq][piece] = seed;
        }
    }
    //castling
    for (int i = 0; i < 16; i++) {
        seed = seed * 1103515245 + 12345;
        zobrist_castling[i] = seed;
    }
    
    //en passant
    for (int i = 0; i < 64; i++) {
        seed = seed * 1103515245 + 12345;
        zobrist_enpassant[i] = seed;
    }
    
    seed = seed * 1103515245 + 12345;
    zobrist_side = seed;
}

uint64_t hashPosition(const struct Position* pos) { //convert position to zobrist hash
    uint64_t hash = 0;
    
    //pieces
    for (int file = 0; file < 8; file++) {
        for (int rank = 0; rank < 8; rank++) {
            char piece = pos->board[file][rank]; 
            if (piece != 'X') {
                int square = rank * 8 + file;
                int pieceIdx = pieceToIndex(piece);
                if (pieceIdx >= 0) {
                    hash ^= zobrist_pieces[square][pieceIdx];
                }
            }
        }
    }
    //player
    if (pos->player == 1) {
        hash ^= zobrist_side;
    }
    //castling
    int castling_key = pos->wcastle * 4 + pos->bcastle;
    hash ^= zobrist_castling[castling_key];
    
    //ep target
    if (pos->enpassant.valid) {
        int ep_square = pos->enpassant.square.y * 8 + pos->enpassant.square.x;
        hash ^= zobrist_enpassant[ep_square];
    }
    
    return hash;
}


struct fastHashTable {
    uint64_t* table;
    size_t size;      
    size_t mask;      
    size_t entries;   
};

//create fast hash 
struct fastHashTable* createFastHashTable(size_t size_mb) {
    struct fastHashTable* ht = malloc(sizeof(struct fastHashTable));
    
    size_t entries = (size_mb * 1024 * 1024) / sizeof(uint64_t);
    size_t size = 1;
    while (size < entries) {
        size *= 2;
    }
    
    ht->size = size;
    ht->mask = size - 1;
    ht->table = calloc(size, sizeof(uint64_t));
    ht->entries = 0;
    
    return ht;
}

void freeFastHashTable(struct fastHashTable* ht) {
    if (ht) {
        free(ht->table);
        free(ht);
    }
}

void clearFastHashTable(struct fastHashTable* ht) {
    memset(ht->table, 0, ht->size * sizeof(uint64_t));
    ht->entries = 0;
}

//fast hash check with linear probing
int containsFastHash(const struct fastHashTable* ht, const uint64_t hash) {
    if (hash == 0) return 0; // Reserve 0 as "empty" marker
    
    size_t index = hash & ht->mask;
    
    //linear probing
    for (int i = 0; i < 8; i++) {
        size_t probe_index = (index + i) & ht->mask;
        
        if (ht->table[probe_index] == 0) {
            return 0; // Empty slot = not found
        }
        if (ht->table[probe_index] == hash) {
            return 1; // Found it!
        }
    }
    return 0; //Not found
}

//add hash to hashtable
void addFastHash(struct fastHashTable* ht, uint64_t hash) {
    if (hash == 0) return; // Skip zero hashes
    
    size_t index = hash & ht->mask;
    
    //find empty slot or existing hash
    for (int i = 0; i < 8; i++) {
        size_t probe_index = (index + i) & ht->mask;
        
        if (ht->table[probe_index] == 0) {
            //if (empty slot) -> add new hash
            ht->table[probe_index] = hash;
            ht->entries++;
            return;
        }
        if (ht->table[probe_index] == hash) {
            //hash already exists
            return;
        }
    }
    
    //all probe slots full
    ht->table[index] = hash;
}