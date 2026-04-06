#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

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

/**
 * Function that draws and colors the game board. LLM assisted code.
 */
void draw_board(GameState *gs) {
    printf("\033[H\033[JTurn: %d\n", gs->turn);
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (gs->board[i][j] == 1) printf("\033[1;31m■ \033[0m"); // Red
            else if (gs->board[i][j] == 2) printf("\033[1;34m■ \033[0m"); // Blue
            else printf(". ");
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in addr = {AF_INET, htons(PORT)};
    GameState gs;
    char *ip = (argc > 1) ? argv[1] : "127.0.0.1";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    while (1) {
        recv(sock, &gs, sizeof(GameState), 0);
        draw_board(&gs);

        if (gs.winner != 0) {
            printf("Game Over! Player %d Wins!\n", gs.winner);
            break;
        }

        for (int i = 0; i < ACTIONS; i++) {
            Move m;
            printf("Move %d/3 - Enter action type (1:Place, 2:Remove), x, y: ", i+1);
            scanf("%d %d %d", &m.action_type, &m.x, &m.y);
            send(sock, &m, sizeof(Move), 0);
        }
        printf("All moves sent. Waiting for next turn...\n");
    }
    close(sock);
    return 0;
}
