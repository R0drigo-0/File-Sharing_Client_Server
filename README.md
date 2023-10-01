# Trivial Torrent Project: Client and Server

## Trivial Torrent Client

It download the file from the server which is specified in the .ttorent file in `/torrent_samples/client/test_file.ttorrent`

Use the file `src/ttorrent.c`.

The `ttorrent` command shall work as follows after execute **make**:

~~~{.diff}
$ bin/ttorrent file.ttorrent
~~~

## Trivial Torrent Server

It listens for incoming connections, and for each connection responds with a message and a block requested from the client if it can be sent.

Use the file `src/ttorrent.c`.

The `ttorrent` command shall be extended to work as follows after execute **make**:

~~~{.diff}
$ bin/ttorrent -l 8080 file.ttorrent
~~~

## Practical Details

### Required packages:
 * make
 * gcc
 * libssl-dev

### Building

Use `make` in a terminal to build your project.

~~~{.bash}
$ make
~~~

This will create the executable file `bin/ttorrent`.

### Testing

~~~{.bash}
$ make test
~~~
It `compiles` and `test` the code.



## File details
### .ttorent
Describes the information necessary to share a file for both clients and servers. It contains:
* The downloaded file length.
* The SHA256 hash of each block
* A list of address of multiple server peers from which this file can be downloaded.

### Makefile
There are three options to compile and run the code:
* **all(make):** Compiles the code and creates an object file in `/bin`.
* **test:** Compile the code and run a series of tests from `/test_script.sh`
* **clean:** Remove the object file from `/bin`

## Errors fixes
* **'\r' command not found** : `tr -d '\r' < test_script.sh > new_test_script.sh  & rm test_script.sh & mv new_test_script.sh test_script.sh`