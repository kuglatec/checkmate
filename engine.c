#include <string.h>
#include "structs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

const char wpromotions[4] = "QRBN";
const char bpromotions[4] = "qrbn";


uint64_t zobrist_pieces[64][12];
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

  seed = seed * 1103515245 + 12345;
  zobrist_side = seed;
}

int64_t hashPosition(const struct Position* pos) {
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

  return hash;
}

struct hashSet* createHashSet(const size_t initialCapacity) { //create the hash set
  struct hashSet* set = malloc(sizeof(struct hashSet));
  set->hashes = malloc(sizeof(uint64_t) * initialCapacity);
  set->size = 0;
  set->capacity = initialCapacity;
  return set;
}

void freeHashSet(struct hashSet* set) { //macro for freeing a hash set
  free(set->hashes);
  free(set);
}

int containsHash(const struct hashSet* set, const uint64_t hash) { //check hash set for hash
  for (size_t i = 0; i < set->size; i++) {
    if (set->hashes[i] == hash) {
      return 1; //hash found in hashset
    }
  }
  return 0;
}

void addHash(struct hashSet* set, const uint64_t hash) {
  if (set->size >= set->capacity) {
    set->capacity *= 2;
    set->hashes = realloc(set->hashes, sizeof(uint64_t) * set->capacity);
  }
  set->hashes[set->size] = hash;
  set->size++;
}

int isThreatened(struct Position position, struct Square square);

void printPosition(struct Position position);


void printTree(struct Node node, int depth) { //simple debugging function
  if (depth == 0) {
    return;
  }
  printPosition(node.position);
  printTree(node.children[0], depth - 1);
}


struct Position copyPosition(const struct Position* original) {
    struct Position copy;
    memcpy(&copy, original, sizeof(struct Position));
    return copy;
}

struct Position FENtoPosition(const char* fen) {
    struct Position position;
    // Initialize board to 'X'
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            position.board[i][j] = 'X';
        }
    }
    
    int over = 0;
    const int len = strlen(fen);
    char rank = 7;  // Start at rank 8 (index 7)
    char file = 0;  
    int field = 0; 

    for (int i = 0; i < len; i++) {
        char nextchar = fen[i];

        if (over) {
            // Process fields after board section
            switch (field) {
                case 1: 
                    position.player = (nextchar == 'w') ? 0 : 1;
                    field++;
                    break;
                // Add other field processing as needed
            }
            continue;
        }

        if (rank < 0) {
            // Finished all 8 ranks
            over = 1;
            field++;
            i--;  // Reprocess current character in field mode
            continue;
        }

        if (isalpha(nextchar)) {
            position.board[file][rank] = nextchar;
            file++;
        } else if (nextchar == '/') {
            rank--;
            file = 0;
        } else if (isdigit(nextchar)) {
            int emptySquares = nextchar - '0';
            file += emptySquares;
        } else if (nextchar == ' ') {
            over = 1;
            field++;
        }
    }

    return position;
} 

char getPiece(const struct Square sq, const struct Position* pos) {
  if (sq.x >= 0 && sq.x < 8 && sq.y >= 0 && sq.y < 8) {
    return pos->board[sq.x][sq.y];
  }
  return 'X';
}
int getSpecialMoveType(const struct Position* pos, const struct Move move) {
  const char piece = getPiece(move.start, pos);

  if (tolower(piece) == 'k' && abs(move.start.x - move.end.x) > 1) {
    return 1; //Castling
  }

  if (tolower(piece) == 'p' && move.start.x != move.end.x && getPiece(move.end, pos) == 'X') {
    return 2; //En passant
  }
  return 0;
}




void ApplyCastlingRights(const char* fen, struct Position *position) {
  int spaceCount = 0;
  position->wcastle = 0;
  position->bcastle = 0;

  for (int i = 0; fen[i] != '\0'; i++) {
    if (fen[i] == ' ') {
      spaceCount++;
      if (spaceCount == 2) {
        i++;
        if (fen[i] == '-') {
          return;
        }
        while (fen[i] != ' ') {
          switch (fen[i]) {
            case 'K': position->wcastle |= 1; break;
            case 'Q': position->wcastle |= 2; break;
            case 'k': position->bcastle |= 1; break;
            case 'q': position->bcastle |= 2; break;
          }
          i++;
        }
        return;
      }
    }
  }
}

