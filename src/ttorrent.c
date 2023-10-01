
// Trivial Torrent

// TODO: some includes here

#include "file_io.h"
#include "logger.h"

#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

// TODO: hey!? what is this?

/**
 * This is the magic number (already stored in network byte order).
 * See https://en.wikipedia.org/wiki/Magic_number_(programming)#In_protocols
 */
static const uint32_t MAGIC_NUMBER = 0xde1c3233; // = htonl(0x33321cde);

static const uint8_t MSG_REQUEST = 0;
static const uint8_t MSG_RESPONSE_OK = 1;
static const uint8_t MSG_RESPONSE_NA = 2;

const int MAX_BACKLOG = 10;

int client(char **argv);
int server(char **argv);

enum
{
	RAW_MESSAGE_SIZE = 13
};

/**
 * This function implements the integration of the client, where the client, depending on some parameters, calls the function and requests a file from the server. When an error ocurr it print the message in the terminal, indicating where it has failed.
 *
 * @param argv indicates the parameters that we receive, it is expressed in form of an array. In this function should arrive two parameteres, where the first indicates the execution and the second it indicates the metainfo file that the client want to utilize for the execution.
 *
 * @return indicates if an error has occurred, if the value is -1 an error has occurred, other value than -1 indicates that no error has occurred.
 */
int client(char **argv)
{
	struct torrent_t torrent;

	// Extract the file path from argv[1] except for the last 9 characters (".ttorrent") and store it in char[] file_path.
	char *file_name = argv[1];
	size_t fplen = strlen(file_name);
	if (fplen < 9 || strcmp(&file_name[fplen - 9], ".ttorrent") != 0)
	{
		perror("Error: invalid file format/name.");
		return -1;
	}

	char file_path[fplen - 8];
	strncpy(file_path, file_name, fplen - 9);
	file_path[fplen - 9] = '\0';

	if (create_torrent_from_metainfo_file(argv[1], &torrent, file_path))
	{
		perror("Could not create torrent from metainfo file");
		return -1;
	}

	int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
	{
		perror("Socket couldn't be created");
		return -1;
	}

	uint8_t rec_buffer[RAW_MESSAGE_SIZE], send_buffer[RAW_MESSAGE_SIZE], rec_msg_type = 0;
	uint32_t rec_magic_number = 0;
	uint64_t current_block = 0, i = 0, rec_block_size = 0;
	char buffer[16]; // 16, max ipv4 ip is 255.255.255.255 (12 ip + 3 "." Bytes) and one more Byte for the line break termination ("\n")

	while (i < torrent.peer_count)
	{
		memset(buffer, 0, sizeof(buffer));
		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;

		snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d", torrent.peers[i].peer_address[0], torrent.peers[i].peer_address[1], torrent.peers[i].peer_address[2], torrent.peers[i].peer_address[3]);

		server_addr.sin_port = torrent.peers[i].peer_port;
		server_addr.sin_addr.s_addr = inet_addr(buffer);

		if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
		{
			perror("Peer connection failed");
			i++;
			continue;
		}

		log_printf(LOG_INFO, "Connected to peer.");

		// Iterate over all the blocks of the torrent file to be received.
		while (current_block < torrent.block_count)
		{
			if (torrent.block_map[current_block] == 0)
			{
				rec_block_size = get_block_size(&torrent, current_block);

				// Prepare the request message.
				send_buffer[0] = (uint8_t)(MAGIC_NUMBER >> 24) & 0xff;
				send_buffer[1] = (uint8_t)(MAGIC_NUMBER >> 16) & 0xff;
				send_buffer[2] = (uint8_t)(MAGIC_NUMBER >> 8) & 0xff;
				send_buffer[3] = (uint8_t)(MAGIC_NUMBER >> 0) & 0xff;

				send_buffer[4] = MSG_REQUEST;

				send_buffer[5] = (uint8_t)(current_block >> 56) & 0xff;
				send_buffer[6] = (uint8_t)(current_block >> 48) & 0xff;
				send_buffer[7] = (uint8_t)(current_block >> 40) & 0xff;
				send_buffer[8] = (uint8_t)(current_block >> 32) & 0xff;
				send_buffer[9] = (uint8_t)(current_block >> 24) & 0xff;
				send_buffer[10] = (uint8_t)(current_block >> 16) & 0xff;
				send_buffer[11] = (uint8_t)(current_block >> 8) & 0xff;
				send_buffer[12] = (uint8_t)(current_block >> 0) & 0xff;

				// Send the request message.
				log_printf(LOG_INFO, "Requesting block {magic_number = %i, block_number = %i, message_code = %i}", htonl(MAGIC_NUMBER), current_block, rec_msg_type);
				if (send(socket_fd, send_buffer, RAW_MESSAGE_SIZE, 0) == -1)
				{
					perror("Request message couldn't be sent");
					current_block++;
					continue;
				}

				log_printf(LOG_INFO, "Waiting for response");

				if (recv(socket_fd, rec_buffer, RAW_MESSAGE_SIZE, 0) == -1)
				{
					perror("Response message couldn't be received");
					current_block++;
					continue;
				}

				// Check the response message.
				rec_msg_type = rec_buffer[4];
				if (rec_msg_type != MSG_RESPONSE_OK)
				{
					perror("Response message type is not correct");
					current_block++;
					continue;
				}

				// Check the magic number
				rec_magic_number = (uint32_t)((rec_buffer[0] << 24) | (rec_buffer[1] << 16) | (rec_buffer[2] << 8) | (rec_buffer[3] << 0));
				log_printf(LOG_INFO, "Response block {magic_number = %d, block_number = %i, message_code = %i}", htonl(rec_magic_number), current_block, rec_msg_type);
				if (htonl(rec_magic_number) != htonl(MAGIC_NUMBER))
				{
					perror("Magic number is not correct");
					return -1;
				}

				// Create a block_t struct to store the data from the recieved block.
				struct block_t rec_block;
				rec_block.size = rec_block_size;

				recv(socket_fd, rec_block.data, rec_block.size, MSG_WAITALL);

				log_printf(LOG_INFO, "Reading %i bytes of payload", rec_block.size);

				log_printf(LOG_INFO, "Storing block %d.\n", current_block);

				if (store_block(&torrent, current_block, &rec_block) == -1)
				{
					perror("Block couldn't be stored");
					return -1;
				}

				log_printf(LOG_INFO, "Block %d stored.\n", current_block);
			}

			current_block++;
		}

		if (close(socket_fd) == -1)
		{
			perror("Socket couldn't be closed");
			return -1;
		}

		i++;

		if (current_block >= torrent.block_count)
		{
			break;
		}
	}

	log_printf(LOG_INFO, "All %d blocks stored.\n", torrent.block_count);

	return 0;
}

