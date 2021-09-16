This program allows two players to play connect 4. One player is the server and the other is the client. The 
way that it works is that once the server and client are connected, the client is prompted to enter the column
that it wants to enter its piece in, after which the server is asked to do the same. The board is displayed to
the client after each turn. 

How to use the program:
1. Start up the server and client using the command line commands
    IMPORTANT NOTE: Use the following command for the server: gcc -pthread -o TCPEchoServer TCPEchoServer.c
2. The client will be asked to enter a column number which indicates the column it wants to drop its piece in
3. The server will then be asked to do the same
4. The game will be done when either the server or the client win or draw
7. Enjoy!