void ApplyEnpassantTarget(const char* fen, struct Position *position) {
    int spaceCount = 0;
    position->enpassant.valid = 0;

    for (int i = 0; fen[i] != '\0'; i++) {
        if (fen[i] == ' ') {
            spaceCount++;
            if (spaceCount == 3) {
                i++;
                // Feld ist „-“ → kein EP
                if (fen[i] == '-') {
                    return;
                }
                if (fen[i] && fen[i+1] && islower((unsigned char)fen[i]) && isdigit((unsigned char)fen[i+1])) {
                    position->enpassant.square.x  = fen[i] - 'a';      
                    position->enpassant.square.y  = fen[i+1] - '1';   
                    position->enpassant.valid = 1;
                }
                return;
            }
        }
    }
}

void printMoves(const struct Move* moves, const int len, struct Position position) {
  for (int i = 0; i < len; i++) {
    printf("\n%c: %d|%d -> %d|%d", getPiece(moves[i].start, &position), moves[i].start.x, moves[i].start.y, moves[i].end.x, moves[i].end.y);
    printf("\n");
  }
}

void printSquares(const struct Square* squares, size_t len) {
  for (size_t i = 0; i < len; i++) {
    printf("\n%d|%d", squares[i].x, squares[i].y);
  }
}

void promote(struct Position* position, struct Move move) {
  position->board[move.end.x][move.end.y] = move.promotion;
}

void printPosition(const struct Position position) {
  printf("\n");
  for (int i = 7; i >= 0; i--) {
    for (int j = 0; j < 8; j++) {
      printf("%c ", (unsigned char)position.board[j][i]);
    }
    printf("\n");
  }
}

int isDiagonalMove(struct Move move) {
  return abs(move.start.x - move.end.x) == abs(move.start.y - move.end.y);
}

int isStraightMove(struct Move move) {
  return (move.start.x == move.end.x) || (move.start.y == move.end.y);
}

void addSquareVectors(struct Square* sq, struct Square direction) {
  sq->x += direction.x;
  sq->y += direction.y;
}

void multiplyVector(struct Square* vector, int factor) {
  vector->x = vector->x * factor;
  vector->y = vector->y * factor;
}





struct Path getPathDiagonal(struct Move move) {
  int dx = move.end.x - move.start.x;
  int dy = move.end.y - move.start.y;

  size_t len = abs(dx) > 1 ? abs(dx) - 1 : 0;

  struct Square direction;
  direction.x = (dx > 0) ? 1 : -1;
  direction.y = (dy > 0) ? 1 : -1;

  struct Square sq = move.start;
  struct Square* squares = calloc(len, sizeof(struct Square));

  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    squares[i] = sq;
  }

  struct Path pth;
  pth.len = len;
  pth.squares = squares;
  return pth;
}
struct Path getPathStraight(struct Move move) {
  size_t len;
  if (move.start.x == move.end.x) {
    len = abs(move.start.y - move.end.y) > 1 ? abs(move.start.y - move.end.y) - 1 : 0;
  } else {
    len = abs(move.start.x - move.end.x) > 1 ? abs(move.start.x - move.end.x) - 1 : 0;
  }
  
  struct Square direction;
  direction.x = (move.end.x > move.start.x) ? 1 : (move.end.x < move.start.x) ? -1 : 0;
  direction.y = (move.end.y > move.start.y) ? 1 : (move.end.y < move.start.y) ? -1 : 0;
  
  
  
  struct Square sq = move.start;
  
  struct Square* squares = (struct Square*)calloc(len, sizeof(struct Square));
  
  for (int i = 0; i < len; i++) {
    addSquareVectors(&sq, direction);
    squares[i] = sq;
  }
  
  struct Path pth;
  pth.squares = squares;
  pth.len = len;
  return pth;
}

int incheck(struct Position* position) {
  const int player_to_check = position->player;
  const char king_to_find = (player_to_check == 0) ? 'K' : 'k';
  struct Square kingSquare = {-1, -1}; 

  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (position->board[i][j] == king_to_find) {
        kingSquare.x = i;
        kingSquare.y = j;
        break;
      }
    }
    if (kingSquare.x != -1) break;
  }

  
  if (kingSquare.x == -1) {
    return 0; 
  }
  
  position->player = (player_to_check == 0) ? 1 : 0;

  const int threatened = isThreatened(*position, kingSquare);
  
  position->player = player_to_check;

  return threatened;
}

