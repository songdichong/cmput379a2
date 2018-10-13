Team members:
Dichong Song
Joey Wong

Instruction:
In console, input make to compile the code and make clean to delete the output file.
To run the server, input ./gameserver379 gridSize period port seed (ex:gameserver379 16 0.2 8989 1547543314). The period should be a number in seconds.
To run the client, input ./gameclient379 IP port (ex:gameclient379 127.0.0.1 8989)

Work distribution:

Joey handled server side and Dichong handled client side. We do the testing/debugging together.

Open source code use:
Only function in the posted example code on eclass is used.

Answer the question:
1.How do you ensure that input response time(keystroke read, transmission of command, receiving at the server) is minimized?
On the server side, there is a thread created for each client, and it is used to receive input whenever it is avaliable.
On the client side, there is a thread created to receive valid keyboard input, and send it to server whenever a key is entered.


2.How do you minimize the amount of traffic flowing back and forth between clients and server?
Server: Calculate the number of points (contain bullet,player) that are on the board, then only send client these points.
Client: Get these points only.
Example: If there are 5 player, 1 bullet, each occupy different point, server will send 6 point to client.

3.How do you handle multiple commands being sent from a client between two successive game board updates? (see also notes)
In the server side, the player input will be overwriten by the latest command from client.
A mutex(mutex2) is used to ensure no update to player input is allowed when the client is computing using client input.

4.How do you ensure that the same(consistent) state is broadcast to all clients?
There is only 1 thread that is responsible for the broadcast.
Unless, the player choose to disconnect, then it doesn't matter since the board is not sended.

5.How do you ensure the periodic broadcast updates happen as quickly as possible?
We start the timer before the refresh begin, and when the refresh is over, we calculate how many timer is remaining in the timer, 
then we wait for that amount of time before next update.

6.
How do you deal with clients that fail without harming the game state?
*if you meant fail by dead.
If the client is dead, then server will set that player disconnect, then in the next update, it will remove in the next iteration.(Both connection pthread corresponded to the player)
See Below for how to disconnection is handled.

7.
How do you handle the graceful termination of the server when it receives a signal? (Provideand implementa reasonable
form of graceful termination.)
When it receive a signal(SIGINAL), it will tell each player in the game the connection is over, and send them the score. Then it will wait for client close the socket,
after that, it will close its own socket from server side.

8.
How do you deal with clients that unilaterally break theconnection to theserver? (Assume the client can exit,e.g.crash,
without advance warning.)
Assuming all the threads in client crashed together, so the socket are not used anymore.
If the client fail, it will disconnect. During recv() call from server, the server will notice an recv()==0, then it will close the 
client thread. But at this point we don't have to delete the player yet. In the next refresh/update, the send() will return EPIPE error,
then the server will set the player to be disconnected, then in the next iteration, the player is deleted from server.
Note, in the rare case where only the main thread exit, it will caused the server to stall, since it is waiting for client to close the socket,
but client socket is still used by child thread, and it is not updating.

9.
How can you improvethe user interface without hurting performance?
Instead of using the same mutex to receive input, create 1 for each client, so the update is smoother.


Design:
How client are stored:
A client/player struct will contain {socket related to the connection, score,dead,location,old_location,etc}
and a linkedList structure is used to store the client, which allow the number of client to grow and shrink.

What kind of message is sended:
INITIALLY, when the player connect, it will receive its pid and size of the board. Although we could also send the location of player, 
it doesn't make much sense without the location of other players. So it will receive its location when the Server "Refresh", where all info in the board are
sended to player, including its location.
DURING THE GAME IS ON, the server will send the points that are meaningful, where bullet/player is on it.

How exit/disconnect is done
General Steps for disconnecting:
1. Server send a message to client requesting close() from client
2. Client got the message, and use close(socket)
3. Server recv() return 0 , indicating that the client closed the connection, then it call close(socket)

Reason behind the steps:
This is a tcp connection, client have to close(socket) first, otherwise we will have to wait for 
CLOSE_WAIT, if we want to restart the server. 
See https://hea-www.harvard.edu/~fine/Tech/addrinuse.html

SIGNAL HANDLER:
WHEN server receive SIGTERM,sigint,sigchld,remvoe all player(disconnecting,free player,etc). then exit.

How players are moved:
Move player to taget location, if the location is occupied by another player. Check if that player is there originally, otherwise return both of them to
original location.

How player1 are returned:
Check if there are any player in its old_location, if there is a player2, recursive call to return that player2 to origianlly location.
Then move player1 back to the location.


Testing
1. Start n+1 client on n sized board
Tried it 5 client on 2*2 board, no issue have occur.
The 5 th player is waiting for 1 player to die, before spawning.
1 player die, the 5th player spawn.

2. Kill another player
    Spawn p1,p2 .P1 kill p2, p2 print out score 0. Spawn p3, p3 kill p1, p1 print out score 1. 
3.Move out of bound
    Player cant move out of bound, working as intended
4.See if other bullet from other player is bolded
    Yes
5. Connect 3 player to the game and see if graceful termination succeed?
    4*4 board, 3 player 
    Succeed at graceful termination
6.Move 2 player in the same point
    Result: The player doesnt move when both try to get into it at the same time. Only get in, when the other player does not issue the command.
7. Player 1 join the game, Player 2 join, Player 3 join
Player 1 at point A, player 2 at point B, player 3 at point C.
Move P1 to D,P2 to A,P3 to D. (the order matter, because the player are move in this order)

8. Check for bolded bullet and player. No issue.
