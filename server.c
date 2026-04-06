#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <poll.h>
#include <netinet/in.h>

#define SIZE 20
#define PORT 54749
#define ACTIONS 3

typedef struct {
    int board[SIZE][SIZE];
    int turn;
    int winner; // 0: ongoing, 1: P1, 2: P2, 3: Draw
} GameState;

typedef struct {
    int x, y;
    int action_type; // 1: Place, 2: Remove
} Move;

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

void process_turn(GameState *gs, struct pollfd *fds, Move moves[3][ACTIONS], int *num_moves) {
    for (int p = 1; p < 3; ++p) {
        for (int i = 0; i < num_moves[p]; ++i) {
            int x = moves[p][i].x;
            int y = moves[p][i].y;
            gs->board[y][x] = (moves[p][i].action_type == 1) ? p : 0; // possible oob for aray
        }
        num_moves[p] = 0;
    }
    calculate_generation(gs);
    gs->turn++;
    send(fds[1].fd, gs, sizeof(GameState), 0);
    send(fds[2].fd, gs, sizeof(GameState), 0);
}

int main() {
    struct sockaddr_in addr = {AF_INET, htons(PORT), INADDR_ANY};
    memset(&(addr.sin_zero),0 ,8);
    GameState gs = {0};
    struct pollfd fds[3];
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
    fds[0].fd = listener;
    fds[0].events = POLLIN;
    int active_fds = 1;
    printf("Waiting for players on port %d... \n", PORT);
    Move moves[3][ACTIONS];
    int num_moves[3] = {0};
    time_t last_turn = time(NULL);
    while (1) {
        time_t now = time(NULL);
        int time_passed = (int)(now - last_turn);
        int time_rem = (30 - time_passed) * 1000;
        if (time_rem < 0) {
            time_rem = 0;
        }

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
                   } else if (bytes == 0) {
                       printf("Player %d disconnected.\n", i);
                       close(fds[i].fd);
                   } else {
                       printf("Received move from player %d.\n", i);
                       if (num_moves[i] < ACTIONS) {
                           moves[i][num_moves[i]] = m;
                           num_moves[i]++;
                       } else {
                           printf("Rejected excess move from player %d.\n", i);
                       }
                   }
               }
            }
        }
    }
    return 0;
}