struct Square getCastleRook(const struct Position* position, struct Move move) {
    struct Square rook;
    int dir = (move.end.x > move.start.x) ? 1 : -1;
    int x = move.start.x + dir;
    int y = move.start.y;

    while (x >= 0 && x < 8) {
        if (tolower(position->board[x][y]) == 'r') {
            rook.x = x;
            rook.y = y;
//            printf("Rook %c position: %d|%d\n", getPiece(rook, position), rook.x, rook.y);
            return rook;
        }
        x += dir;
    }
    rook.x = -1;
    rook.y = -1;
    
    return rook;
} 


int removeCastling(const struct Position* position, struct Square rook) {
  switch (position->wcastle)
    {
    case 0:
      return 0;

    case 1: // kingside
      if (rook.x == 7) {
        return 0;
      }
      break;

    case 2: // queenside
      if (rook.x == 0) {
        return 0;
      }
      break;

    case 3: // both
      if (rook.x == 7) {
        return 2;
      }
      else if (rook.x == 0) {
        return 1;
      }
      break;
    default: printf("\nerror\n");
    }

    return 0;
}

int isValidPiece(char piece) {
  return piece == 'P' || piece == 'p' ||
         piece == 'N' || piece == 'n' ||
         piece == 'B' || piece == 'b' ||
         piece == 'R' || piece == 'r' ||
         piece == 'Q' || piece == 'q' ||
         piece == 'K' || piece == 'k' ||
         piece == '.';
}

void movePiece(struct Position* position, struct Move move) {
 /* if (isValidPiece(getPiece(move.start, position)) == 0) {

  }
  */

  char piece = getPiece(move.start, position);
  position->board[move.start.x][move.start.y] = 'X';
  position->board[move.end.x][move.end.y] = piece;
}

int epValid(const struct Position* position, const struct Move move) {
  struct Square epsquare1;
  struct Square epsquare2;
  epsquare1.x = move.end.x + 1;
  epsquare1.y = move.end.y;
  epsquare2.x = move.end.x - 1;
  epsquare2.y = move.end.y;
  if (tolower(getPiece(epsquare1, position)) == 'p') {
    return 1;
  }
  if (tolower(getPiece(epsquare2, position)) == 'p') {
    return 1;
  }
  epsquare1.x = -1;
  epsquare1.y = -1;
  return 0;

}


struct moveReturn makeMove(struct Position* position, const struct Move move, const int special) { // special: 0 = normal move, 1 = castling 2 = e.p.
  struct moveReturn ret;
  struct SquareState* states = calloc(4, sizeof(struct SquareState));
  ret.states = states;
  states[0].square = move.start;
  states[0].piece = position->board[move.start.x][move.start.y];
  states[1].square = move.end;
  states[1].piece = position->board[move.end.x][move.end.y];
  ret.enpassant = position->enpassant;
  position->enpassant.valid = 0;

  if (special == 1) { //castling
    states[2].square = getCastleRook(position, move);
    states[2].piece = position->board[states[2].square.x][states[2].square.y];
    states[3].square.x = move.start.x + ((move.end.x - move.start.x > 1) ? 1 : -1);
    states[3].square.y = move.start.y;
    states[3].piece = position->board[states[3].square.x][states[3].square.y];
    ret.len = 4;
  }
  else if (special == 2) { //en passant
    states[2].square.x = move.end.x;
    states[2].square.y = move.start.y;
    states[2].piece = position->board[states[2].square.x][states[2].square.y];
    ret.len = 3;
  }

  else {
    ret.len = 2;
  }

