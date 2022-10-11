Lab #2. Chord.

The basic version of the Chord protocol is implemented. A peer-to-peer network is presented, where all participants (nodes) are equal.
Each node can act as a client, sending requests to perform any actions on another node, and as a server,
processing such requests. At startup, a node with parameters (id, ip, port) is created, after which the node must be added to the network
(join), while specifying its parameters. After that, the node is the only node in the network. To attach another node
you need to run .the exe file of the assembled program with the node parameters and write 'join', you need to enter the parameters of an already existing
node in the network (the first one). Etc.

Some input errors are displayed in the console and the program continues its execution. In case of errors related to connection
via sockets, the program terminates.

User Interface Methods:

join - adding a node to the network; parameters - id, ip, port of a node already existing in the network (for the first one - its own parameters);

leave - deleting a node from the network, and all keys stored by this node are moved correctly;

put - adding a stored key, the key is stored in the node, which is the successor for the node with the identifier 'key';

find - search for the identifier in which the specified key is stored;

cls - clearing the screen;

display - displays the table, the successor and predecessor numbers, and the list of stored keys for the current node.

LAUNCH:

Build the CMAKE project.

Start main.exe file with command line arguments - node's id, ip, port.
The user's interface methods are performed earlier.
