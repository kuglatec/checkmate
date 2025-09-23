#include <stdio.h>
#include <string.h>
#include "structs.h"
#include "engine.c"
#include <stdlib.h>

// Bitboard variation
int main() {
    FENtoBitboardPosition("123");
    init_zobrist();
    struct HashTable* hash_table = createHashTable(1024);


    char fen[90];
    struct PositionWBitboard position;

    //starting pos
    strcpy(fen, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //position = FENtoPosition(fen);
    position = FENtoBitboardPosition(fen);
    //ApplyCastlingRights(fen, &position);
    //ApplyEnpassantTarget(fen, &position);


    char buffer[2048];
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, buffer, _IOLBF, 2048);

    char* line = NULL;
    size_t len = 0;


    while (1) {
        if (getline(&line, &len, stdin) == -1) {
            break;
        }

        if (strncmp(line, "uci", 3) == 0) {
            printf("id name checkmate\n");
            printf("id author kuglatec\n");
            printf("uciok\n");
        } else if (strncmp(line, "isready", 7) == 0) {
            printf("readyok\n");

        } else if (strncmp(line, "ppos", 4) == 0)
        {
            printPositionBitBoard(position.board);
        }

        else if (strncmp(line, "ucinewgame", 10) == 0) {
            clearHashTable(hash_table);
            strcpy(fen, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
            position = FENtoBitboardPosition(fen);
         //   ApplyCastlingRights(fen, &position);
          //  ApplyEnpassantTarget(fen, &position);
        } else if (strncmp(line, "position", 8) == 0) {
            printf("Processing position command\n");
            if (strstr(line, "startpos")) {
                strcpy(fen, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
                position = FENtoBitboardPosition(fen);
                //position = FENtoPosition(fen);
                //ApplyCastlingRights(fen, &position);
                //ApplyEnpassantTarget(fen, &position);
            } else if (strstr(line, "fen")) {
                char* fen_ptr = strstr(line, "fen") + 4;
                strcpy(fen, fen_ptr);
                position = FENtoBitboardPosition(fen);
          //      ApplyCastlingRights(fen, &position);
          //      ApplyEnpassantTarget(fen, &position);
            }

           /* if (strstr(line, "moves")) {
                char* moves_ptr = strstr(line, "moves") + 6;
                int move_count;
                struct Move* moves = notationsToMoves(moves_ptr, strlen(moves_ptr), &move_count);
                for (int i = 0; i < move_count; i++) {
                    makeMove(&position, moves[i], getSpecialMoveType(&position, moves[i]));
                }
            }*/
        } else if (strncmp(line, "go", 2) == 0) {
            printf("Processing go command\n");
            printPositionBitBoard(position.board);
         /*   struct Move best_move = getBestMoveWithKillers(&position, 10, hash_table);
            char promo_char = best_move.promotes ? best_move.promotion : ' ';
            if (promo_char != ' ') {
              printf("bestmove %c%d%c%d%c\n", best_move.start.x + 'a', best_move.start.y + 1, best_move.end.x + 'a', best_move.end.y + 1, promo_char);
            } else {
              printf("bestmove %c%d%c%d\n", best_move.start.x + 'a', best_move.start.y + 1, best_move.end.x + 'a', best_move.end.y + 1);
            }*/
        } else if (strncmp(line, "quit", 4) == 0) {
            break;
        }
    }

    free(line);
    freeHashTable(hash_table);
    return 0;
}