  if (!special){

    struct Square pc;
    pc.x = move.end.x;
    pc.y = move.end.y;
    const char pChar = getPiece(pc, position);
    if (pChar == 'P' || pChar == 'p') { // check if its a pawn
      if (isupper(pChar) && move.end.y - move.start.y == 2) { // white pawn moving 2 squares
        if (epValid(position, move)) {
          position->enpassant.square.x = move.end.x;
          position->enpassant.square.y = move.end.y - 1;
          position->enpassant.valid = 1;
        }

      }

      else if (islower(pChar) && move.start.y - move.end.y == 2) {
        if (epValid(position, move)) {
          position->enpassant.square.x = move.end.x;
          position->enpassant.square.y = move.end.y + 1;
          position->enpassant.valid = 1;
        }
      }

    }
    movePiece(position, move);
  }
  else if (special == 1) { 
    struct Square rook = getCastleRook(position, move);
    int dir = (move.end.x - move.start.x > 1);
    movePiece(position, move);

    struct Move mv2;
    mv2.start = rook;
    mv2.end.x = move.start.x + (dir ? 1 : -1);
    mv2.end.y = move.start.y;
    movePiece(position, mv2);   
  }
  else if (special == 2) { //e.p.
    movePiece(position, move);
    position->board[move.end.x][move.start.y] = 'X'; //capture pawn
}
  if (move.promotes) {
    promote(position, move);
  }
  position->player = abs(position->player - 1); //its the other players turn now
  return ret;
}

void undoMove(struct Position* position, struct moveReturn ret) {
  for (size_t i = 0; i < ret.len; i++) {
    position->board[ret.states[i].square.x][ret.states[i].square.y] = ret.states[i].piece;
  }
  position->player = abs(position->player - 1);
  position->enpassant = ret.enpassant;
  free(ret.states);
}

struct Path getPath(struct Move move, int dir) {
  if (dir == 0) {
    return getPathStraight(move);
  }
  return getPathDiagonal(move);
}

int qrsvc(const struct Position* position, struct Move move, int owner) { //Queen-Rook-Straight-Validity-Check
  
  struct Path path = getPath(move, 0);
  for (int i = 0; i < path.len; i++) {
    if (getPiece(path.squares[i], position) != 'X') {
      free(path.squares);
      return 0;
    }
  }
  free(path.squares);
  return 1;
}

int qbdvc(const struct Position* position, struct Move move, int owner) { //Queen-Bishop-diagonal-validitiy check
    struct Path path = getPath(move, 1);
    for (int i = 0; i < path.len; i++) {
   
    if (getPiece(path.squares[i], position) != 'X') {
      free(path.squares);
      return 0; 
    }
  }
  free(path.squares);
  return 1;
}


int castleCheck(struct Position position, const struct Square sq) {
  if ((getPiece(sq, &position) != 'X')) {
    return 0; 
    }

  position.player = abs(position.player - 1);

  if (isThreatened(position, sq)) {
    return 0;
  }

  return 1;
}

int kvc(struct Position position, const struct Move move, const int owner, const int skip_king_check) {
  const int opponent = (owner == 0) ? 1 : 0;

  const int original_player = position.player;
  position.player = opponent;
  if (skip_king_check == 0) {
    const int threatened = isThreatened(position, move.end);

    
    if (threatened) {
      return 0; 
    }
  }
  position.player = original_player;
  if (abs(move.start.x - move.end.x) > 1) { //castling
    struct Square rook = getCastleRook(&position, move);
    if (rook.x == -1) {
      return 0; 
    }
    const char pc = getPiece(move.start, &position);
    if (isupper(pc)) { // white
        if (position.wcastle == 0) {
          printf("\nno castling rights for white\n");
          return 0; 
        }
        if (move.end.x == 6 && move.start.x == 4 && (position.wcastle == 1 || position.wcastle == 3)) { //kingside
            for (int i = 5; i < 7; i++) {
              struct Square sq = {i, move.start.y};
              if (!castleCheck(position, sq)) {return 0;}
            }
        }
        else if (move.start.x == 4 && move.end.x == 2 && (position.wcastle == 2 || position.wcastle == 3)) { //queenside
            for (int i = 3; i > 0; i--) {
              struct Square sq = {i, move.start.y};
              if (!castleCheck(position, sq)) {return 0;}
            }
        }
    }
    else {
      //black 
      if (position.bcastle == 0) {
        printf("\nno castling rights for black\n");
        return 0; 
      }
      if (move.start.x == 4 && move.end.x == 6 && (position.bcastle == 1 || position.bcastle == 3)) { //kingside
        for (int i = 5; i < 7; i++) {
          struct Square sq = {i, move.start.y};
          if (!castleCheck(position, sq)) {return 0;}
        }
      }
      else if (move.start.x == 4 && move.end.x == 2 && (position.bcastle == 2 || position.bcastle == 3)) { //queenside
        for (int i = 3; i > 0; i--) {
          struct Square sq = {i, move.start.y};
          if (!castleCheck(position, sq)) {return 0;}
        }
      }
    }
  }

  return 1;

}



