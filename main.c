#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "engine.c"
#define LINE_MAX 1024
#define DEFAULT_FEN "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"


static void handle_uci() {
    printf("id name checkmate\n");
    printf("id author kuglatec\n");
    printf("uciok\n");
    fflush(stdout);
}

static void handle_isready() {
    printf("readyok\n");
    fflush(stdout);
}

static void handle_position(const char *args, struct Position *position) {
    if (strstr(args, "startpos") == args) {
        *position = FENtoPosition(DEFAULT_FEN);
        ApplyCastlingRights(DEFAULT_FEN, position);
        ApplyEnpassantTarget(DEFAULT_FEN, position);
        const char *moves = strstr(args, "moves");
        if (moves) {
            moves += strlen("moves");
            while (*moves == ' ') moves++;
            if (*moves) {
                      int sc = 1;
                      struct Move* mvs = notationsToMoves(moves, strlen(moves), &sc);
                      for (int i = 0; i < sc; i++) {

                          makeMove(position, mvs[i], getSpecialMoveType(position, mvs[i]));
                      }
                    free(mvs);
            }
        }

    } else if (strncmp(args, "fen", 3) == 0) {
        const char *moves = strstr(args, "moves");
        if (moves) {
            size_t fenlen = (size_t)(moves - args);
            char *fen = malloc(fenlen + 1);
            if (fen) {
                strncpy(fen, args, fenlen);
                fen[fenlen] = '\0';
                *position = FENtoPosition(fen + 4);
                ApplyCastlingRights(fen, position);
                ApplyEnpassantTarget(fen, position);
                free(fen);
            }
            moves += strlen("moves");
            while (*moves == ' ') moves++;
            int sc = 1;
            struct Move* mvs = notationsToMoves(moves, strlen(moves), &sc);
            for (int i = 0; i < sc; i++) {
                makeMove(position, mvs[i], getSpecialMoveType(position, mvs[i]));

            }
            free(mvs);
        } else {
            *position = FENtoPosition(args + 4);
            ApplyCastlingRights(args + 4, position);
            ApplyEnpassantTarget(args + 4, position);
        }
    }
    fflush(stdout);
}


static void handle_go(const char *args, struct Position *pos) {
    (void)args;
  //  printf("\nhash: %lu\n", hash_position(pos));

  //  struct Move bestmove = getBestMoveNegamax(pos, 5);
    //printPosition(*pos);
    struct HashTable* ht = createHashTable(1024); //1gb table size
    struct Move bestmove = /*getBestMoveWithHash(pos, 6, ht);*/getBestMoveWithKillers(pos, 8, ht);
    printf("\nhas zugzwang: %d\n", hasZugzwang(*pos));
    //struct Move bestmove = getBestMoveWithHash(pos, 5, hashTable);

    if (bestmove.start.x == -1) {
        printf("bestmove 0000\n");
    } else {
        printf("bestmove %c%d%c%d",
               bestmove.start.x + 'a', bestmove.start.y + 1,
               bestmove.end.x + 'a', bestmove.end.y + 1);

        if (bestmove.promotes) {
            printf("%c", bestmove.promotion);
        }
        printf("\n");
    }
    fflush(stdout);
}

int main(void) {
    struct Position position;
    char line[LINE_MAX];
    printf("Ready to accept UCI commands.\n");
    fflush(stdout);

    while (fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        if (len && (line[len-1] == '\n' || line[len-1] == '\r')) {
            while (len && (line[len-1] == '\n' || line[len-1] == '\r')) {
                line[--len] = '\0';
            }
        }
        if (len == 0) continue;

        if (strcmp(line, "uci") == 0) {
            handle_uci();
        } else if (strcmp(line, "isready") == 0) {
            handle_isready();
        } else if (strcmp(line, "ucinewgame") == 0) {

        } else if (strncmp(line, "setoption", 9) == 0) {

        } else if (strncmp(line, "position", 8) == 0) {
            const char *args = line + 8;
            while (*args == ' ') args++;
            handle_position(args, &position);
        } else if (strncmp(line, "go", 2) == 0) {
            const char *args = line + 2;
            while (*args == ' ') args++;
            handle_go(args, &position);
        } else if (strcmp(line, "quit") == 0) {
            break;
        } else {
            printf("info unknown command: %s\n", line);
            fflush(stdout);
        }
    }

    return 0;
}
