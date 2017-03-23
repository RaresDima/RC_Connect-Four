# RC_Connect-Four

Implementation of a server-client application for the linux platform which allows users to play the game Connect four. 

The users create an account which is stored in a remote Oracle database, which also stores their lifetime victories against other players.

Multiple sessions of the game cand be run at the same time due to the fact that the server uses separate threads to hande the login of each individual user, and then the game session between two users.

When a match is over the users can choose to play agan, at which point they are put into the matchmaking queue again, or to disconnect. 

The Client application needs two command line areguments: the first is the IP of the server and the second is the port
