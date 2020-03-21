Ana Vukojevic
03/08/2020


HOW IT WORKS:

The program listens on port 8888 and creates a thread every time it receives a request. It reads the request (currently only supports GET) and searches for the file. If the file is found, it is read into the buffer and sent to the client. The connection remains alive for 10 seconds.
The program is witten in C.

HOW TO RUN:

To compile, run:
gcc -pthread -o wwwserver www_server.c

Then:
./wwwserver
will start the server 

