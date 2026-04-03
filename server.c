#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SIZE 30
#define PORT 8080
#define ACTIONS 3

typedef struct {
    int board[SIZE][SIZE];
    int turn;
    int winner; // 0: ongoing, 1: P1, 2: P2, 3: Draw
} GameState;

typedef struct {
    int x[ACTIONS], y[ACTIONS];
    int action_type[ACTIONS]; // 1: Place/Convert, 2: Remove Opponent
} Move;

void calculate_generation(GameState *gs) {
    int next[SIZE][SIZE] = {0};
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            int p1_n = 0, p2_n = 0;
            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i == 0 && j == 0) continue;
                    int nr = (r + i + SIZE) % SIZE;
                    int nc = (c + j + SIZE) % SIZE;
                    if (gs->board[nr][nc] == 1) p1_n++;
                    else if (gs->board[nr][nc] == 2) p2_n++;
                }
            }
            int total = p1_n + p2_n;
            if (gs->board[r][c] > 0) {
                next[r][c] = (total == 2 || total == 3) ? gs->board[r][c] : 0;
            } else {
                if (total == 3) next[r][c] = (p1_n > p2_n) ? 1 : 2;
            }
        }
    }
    memcpy(gs->board, next, sizeof(next));
}

int main() {
    int sfd, c1, c2;
    struct sockaddr_in addr = {AF_INET, htons(PORT), INADDR_ANY};
    GameState gs = {0};

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    bind(sfd, (struct sockaddr*)&addr, sizeof(addr));
    listen(sfd, 2);

    printf("Waiting for players...\n");
    c1 = accept(sfd, NULL, NULL);
    printf("Player 1 connected.\n");
    c2 = accept(sfd, NULL, NULL);
    printf("Player 2 connected. Starting game...\n");

    while (1) {
        send(c1, &gs, sizeof(GameState), 0);
        send(c2, &gs, sizeof(GameState), 0);

        Move m1, m2;
        recv(c1, &m1, sizeof(Move), 0);
        recv(c2, &m2, sizeof(Move), 0);

        // Apply Player 1 moves
        for(int i=0; i<ACTIONS; i++) gs.board[m1.x[i]][m1.y[i]] = (m1.action_type[i] == 1) ? 1 : 0;
        // Apply Player 2 moves
        for(int i=0; i<ACTIONS; i++) gs.board[m2.x[i]][m2.y[i]] = (m2.action_type[i] == 1) ? 2 : 0;

        calculate_generation(&gs);
        gs.turn++;

        // Simple win check: count cells
        int count1 = 0, count2 = 0;
        for(int i=0; i<SIZE*SIZE; i++) {
            if (((int*)gs.board)[i] == 1) count1++;
            if (((int*)gs.board)[i] == 2) count2++;
        }
        if (gs.turn > 5 && (count1 == 0 || count2 == 0)) {
            gs.winner = (count1 > count2) ? 1 : 2;
            send(c1, &gs, sizeof(GameState), 0);
            send(c2, &gs, sizeof(GameState), 0);
            break;
        }
    }
    close(sfd);
    return 0;
}
