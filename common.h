//
// Created by james on 4/6/2026.
//

#ifndef COMMON_H
#define COMMON_H

#define SIZE 20
#define PORT 54749
#define ACTIONS 3

typedef struct {
    int board[SIZE][SIZE];
    int turn;
    int winner;
} GameState;

typedef struct {
    int x, y;
    int action_type;
} Move;

#endif //LIFE_C_COMMON_H