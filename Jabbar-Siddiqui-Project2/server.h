// Connection info
typedef struct {

 	int socket;							// Socket file
 	struct sockaddr_in client_address;  // Client address
 	struct sockaddr_in serv_address;    // Server address

 } conn_t;

// Functions
void log_connection(FILE* logfile, struct sockaddr_in client, int socket);
char* current_time();
void log_client_move(FILE* logfile, struct sockaddr_in client, int socket, 
	int move);
void log_server_move(FILE* logfile, struct sockaddr_in server, int socket, 
	int move);
void log_gameover(FILE* logfile, struct sockaddr_in client, int socket, 
	int status);
void* thread_game_handler(void* connection);