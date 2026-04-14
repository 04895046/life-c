#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <poll.h>
#include <signal.h>

#include "common.h"

/**
 * Function that draws and colors the game board.
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
    printf("\n\033[1;32mEnter up to 3 moves - action type (1: Place, 2: Remove), x, y:\033[0m\n");
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    GameState gs;
    char *ip = (argc > 1) ? argv[1] : "127.0.0.1";

    sock = socket(AF_INET, SOCK_STREAM, 0);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));

    struct pollfd client_fds[2];
    client_fds[0].fd = STDIN_FILENO;
    client_fds[0].events = POLLIN;
    client_fds[1].fd = sock;
    client_fds[1].events = POLLIN;
    int num_moves = 0;
    printf("Waiting for game start.\n");

    char notif[50];
    memset(notif, 0, sizeof(notif));
    if (recv(sock, notif, sizeof(notif) - 1, 0) < 0) {
        perror("recv");
        exit(1);
    }
    printf("%s", notif);
    while (1) {
        poll(client_fds, 2, -1);

        if (client_fds[1].revents & POLLIN) { // new state incoming
            if (recv(sock, &gs, sizeof(GameState), 0) <= 0) {
                printf("\n\033[1;32mServer disconnected. Exiting.\033[0m\n");
                close(sock);
                exit(1);
            }
            draw_board(&gs);
            num_moves = 0;

        }

        if (client_fds[0].revents & POLLIN) {
            char buf[100];
            if (fgets(buf, sizeof(buf), stdin)) {
                if (num_moves < ACTIONS) {
                    Move m;
                    if (sscanf(buf, "%d %d %d", &m.action_type, &m.x, &m.y) == 3) {
                        send(sock, &m, sizeof(Move), 0);
                        num_moves++;
                        if (num_moves == ACTIONS) {
                            printf("\033[1;32mAll moves sent. Waiting for next state.\033[0m\n");
                        }
                    } else {
                        printf("\033[1;32mInvalid format. Expected: action x y\033[0m\n");
                    }
                } else {
                    printf("\033[1;32mMove limit for this turn exceeded.\033[0m\n");
                }
            }
        }
        if (gs.winner != 0) {
            printf("\033[1;32mGame Over! Player %d Wins!\033[0m\n", gs.winner);
            break;
        }
    }
    close(sock);
    return 0;
}
