#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include <unistd.h>

#include "connect4.h"


int main(int argc, char**argv) {

	// Build connection
	int sockfd, port;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	// Game variables
	int client_move, server_move;
	conn4 board;

	// Check for correct # of args
	if (argc < 3) {

		fprintf(stderr,"usage %s hostname port\n", argv[0]);
		exit(0);
	}

	port = atoi(argv[2]); // Initialize port #

	server = gethostbyname(argv[1]); // Translate host name into peer's IP address
	
	// Make sure translation worked
	if (server == NULL) {

		fprintf(stderr,"ERROR, no such host\n");
		exit(0);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Data structures for socket

	serv_addr.sin_family = AF_INET;

	bcopy((char *)server->h_addr, 
			(char *)&serv_addr.sin_addr.s_addr,
			server->h_length);

	serv_addr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create TCP socket
	
	// Check for error
	if (sockfd < 0) {

		perror("ERROR opening socket");
		exit(0);
	}
	
	// Connect to the server address
	if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {

		perror("ERROR connecting");
		exit(0);
	}

	// Initialize board
	init_empty(board);
	print_config(board);

	// Send client move, receive server move
	while((client_move = get_move(board)) != EOF) {
	
		send(sockfd, &client_move, sizeof(client_move), 0); // Send the client move to  server

		// Process client move
		if (do_move(board, client_move, O)!=1) {
			printf("Error\n");
			exit(EXIT_FAILURE);
		}

		// Print current board
		printf("Users move is: %d \n", client_move);
		print_config(board);

		// If client wins
		if (winner_found(board) == O) {
			
			printf("You win!\n");
			exit(EXIT_SUCCESS);
		}

		// Recieve move from server
		recv(sockfd, &server_move, sizeof(server_move), 0);

		printf("Servers move is: %d \n", server_move);

		if (do_move(board, server_move, X)!=1) {
			printf("Error\n");
			exit(EXIT_FAILURE);
		}
		
		print_config(board); // Print board
 
		// If server wins
		if (winner_found(board) == X) {
			
			printf("I win!\n");
			exit(EXIT_SUCCESS);
		}

	}

	return 0;
}


// Initialize the playing array to empty cells
void init_empty(conn4 board) {

	int r, c;
	for (r=HEIGHT-1; r>=0; r--) {
		for (c=0; c<WIDTH; c++) {
			board[r][c] = EMPTY;
		}
	}
}

// Apply move to board
int do_move(conn4 board, int c, char colour) {

	int r=0;
	
	while ((r<HEIGHT) && (board[r][c-1]!=EMPTY)) {
		r += 1;
	}
	if (r==HEIGHT) {

		return 0;
	}

	board[r][c-1] = colour;
	return 1;
}

// Undo move
void undo_move(conn4 board, int c) {

	int r=0;

	while ((r<HEIGHT) && board[r][c-1] != EMPTY) {
		r += 1;
	}
	/* then do the assignment, assuming that r>=1 */
	board[r-1][c-1] = EMPTY;
	return;
}

// If board is full
int move_possible(conn4 board) {
	int c;

	for (c=0; c<WIDTH; c++) {
		if (board[HEIGHT-1][c] == EMPTY) {

			return 1;
		}
	}

	return 0;
}


// check move
int get_move(conn4 board) {

	int c;

	if (!move_possible(board)) {
		return EOF;
	}

	printf("Enter column number: ");
	if (scanf("%d", &c) != 1) {
		return EOF;
	}

	while ((c<=0) || (c>WIDTH) || (board[HEIGHT-1][c-1]!=EMPTY)) {
		printf("Move is not possible. \n");
		printf("Enter column number: ");
		if (scanf("%d", &c) != 1) {
			return EOF;
		}
	}

	return c;
}

// Print board
void print_config(conn4 board) {

	int r, c, i, j;

	printf("\n");

	for (r=HEIGHT-1; r>=0; r--) {
		for (i=0; i<HGRID; i++) {
			printf("\t|");

			for (c=0; c<WIDTH; c++) {
				for (j=0; j<WGRID; j++) {
					printf("%c", board[r][c]);
				}
				printf("|");
			}
			printf("\n");
		}
	}

	printf("\t+");
	for (c=0; c<WIDTH; c++) {
		for (j=0; j<WGRID; j++) {
			printf("-");
		}
		printf("+");
	}
	printf("\n");

	printf("\t ");
	for (c=0; c<WIDTH; c++) {
		for (j=0; j<(WGRID-1)/2; j++) {
			printf(" ");
		}
		printf("%1d ", c+1);
		for (j=0; j<WGRID-1-(WGRID-1)/2; j++) {
			printf(" ");
		}
	}
	printf("\n\n");
}

// If winner found
char winner_found(conn4 board) {

	int r, c;

	for (r=0; r<HEIGHT; r++) {
		for (c=0; c<WIDTH; c++) {
			if ((board[r][c]!=EMPTY) && rowformed(board,r,c)) {
				return board[r][c];
			}
		}
	}

	return EMPTY;
}


// check for row
int rowformed(conn4 board, int r, int c) {
	
	return
		check(board, r, c, +1,  0) ||
		check(board, r, c, -1,  0) ||
		check(board, r, c,  0, +1) ||
		check(board, r, c,  0, -1) ||
		check(board, r, c, -1, -1) ||
		check(board, r, c, -1, +1) ||
		check(board, r, c, +1, -1) ||
		check(board, r, c, +1, +1);
}
// check function
int check(conn4 board, int r_fix, int c_fix, int r_off, int c_off) {

	int r_lim, c_lim;
	int r, c, i;
	r_lim = r_fix + (STRAIGHT-1)*r_off;
	c_lim = c_fix + (STRAIGHT-1)*c_off;

	if (r_lim<0 || r_lim>=HEIGHT || c_lim<0 || c_lim>=WIDTH) {

		return 0;
	}

	for (i=1; i<STRAIGHT; i++) {
		r = r_fix + i*r_off;
		c = c_fix + i*c_off;
		if (board[r][c] != board[r_fix][c_fix]) {

			return 0;
		}
	}

	return 1;
}

// Suggest move
int suggest_move(conn4 board, char colour) {

	int c;

	for (c=0; c<WIDTH; c++) {
	
		if (do_move(board, c+1, colour)) {
		
			if (winner_found(board) == colour) {
				
				undo_move(board, c+1);
				return c+1;
			} else {
				undo_move(board, c+1);
			}
		}
	}
	
	if (colour == X) {
		colour = O;
	} else {
		colour = X;
	}
	for (c=0; c<WIDTH; c++) {

		if (do_move(board, c+1, colour)) {
		
			if (winner_found(board) == colour) {
			
				undo_move(board, c+1);
				return c+1;
			} else {
				undo_move(board, c+1);
			}
		}
	}

	c = rand()%WIDTH;
	while (board[HEIGHT-1][c]!=EMPTY) {
		c = rand()%WIDTH;
	}
	return c+1;
}

