#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include "connect4.h"
#include "server.h"

// Outcome codes
#define STATUS_UNFINISHED   1
#define STATUS_ILLEGAL_MOVE 2
#define STATUS_USER_WON     3
#define STATUS_AI_WON       4
#define STATUS_DRAW         5

// For thread processing
pthread_mutex_t lock;

int main(int argc, char **argv) {

	int sockfd, port, clilen;

	struct sockaddr_in serv_addr, cli_addr; // Client/server address info

	conn_t *connection; // Pass connection info to thread

	FILE *logfile; // Server logfile

	pthread_t tid; // Thread ID

	// Initialize mutex lock
	if (pthread_mutex_init(&lock, NULL) != 0) {

        printf("\n mutex init failed\n");
        exit(1);
    }	
	
	// Open logfile
	logfile = fopen("log.txt", "w");

	if(logfile == NULL) {

		printf("Error opening the logfile \n");
		exit(0);
	}

	// Input should have 2 arguments: <executable>, <port #>
	if (argc < 2) {
		fprintf(stderr,"ERROR: No port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create TCP socket

	// If socket opening successful
	if (sockfd < 0) {

		perror("ERROR opening socket");
		exit(1);
	}

	bzero((char *) &serv_addr, sizeof(serv_addr)); // Zero bytes in server address

	port = atoi(argv[1]); // Initialize port #

	// Create address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	serv_addr.sin_port = htons(port); // Store Port in machine-neutral format

	// Bind address and socket
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr))<0) {

		perror("ERROR in binding");
		exit(1);
	}

	// Incoming client connections
	listen(sockfd,5);
	clilen = sizeof(struct sockaddr_in);

	// Accept  connections and play game
	while(1) {

		connection = (conn_t *)malloc(sizeof(conn_t)); // Connection data structure to pass to thread

		connection->socket = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); // Assign new socket to connection

		// For printing client/server addresses
		connection->client_address = cli_addr;
		connection->serv_address = serv_addr;

		// Check for socket
		if (connection->socket <= 0) {
	        
			free(connection);
        }
        else {
	    	
			// New log entry and connection
	    	fopen("log.txt","a+");
			log_connection(logfile, connection->client_address, connection->socket);

	        // New thread to handle game + continue running
	        pthread_create(&tid, 0, thread_game_handler, (void *)connection);
	        pthread_detach(tid);
	    }
	    
	    fclose(logfile);
	}

	pthread_mutex_destroy(&lock); // Destroy mutex lock
	
	return 0;
}

// Log client connection in logfile
void log_connection(FILE* logfile, struct sockaddr_in client, int socket) {

	pthread_mutex_lock(&lock); // Lock

	// Convert address for readability
	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN);

	fprintf(logfile, "(%s)(%s)(soc_id %d) client connected \n", 
		current_time(), address, socket);

	fflush(logfile);

	pthread_mutex_unlock(&lock); // Unlock
}

// Returns the current time
char* current_time() {

	time_t current_time;
	char* time_string;

    current_time = time(NULL); // Current time in seconds

    time_string = ctime(&current_time); // Convert to local time
    
	// Remove \n from end of string
    size_t index = strlen(time_string);
    time_string[index - 1] = '\0';

	return time_string;
}

// Function logs a client move in logfile
void log_client_move(FILE* logfile, struct sockaddr_in client, int socket, int move) {

	pthread_mutex_lock(&lock); // Lock

	// Convert address into readable form
	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN);

	fprintf(logfile, "(%s)(%s)(soc_id %d) client's move = %d \n",
		current_time(), address, socket, move);

	fflush(logfile);

	pthread_mutex_unlock(&lock); // Unlock
}

// Log servers moving to logfile
void log_server_move(FILE* logfile, struct sockaddr_in server, int socket, int move) {

	pthread_mutex_lock(&lock); // Lock

	// Convert address for readability
	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(server.sin_addr), address, INET_ADDRSTRLEN);

	fprintf(logfile, "(%s)(%s)(soc_id %d) server's move = %d \n", current_time(), address, socket, move);

	fflush(logfile);

	pthread_mutex_unlock(&lock); // Unlock
}

// Log  end of game
void log_gameover(FILE* logfile, struct sockaddr_in client, int socket, int status) {

	pthread_mutex_lock(&lock); // Lock

	// Convert address for readability
	char address[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(client.sin_addr), address, INET_ADDRSTRLEN);

	fprintf(logfile, "(%s)(%s)(soc_id %d) game over, code = %d \n",
		current_time(), address, socket, status);

	fflush(logfile);

	pthread_mutex_unlock(&lock); // Unlock
}

