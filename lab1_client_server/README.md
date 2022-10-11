Laboratory work #1. Client-server architecture.

The image is transmitted from the client to the server via Berkeley sockets.
Transmission distortion is simulated by adding pulse noise to the image.
All errors are processed and the text is displayed in the console.

On the client side, noise is added to the image. The image is compressed by the JPEG algorithm and transmitted.

On the server side, the image is decoded and subjected to median filtering.

Launch:

Build a CMAKE project.

Start the server first (server.exe ).
Launch the client (client.exe ) with command line arguments - the full path to the image and the noise intensity.