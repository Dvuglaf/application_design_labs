Lab #4. MJPEG Streaming.

The broadcasting of the image from the webcam was implemented. The image from the repeater in encoded form is transmitted to the server,
which in turn sends it to the clients. The client can be either a web browser or a user who has launched the client application.

Server port: 7777.

Repeater port: 7778.

If critical error related to server or repeater connection, server terminated. When the client is disconnected, the server continues to work.

LAUNCH:

Build the CMAKE project.

Start server.exe file with command line arguments - ip address of server. 

Start repeater.exe file with command line arguments - ip address of server.

Open web-browser, enter server_ip:7777 OR start client.exe file with command line arguments - ip address of server.