#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SIZE 30
#define PORT 8080
#define ACTIONS 3

typedef struct { int board[SIZE][SIZE]; int turn; int winner; } GameState;
typedef struct { int x[ACTIONS], y[ACTIONS]; int action_type[ACTIONS]; } Move;

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
    struct sockaddr_in serv_addr = {AF_INET, htons(PORT)};
    GameState gs;
    char *ip = (argc > 1) ? argv[1] : "127.0.0.1";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);
    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    while (1) {
        recv(sock, &gs, sizeof(GameState), 0);
        draw_board(&gs);

        if (gs.winner != 0) {
            printf("Game Over! Player %d Wins!\n", gs.winner);
            break;
        }

        Move m;
        printf("Enter %d moves (type x y): \n", ACTIONS);
        for (int i = 0; i < ACTIONS; i++) {
            printf("Move %d Action (1:Place/Convert, 2:Remove): ", i+1);
            scanf("%d %d %d", &m.action_type[i], &m.x[i], &m.y[i]);
            m.x[i] %= SIZE; m.y[i] %= SIZE; // Bounds check
        }
        send(sock, &m, sizeof(Move), 0);
        printf("Waiting for opponent...\n");
    }
    close(sock);
    return 0;
}
