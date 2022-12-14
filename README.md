# Live Music Streaming
  
Our project involves audio streaming using customized protocol to stream the audio from server to client with added reliability.
The client will ask for list of available songs on the server. The server will send the list to the client. From the list the client can select any song it wants. Also play, resume, pause, help commands will be available for client to control audio streaming.
This project uses the concept of socket programming, multithreading and basic concepts of reliable data transfer.

Install vlc:
  sudo apt-get install libpthread-stubs0-dev libvlc-dev vlc


SERVER
$gcc server.c -o exeSer -lpthread
$./exeSer 127.0.0.1 8080 8081


CLIENT
$gcc client.c -o exeCli -lpthread -l vlc
$./exeCli 127.0.0.1 8080 8081

i.e. ./server 127.0.0.1 8080 8081, ./client 127.0.0.1 8080 8081 make sure that port1 and port2 are same for both client and server.
<port-no-1> This port is for song transmission.
<port-no-2> This port is for transmitting list of songs available on the server side.
