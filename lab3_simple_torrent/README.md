Lab #3. Torrent client.

The simple torrent client without DHT is implemented. First, the dictionary is parsed (in bencoding format), while 
extracting data about the file itself (file name, size, etc), then a request is made to the torrent tracker, which sends a 
list of peers, after which there is a handshake with peers. Active peers reply and send list of pieces that are available for 
downloading. After the file is downloading in parallel threads (if there are several active peers). 

LIMIATATIONS: 1) Only one file can be downloaded! 2) peers list are not refreshing dynamically; 3) it is possible to get peers list 
only from [http/https] tracker; 4) one peer has one thread, but if number of peers will be greater than maximum possible 
number of threads, only the first N peers will start downloading file.

Some errors are displayed in the console. If critical error related to peer connection, only this thread are terminated, other continue
downloading file. Once every ten seconds, an attempt is made to connect to inactive peers.

LAUNCH:

Build the CMAKE project.

Start torrent.exe file with command line arguments - path to .torrent file, directory where the file will be saved (without '\' at
the end and without out file name).