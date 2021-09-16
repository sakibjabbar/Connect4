#ifndef CONNECT4_H
#define CONNECT4_H

/* Constants */
#define WIDTH    7
#define HEIGHT   6
#define STRAIGHT 4

#define EMPTY   ' '
#define X	    'X'
#define O       'O'

#define WGRID    5
#define HGRID    3

#define RSEED 876545678

// Gameboard
typedef char conn4[HEIGHT][WIDTH];

// Functions
void print_config(conn4 board);
void init_empty(conn4 board);
int do_move(conn4 board, int col, char symbol);
void undo_move(conn4 board, int col);
int get_move(conn4 board);
int move_possible(conn4 board);
char winner_found(conn4 board);
int rowformed(conn4 board,  int r, int c);
int check(conn4 board, int r_fix, int c_fix, int r_off, int c_off);
int suggest_move(conn4 board, char symbol);

#endif