int pvc(const struct Position* position, const struct Move move, const int owner) {

  if (move.end.x == position->enpassant.square.x && move.end.y == position->enpassant.square.y) {
    return 1;
  }
  const int dir = (owner == 0) ? 1 : -1;

  if (move.start.x == move.end.x) {
    if (getPiece(move.end, position) != 'X') {
      return 0;
    }

    if (move.end.y - move.start.y == dir) {
        return 1;
    }

    if (move.end.y - move.start.y == 2 * dir) {
        int start_rank = (owner == 0) ? 1 : 6;
        if (move.start.y == start_rank) {
            struct Move singleMove = move;
            singleMove.end.y = move.start.y + dir;
            return qrsvc(position, singleMove, owner);
        }
    }
    return 0;
  }
  if ((abs(move.start.x - move.end.x) == 1) && (move.end.y - move.start.y == dir)){ // Pawn captures diagonal
    char dpiece = getPiece(move.end, position);
    if (dpiece == 'X') {
      return 0;
    }
    if (!!isupper(dpiece) == owner) {
      return 0;
    }
    return 1;
  }
  return 0;
}


int validityCheck(const struct Position* position, struct Move move, const int skip_king_check) {
  if (move.end.x < 0 || move.end.x > 7 || move.end.y < 0 || move.end.y > 7) {
    return 0; 
  }



  const char sp = position->board[move.start.x][move.start.y];
  if (sp == 'X') {
    return 0;
  }

  const int owner = !!isupper((unsigned char)sp) ? 0 : 1;
  if (owner != position->player) {
    return 0; 
  }

  const char dp = position->board[move.end.x][move.end.y];
  if (dp != 'X' && (!!isupper((unsigned char)dp) ? 0 : 1) == owner) {
      return 0; 
  }

  const char piece_type = tolower(sp);

  switch(piece_type) {
    case 'p':
      return pvc(position, move, owner);
    case 'k':
      return kvc(*position, move, owner, skip_king_check);
    case 'q':
      if (isDiagonalMove(move)) return qbdvc(position, move, owner);
      if (isStraightMove(move)) return qrsvc(position, move, owner);
      break;
    case 'b':
      if (isDiagonalMove(move)) return qbdvc(position, move, owner);
      break;
    case 'r':
      if (isStraightMove(move)) return qrsvc(position, move, owner);
      break;
    case 'n':
      // Knight move logic: L-shape (2 squares in one direction, 1 in another)
      return (abs(move.start.x - move.end.x) == 1 && abs(move.start.y - move.end.y) == 2) ||
             (abs(move.start.x - move.end.x) == 2 && abs(move.start.y - move.end.y) == 1);
    default: printf("\nerror\n");
  }
  printf("\nreturn 0\n");
  return 0;
}



int valid(struct Position* position, struct Move move, const int skip_king_check) {
  if (validityCheck(position, move, skip_king_check)) {
    if (skip_king_check == 0) {
      const int special_type = getSpecialMoveType(position, move);
      const struct moveReturn ret = makeMove(position, move, special_type);
      if (incheck(position)) {
        undoMove(position, ret);
        printf("\nMove would put player in check\n");
        return 0;
      }
      undoMove(position, ret);
      return 1;
    }
      return 1;
  }
  return 0;
}