/**
 * This function implements the integration of the server, where depending on some parameters, calls the function and sends the file to the client. When an error ocurr it print the message in the terminal, indicating where it has failed.
 *
 * @param argv indicates the parameters we receive, it is expressed in the form of an array. In this function four parameters must arrive, where the first indicates the execution, the second indicates if we are specifying a port, the third indicates the port and the fourth indicates the metainfo file that the server uses to send the files to the clients.
 *
 * @return indicates if an error has occurred, if the value is -1 an error has occurred, other value than -1 indicates that no error has occurred.
 */
int server(char **argv)
{
	struct torrent_t torrent;

	// Extract the file path from argv[3] except for the last 9 characters (".ttorrent") and store it in char[] file_path.
	char *file_name = argv[3];
	size_t fplen = strlen(file_name);
	if (fplen < 9 || strcmp(&file_name[fplen - 9], ".ttorrent") != 0)
	{
		perror("Error: invalid file format/name.");
		return -1;
	}

	char file_path[fplen - 8];
	strncpy(file_path, file_name, fplen - 9);
	file_path[fplen - 9] = '\0';

	if (create_torrent_from_metainfo_file(argv[3], &torrent, file_path))
	{
		perror("Could not create torrent from metainfo file");
		return -1;
	}

	int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket_fd == -1)
	{
		perror("Socket couldn't be created");
		return -1;
	}

	struct sockaddr_in addr_server;
	memset(&addr_server, 0, sizeof(addr_server));
	addr_server.sin_family = AF_INET;
	addr_server.sin_port = htons((uint16_t)(atoi(argv[2])));
	addr_server.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(server_socket_fd, (struct sockaddr *)&addr_server, sizeof(addr_server)) == -1)
	{
		perror("Socket binding error");
		return -1;
	}

	if (listen(server_socket_fd, MAX_BACKLOG) == -1)
	{
		perror("Listen error");
		return -1;
	}

	struct sockaddr_in addr_client;
	memset(&addr_client, 0, sizeof(addr_client));

	uint8_t msg_recv[MAX_BLOCK_SIZE];

	uint64_t block_req_last = 0xFFFFFFFFFFFFFFFF;

	socklen_t addr_client_size = sizeof(addr_client);

	while (1)
	{
		int client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&addr_client, &addr_client_size);

		if (client_socket_fd > 0)
		{
			int child = fork();

			if (child == 0)
			{
				if (close(server_socket_fd) == -1)
				{
					perror("Server socket could not be closed");
					return -1;
				}

				while (1)
				{
					if (recv(client_socket_fd, msg_recv, RAW_MESSAGE_SIZE, 0) < 0)
					{
						perror("Message could not be received");
						return -1;
					}

					uint32_t magic_number =
						((uint32_t)msg_recv[3] << 0) |
						((uint32_t)msg_recv[2] << 8) |
						((uint32_t)msg_recv[1] << 16) |
						((uint32_t)msg_recv[0] << 24);

					if (magic_number != MAGIC_NUMBER)
					{
						log_printf(LOG_INFO, "Magic number error");

						if (destroy_torrent(&torrent) == -1)
						{
							perror("Torrent could not be destroyed");
						}

						return -1;
					}

					uint64_t block_req =
						((uint64_t)msg_recv[12] << 0) |
						((uint64_t)msg_recv[11] << 8) |
						((uint64_t)msg_recv[10] << 16) |
						((uint64_t)msg_recv[9] << 24) |
						((uint64_t)msg_recv[8] << 32) |
						((uint64_t)msg_recv[7] << 40) |
						((uint64_t)msg_recv[6] << 48) |
						((uint64_t)msg_recv[5] << 56);

					if (block_req == block_req_last)
					{
						exit(0);
					}

					log_printf(LOG_INFO, "\nBlock number: %d", block_req);

					if (block_req > torrent.block_count)
					{
						perror("Requested block is out of range");
						return -1;
					}

					if (torrent.block_map[block_req] == 1)
					{
						msg_recv[4] = MSG_RESPONSE_OK;

						if (send(client_socket_fd, msg_recv, RAW_MESSAGE_SIZE, 0) < 0)
						{
							perror("Message could not be sent");
							return -1;
						}

						struct block_t block;
						uint64_t blockSize = get_block_size(&torrent, block_req);

						load_block(&torrent, block_req, &block);

						log_printf(LOG_INFO, "Block size: %d bytes", blockSize);

						if (send(client_socket_fd, block.data, block.size, 0) < 0)
						{
							perror("Message could not be sent");
							return -1;
						}
					}

					else
					{
						msg_recv[4] = MSG_RESPONSE_NA;

						if (send(client_socket_fd, msg_recv, RAW_MESSAGE_SIZE, 0) < 0)
						{
							perror("Message could not be sent");
							return -1;
						}
					}

					block_req_last = block_req;
				}
			}

			else
			{
				if (close(client_socket_fd) == -1)
				{
					perror("Client socket could not be closed");
					return -1;
				}
			}
		}
	}

	if (destroy_torrent(&torrent) == -1)
	{
		perror("Torrent could not be destroyed");
		return -1;
	}

	if (close(server_socket_fd) == -1)
	{
		perror("Socket could not be closed");
		return -1;
	}

	return 0;
}

/**
 * Main function.
 */
int main(int argc, char **argv)
{

	set_log_level(LOG_DEBUG);

	log_printf(LOG_INFO, "Trivial Torrent (build %s %s) by %s", __DATE__, __TIME__, "Nil Caballero Milà and Rodrigo Chele");

	// ==========================================================================
	// Parse command line
	// ==========================================================================

	// TODO: some magical lines of code here that call other functions and do various stuff.

	int res = 0;

	if (argc == 2)
	{
		res = client(argv);
	}

	else if (argc == 4)
	{
		res = server(argv);
	}

	else
	{
		perror("Not enough or too many arguments");
		res = -1;
	}

	// The following statements most certainly will need to be deleted at some point...
	// (void)argc;
	// (void)argv;
	(void)MAGIC_NUMBER;
	(void)MSG_REQUEST;
	(void)MSG_RESPONSE_NA;
	(void)MSG_RESPONSE_OK;

	/*
	argc: nombre d'arguments passats al programa.
	argv: vector/array unidimensional de cadenes.
		- argv[x]: cadenes (char[], *char).
		- argv[0]: nom amb què s’ha cridat al programa.
		- argv[1..n]: arguments.
		- argc sempre sera almenys 1.

	*/

	return res;
}
