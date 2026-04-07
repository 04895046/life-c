#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <poll.h>
#include <netinet/in.h>
#include <signal.h>

#include "common.h"

/**
 * Determines result of game state with customized rules. LLM assisted code.
 */
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

void check_winner(GameState *gs) {
    int p1_count = 0;
    int p2_count = 0;

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (gs->board[i][j] == 1) {
                p1_count++;
            }
            else if (gs->board[i][j] == 2) {
                p2_count++;
            }
        }
    }

    if (gs->turn > 1) { // extinction
        if (p1_count > 0 && p2_count == 0) {
            gs->winner = 1;
        }
        else if (p2_count > 0 && p1_count == 0) {
            gs->winner = 2;
        }
        else if (p1_count == 0 && p2_count == 0) {
            gs->winner = 3;
        }
    }

    if (gs->turn >= 100 && gs->winner == 0) { // turn limit
        if (p1_count > p2_count) {
            gs->winner = 1;
        }
        else if (p2_count > p1_count) {
            gs->winner = 2;
        }
        else {
            gs->winner = 3;
        }
    }
}

void process_turn(GameState *gs, struct pollfd *fds, Move moves[3][ACTIONS], int *num_moves) {
    for (int p = 1; p < 3; ++p) {
        for (int i = 0; i < num_moves[p]; ++i) {
            int x = moves[p][i].x;
            int y = moves[p][i].y;
            if (x < 0 || x >= SIZE || y < 0 || y >= SIZE) {
                continue;
            }
            gs->board[y][x] = (moves[p][i].action_type == 1) ? p : 0; // possible oob for aray
        }
        num_moves[p] = 0;
    }
    calculate_generation(gs);
    check_winner(gs);
    printf("Next state processed. Forwarding to clients.\n");
    gs->turn++;
    for (int i = 1; i < 3; i++) {
        if (fds[i].fd != -1) {
            send(fds[i].fd, gs, sizeof(GameState), 0);
        }
    }
}

void remove_player(struct pollfd *fds, int index, int *num_moves) {
    printf("Player %d disconnected.\n", index);
    close(fds[index].fd);
    fds[index].fd = -1;
    num_moves[index] = 0;
}

int server_sock;

void handle_sigint(int sig) {
    (void) sig;
    printf("\nShutting down gracefully.\n");
    close(server_sock);
    exit(0);
}

int main() {
    // game data initialization
    GameState gs = {0};
    Move moves[3][ACTIONS];
    int num_moves[3] = {0};

    // socket initialization
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(addr.sin_zero),0 ,8);
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (bind(listener, (struct sockaddr *) &addr, sizeof(struct sockaddr_in)) == -1) {
        perror("bind");
        close(listener);
        exit(1);
    }
    if (listen(listener, 5) < 0) {
        perror("listen");
        exit(1);
    }
    server_sock = listener;

    // poll initialization
    struct pollfd fds[3];
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    int active_fds = 1;

    // timer
    time_t last_turn = time(NULL);

    printf("Waiting for players on port %d... \n", PORT);

    // main game loop
    while (1) {
        signal(SIGINT, handle_sigint);

        // timer check
        time_t now = time(NULL);
        int time_passed = (int)(now - last_turn);
        int time_rem = (30 - time_passed) * 1000;
        if (time_rem < 0) {
            time_rem = 0;
        }

        // poll loop
        int status = poll(fds, active_fds, time_rem);
        if (status < 0) {
            perror("poll");
            exit(1);
        }
        if (status == 0 || time_passed >= 30) {
            process_turn(&gs, fds, moves, num_moves);
            last_turn = time(NULL);
        }
        for (int i = 0; i < active_fds; ++i) {
            if (fds[i].revents & POLLIN) { // update occured for fd
               if (fds[i].fd == listener) { // new connection established
                   if (active_fds < 3) {
                       int new_sock = accept(listener, NULL, NULL);
                       if (new_sock == -1) {
                           perror("accept");
                           exit(1);
                       }
                       fds[active_fds].fd = new_sock;
                       fds[active_fds].events = POLLIN;
                       char notif[50];
                       if (active_fds == 1) {
                           sprintf(notif, "Ready player \033[1;31m1\033[0m\n");
                       } else {
                           sprintf(notif, "Ready player \033[1;34m2\033[0m\n");
                       }
                       send(new_sock, notif, strlen(notif), 0);
                       active_fds++;
                   }
                   printf("Player %d connected.\n", active_fds - 1);
                   if (active_fds == 3) {
                       printf("Game starting.\n");
                       process_turn(&gs, fds, moves, num_moves);
                       last_turn = time(NULL);
                   }
               } else { // player sending message
                   Move m;
                   int bytes = recv(fds[i].fd, &m, sizeof(Move), 0);
                   if (bytes < 0) {
                       perror("recv");
                       exit(1);
                   }
                   if (bytes == 0) {
                       printf("Player %d disconnected.\n", i);
                       remove_player(fds, i, num_moves);
                   } else {
                       printf("Received move from player %d.\n", i);
                       if (num_moves[i] < ACTIONS) {
                           moves[i][num_moves[i]] = m;
                           num_moves[i]++;
                       } else {
                           printf("Rejected additional move from player %d.\n", i);
                       }
                   }
               }
            }
        }
    }
    close(listener);
    return 0;
}