void getDiagonalSquares(struct Path* lines, struct Square start, const struct Position* position) {
    int count = lines->len;

    for (int dir = 0; dir < 4; dir++) {
      const int dy[] = {-1, 1, -1, 1};
      const int dx[] = {-1, -1, 1, 1};
      int x = start.x + dx[dir];
        int y = start.y + dy[dir];
        
        while (x >= 0 && x < 8 && y >= 0 && y < 8 && count < 28) {
            lines->squares[count].x = x;
            lines->squares[count].y = y;
            count++;
            
            struct Square current_square = {x, y};
            if (getPiece(current_square, position) != 'X') {
                break; 
            }
            
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

void getStraightSquares(struct Path* lines, struct Square start, struct Position* position) {
    int count = lines->len;
    int dx[] = {0, 0, 1, -1}; 
    int dy[] = {1, -1, 0, 0};
    
    for (int dir = 0; dir < 4; dir++) {
        int x = start.x + dx[dir];
        int y = start.y + dy[dir];
        
        while (x >= 0 && x < 8 && y >= 0 && y < 8 && count < 28) {
            lines->squares[count].x = x;
            lines->squares[count].y = y;
            count++;

            struct Square current_square = {x, y};
            if (getPiece(current_square, position) != 'X') {
                break; 
            }
            
            x += dx[dir];
            y += dy[dir];
        }
    }
    lines->len = count;
}

void getPawnSquares(struct Path* lines, struct Square piece, struct Position* position) {
  int dir = (!!isupper(getPiece(piece, position))) ? 1 : -1;
  int count = 0;

  // 1 square forward
  int x = piece.x;
  int y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    struct Square one_forward = {x, y};
    if (getPiece(one_forward, position) == 'X') {
        lines->squares[count].x = x;
        lines->squares[count].y = y;
        count++;

        // 2 squares forward from starting position
        int start_rank = (dir == 1) ? 1 : 6;
        if (piece.y == start_rank) {
            x = piece.x;
            y = piece.y + (2 * dir);
            if (x >= 0 && x < 8 && y >= 0 && y < 8) {
                struct Square two_forward = {x, y};
                if (getPiece(two_forward, position) == 'X') {
                    lines->squares[count].x = x;
                    lines->squares[count].y = y;
                    count++;
                }
            }
        }
    }
  }

  // Capture right
  x = piece.x + 1;
  y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    lines->squares[count].x = x;
    lines->squares[count].y = y;
    count++;
  }

  // Capture left
  x = piece.x - 1;
  y = piece.y + dir;
  if (x >= 0 && x < 8 && y >= 0 && y < 8) {
    lines->squares[count].x = x;
    lines->squares[count].y = y;
    count++;
  }

  lines->len = count;
}

void getKingSquares(struct Path* lines, struct Square piece, struct Position* position) {
  lines->len = 0;
  int dx[] = {0, 0, 1, -1, 1, -1, 1, -1};
  int dy[] = {1, -1, 0, 0, 1, 1, -1, -1};
  for (int i = 0; i < 8; i++) {
    int x = piece.x + dx[i];
    int y = piece.y + dy[i];
    if (x >= 0 && x < 8 && y >= 0 && y < 8) {
      lines->squares[lines->len].x = x;
      lines->squares[lines->len].y = y;
      lines->len++;
    }
  }
  if (position->player == 0) { 
    if (position->wcastle & 1) { 
      lines->squares[lines->len].x = 6; 
      lines->squares[lines->len].y = 0;
      lines->len++;
    }
    if (position->wcastle & 2) { 
      lines->squares[lines->len].x = 2; 
      lines->squares[lines->len].y = 0;
      lines->len++;
    }
  } else { 
    if (position->bcastle & 1) { 
      lines->squares[lines->len].x = 6; 
      lines->squares[lines->len].y = 7;
      lines->len++;
    }
    if (position->bcastle & 2) { 
      lines->squares[lines->len].x = 2; 
      lines->squares[lines->len].y = 7;
      lines->len++;
    }
  }
}




void getKnightSquares(struct Path* lines, struct Square piece) {
    lines->len = 0;
    int dx[] = {1, 1, 2, 2, -1, -1, -2, -2};
    int dy[] = {2, -2, 1, -1, 2, -2, 1, -1};
    for (int i = 0; i < 8; i++) {
        int x = piece.x + dx[i];
        int y = piece.y + dy[i];
        if (x >= 0 && x < 8 && y >= 0 && y < 8) {
            lines->squares[lines->len].x = x;
            lines->squares[lines->len].y = y;
            lines->len++;
        }
    }
}

void genMoves(struct Move* moves, struct Position* position, struct Square piece, int* counter, const int skip_king_check, struct Square target) {
  struct Path lines;
  lines.squares = malloc(sizeof(struct Square) * 28);
  lines.len = 0;
  struct Move mv;

  char piece_type = tolower(getPiece(piece, position));

  switch (piece_type) {
    case 'b':
      getDiagonalSquares(&lines, piece, position);
      break;
    case 'r':
      getStraightSquares(&lines, piece, position);
      break;
    case 'p':
      getPawnSquares(&lines, piece, position);
      break;
    case 'k':
      getKingSquares(&lines, piece, position);
      break;
    case 'q':
      getStraightSquares(&lines, piece, position);
      getDiagonalSquares(&lines, piece, position);
      break;
    case 'n':
      getKnightSquares(&lines, piece);
      break;
    default: printf("\ninvalid piece\n");
  }

  for (int i = 0; i < lines.len; i++) {
    mv.start = piece;
    mv.end = lines.squares[i];

    if (validityCheck(position, mv, skip_king_check)) {

      if ((tolower(getPiece(mv.start, position)) == 'p') && (mv.end.y == 0 || mv.end.y == 7)) { //promotion
        for (int j = 0; j < 4; j++) {
          struct Move pm = mv;
          if (position->player == 0) {
            pm.promotion = wpromotions[j]; 
            pm.promotes = 1;
          } else {
            pm.promotion = bpromotions[j];
            pm.promotes = 1;
          }
          moves[*counter] = pm;
          (*counter)++;
        }
      }
      else {
        mv.promotion = 0;
        mv.promotes = 0;
        moves[*counter] = mv;
        (*counter)++;
      }
    }
  }
  
  free(lines.squares);
}



struct Move* getMoves(struct Position* position, int* counter, const int skip_king_check, struct Square target) {
  struct Move* mvs = (struct Move*)calloc(218, sizeof(struct Move));
  struct Square sq;
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      sq.x = i;
      sq.y = j;
      char piece = getPiece(sq, position);
      if (piece == 'X' || (!!isupper(piece) ? 0 : 1) != position->player) {
        continue; 
      }
      genMoves(mvs, position, sq, counter, skip_king_check, target);
    }
  }
  return mvs;
}



