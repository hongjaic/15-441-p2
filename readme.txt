readme.txt

Authors: Raul Gonzalez <rggonzal>, Hong Jai Cho <hongjaic>


CP1
===

This is a distributed network of nodes, that clients can connect to to retrieve 
files from the system or add new files into the system. a single node consists 
of both a routing daemon and a webserver.

rd.c:	routing daemon, figures out where the objects are stored on the system 
and tells the webserver. Needs a .conf file sothat it can communicate with other 
nodes, and a .files file to keep track of the local objects 
	
webserver.py:	webserver interface to clients. queries the routing daemon,
fetches the files from the system and returns them to the client. also, adds 
files to the local node.



+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

CP2
===

Added functionality to actually route to other nodes. We maintain a table of
of direct neighbors, a table of all local objects, a hash table of all global
objects and nearest paths to them, and a routing table.

When a request comes in from flask, the hash table of all global objects is
accessed first to see if the object exists. If the object exists, the hash
table to provided the node id of the node that has the object. Routing table
is referred to using the node id for the next hop. When routing table returns
the next hop, the next direct neighbors table is referred to using that
information for the ip address and host of the next hop. When this process is
successful, the full next hop informaiton is returned to flask server.

When flooding, the direct neighbors table and local objects table are referred
to build an LSA packet. when the LSA packet is built, it is sent to all
neighbor node.

The routing table acts an important role. apart from containing the routing
data, it maintains the last LAS received from all nodes and is used for
retransmissions.

When adding files, the file is not addded if a file of the same name with
different sha hash exists. 

SETUP:

	in /rd directory:
		>make
		>./rd ../starter/node#.conf ../starter/node#.files
	
	in /starter directory:
		>./webserver.py node#.conf

	open up web browser and type:
		http://<webserver-IP>:<webserver-PORT>

node#.conf:
node_id rd_tcp_port rd_udp_port flask_server_port

node#.file:
obj_name obj_relative_path