// Run game
void* thread_game_handler(void* connection) {

	int server_move, client_move; // Column value of each move

	conn4 board; // Game Board

	FILE* logfile = fopen("log.txt", "a+");
	conn_t *client_conn;

	// Check for connection parameter
	if(!connection) {

		pthread_exit(0);
	}
	else{

		client_conn = (conn_t *)connection;
	}

	// Initialize board
	srand(RSEED);
	init_empty(board);

	// Play until finish
	while(1) {

		// Wait until client move
		if(recv(client_conn->socket, &client_move, sizeof(client_move),0)==0) {
			
			// Unfinished Game
			log_gameover(logfile, client_conn->client_address, client_conn->socket, STATUS_UNFINISHED);
			break;
		}

		// Client move
		if (do_move(board, client_move, O)!=1) {
			
			// Illegal Move
			log_gameover(logfile, client_conn->client_address, 
				client_conn->socket, STATUS_ILLEGAL_MOVE);
			break;
		}

		// Client's move
		log_client_move(logfile, client_conn->client_address,
		 	client_conn->socket, client_move);

		// If client wins
		if (winner_found(board) == O) {

			// User won
			log_gameover(logfile, client_conn->client_address, 
				client_conn->socket, STATUS_USER_WON);
			break;
		}

		// If no available moves
		if (!move_possible(board)) {

			// Log entry - Draw
			log_gameover(logfile, client_conn->client_address,
				 client_conn->socket, STATUS_DRAW);
			break;
		}

		// Server move
		printf("enter your move: ");
		scanf("%d",&server_move);

		if (do_move(board, server_move, X)!=1) {
			
			// Illegal Move
			log_gameover(logfile, client_conn->client_address, 
				client_conn->socket, STATUS_ILLEGAL_MOVE);
			break;
		}

		// Server move
		log_server_move(logfile, client_conn->serv_address, 
			client_conn->socket, server_move);

		// Send server move to client
		send(client_conn->socket, &server_move, sizeof(server_move), 0);

		// If server wins
		if (winner_found(board) == X) {

			// Server won
			log_gameover(logfile, client_conn->client_address,
				 client_conn->socket, STATUS_AI_WON);
			break;
		}


	}

	// Close log file and connection
	fclose(logfile);
	close(client_conn->socket);
	free(client_conn);
	
	pthread_exit(0); // Exit thread once loop is broken and game is finished

}


// Initialize playing array to empty cells
void init_empty(conn4 board) {
	int r, c;
	for (r=HEIGHT-1; r>=0; r--) {
		for (c=0; c<WIDTH; c++) {
			board[r][c] = EMPTY;
		}
	}
}

// Apply move to the board
int do_move(conn4 board, int c, char symbol) {

	int r=0;

	// find next empty slot
	while ((r<HEIGHT) && (board[r][c-1]!=EMPTY)) {
		r += 1;
	}
	if (r==HEIGHT) {

		// no move possible
		return 0;
	}

	board[r][c-1] = symbol;
	return 1;
}

// Undo move
void undo_move(conn4 board, int c) {

	int r=0;

	while ((r<HEIGHT) && board[r][c-1] != EMPTY) {
		r += 1;
	}

	board[r-1][c-1] = EMPTY;
	return;
}

// Check if board is full
int move_possible(conn4 board) {

	int c;

	for (c=0; c<WIDTH; c++) {

		if (board[HEIGHT-1][c] == EMPTY) {

			return 1;
		}
	}

	return 0;
}


// Check each move
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
		printf("Move not possible. \n");
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

	// bottom line
	printf("\t+");
	for (c=0; c<WIDTH; c++) {
		for (j=0; j<WGRID; j++) {
			printf("-");
		}
		printf("+");
	}
	printf("\n");

	// bottom labels
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


// If row formed
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

// check if symbols all the same
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

// suggest move
int suggest_move(conn4 board, char symbol) {

	int c;

	for (c=0; c<WIDTH; c++) {

		if (do_move(board, c+1, symbol)) {

			if (winner_found(board) == symbol) {

				undo_move(board, c+1);
				return c+1;
			} else {
				undo_move(board, c+1);
			}
		}
	}

	if (symbol == X) {
		symbol = O;
	} else {
		symbol = X;
	}
	for (c=0; c<WIDTH; c++) {

		if (do_move(board, c+1, symbol)) {

			if (winner_found(board) == symbol) {

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