/*
int isThreatened(struct Position position, struct Square square) {
  
  int counter = 0;
  struct Move* moves = getMoves(&position, &counter, 1, square);
  for (int i = 0; i < counter; i++) {
    if (moves[i].end.x == square.x && moves[i].end.y == square.y) {
      free(moves);
      return 1; 
    }
  }
  free(moves);
  return 0;
}
  
 */

int isThreatened(struct Position position, struct Square square) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      struct Square pieceSquare = {i, j};
      char piece = getPiece(pieceSquare, &position);
      if (piece == 'X' || (!!isupper(piece) ? 0 : 1) == position.player) {
        continue; 
      }
      struct Move move;
      move.start = pieceSquare;
      move.end = square;
      move.promotion = 0;
      move.promotes = 0;
      if (validityCheck(&position, move, 1)) {
        return 1; 
      }
    }
  }
  return 0;
}



/*

void buildTree(struct Node* nd, int depth) {
  if (depth == 0) {
    return;
  }
  struct Position position = nd->position;
  int counter = 0;
  struct Square target;
  target.x = -1;
  target.y = -1;
  struct Move* moves = getMoves(&position, &counter, 0, target);
  nd->children = (struct Node*)malloc(sizeof(struct Node) * counter);
  nd->nchildren = counter;
  for (int i = 0; i < counter; i++) {
    nd->children[i].move = moves[i];
    nd->children[i].position = copyPosition(&position);
    makeMove(&nd->children[i].position, moves[i], getSpecialMoveType(&nd->position, moves[i]));
    buildTree(&nd->children[i], depth - 1);
  
  }
  free(moves);
}

*/
void buildTreeWithHashCheck(struct Node* nd, const int depth, struct hashSet* seen_positions) {
  if (depth == 0) {
    return;
  }

  const uint64_t posHash = hashPosition(&nd->position);

//check if position is already hashed
  if (containsHash(seen_positions, posHash)) {
    nd->nchildren = 0;
    nd->children = NULL;
    return; // Cut the branch!
  }

//add positions hash to hash set
  addHash(seen_positions, posHash);

  struct Position position = nd->position;
  int counter = 0;
  struct Square target = {-1, -1};
  struct Move* moves = getMoves(&position, &counter, 0, target);

  nd->children = (struct Node*)malloc(sizeof(struct Node) * counter);
  nd->nchildren = counter;

  for (int i = 0; i < counter; i++) {
    nd->children[i].move = moves[i];
    nd->children[i].position = copyPosition(&position);
    makeMove(&nd->children[i].position, moves[i],
             getSpecialMoveType(&nd->position, moves[i]));

    buildTreeWithHashCheck(&nd->children[i], depth - 1, seen_positions);
  }

  free(moves);
}
