/**
 * This file implements a File IO API that manages all input and output operations at the file system level.
 * Including validation of hash blocks.
 *
 * Usage:
 *
 * struct torrent_t torrent;
 *
 * if (create_torrent_from_metainfo_file("f.ttorrent", &torrent, "f")) {
 *     // error handling...
 * }
 *
 * if (torrent.block_map[0]) {
 *     // block 0 is available
 *     struct block_t block;
 *
 *     if (load_block(&torrent, 0, &block)){
 *         // handle error
 *     }
 *
 *     // do stuff with this block...
 * }
 *
 * struct block_t another_block;
 *
 * // fill this block somehow...
 *
 * int r = store_block(&torrent, 1, &another_block);
 *
 * if (r) {
 *     // block was not stored correctly, either because of an invalid hash or because of an I/O error.
 * } else {
 *     // the block was correctly stored and torrent.block_map[1] evaluates now to true.
 *     assert(torrent.block_map[1]);
 * }
 *
 * if(destroy_torrent (&torrent)) {
 *     // error handling
 * }
 *
 *
 */

#ifndef FILE_IO_H_
#define FILE_IO_H_

#include <stdio.h>
#include <stdint.h>
#include <netinet/in.h>
#include <openssl/sha.h>

/**
 * The maximum size of each block. All blocks, except possibly for the last one, shall have this size.
 */
enum { MAX_BLOCK_SIZE = 0x10000 };//!< MAX_BLOCK_SIZE

/**
 * A type definition for a SHA256 hash digest.
 */
typedef unsigned char sha256_hash_t[SHA256_DIGEST_LENGTH];

/**
 * This structure represents a torrent peer as an address and port pair.
 */
struct peer_information_t {
	uint8_t peer_address[4]; ///< Peer address in network byte order.
	uint16_t peer_port; ///< Peer port in network byte order.
};

/**
 * This structure represents the disk state of a downloaded torrent file.
 *
 * Only the block_map field needs to be *read* directly. All other fields should not be employed directly.
 */
struct torrent_t {
	const char * metainfo_file_name; ///< File name of the ".ttorrent" file.

	FILE * downloaded_file_stream; ///< A file stream where to read or write the downloaded data.

	sha256_hash_t downloaded_file_hash; ///< A hash of the whole downloaded file. Currently unused.

	uint64_t downloaded_file_size; ///< The size, in bytes, of the downloaded file.

	uint64_t block_count; ///< Number of blocks in the downloaded file.

	sha256_hash_t * block_hashes; ///< Hash of each of the blocks of the downloaded file.

	uint_fast8_t * block_map; ///< An array of integers denoting whether a block is correctly downloaded.

	uint64_t peer_count; ///< Number of peers available in the "peers" field.

	struct peer_information_t * peers; ///< An array of the peers available.
};

/**
 * A structure representing a block of data.
 */
struct block_t {
	uint8_t data[MAX_BLOCK_SIZE]; ///< Buffer where to store data or from where to read it.
	uint64_t size; ///< The number of valid elements in data buffer, starting at the beginning.
};

/**
 * This function initializes a torrent_t data structure from a disk ".ttorrent" file.
 * @param metainfo_file_name is the ".ttorrent" file.
 * @param torrent is the data structure to initialize.
 * @param downloaded_file_name is the file name of the downloaded file.
 * @return 0 on success, or -1 and errno is set.
 */
int create_torrent_from_metainfo_file (char const * const metainfo_file_name, struct torrent_t * const torrent,
		char const * const downloaded_file_name);

/**
 * Gets the size of a block in the downloaded file.
 * @param torrent is a torrent_t data structure.
 * @param block_number is the index of the block.
 * @return the size of the block.
 */
uint64_t get_block_size(const struct torrent_t * const torrent, const uint64_t block_number);

/**
 * Loads a block from disk into the block memory structure.
 * @param torrent is a torrent_t data structure.
 * @param block_number is the index of the block to load.
 * @param block is where the loaded block will be stored.
 * @return 0 on success, or -1 and errno is set.
 */
int load_block (const struct torrent_t * const torrent, const uint64_t block_number, struct block_t * const block);

/**
 * Stores a block in the downloaded file.
 * @param torrent is a torrent_t data structure.
 * @param block_number is the index of the block to store.
 * @param block contains the data that needs to be stored.
 * @return 0 on success, or -1 and errno is set.
 */
int store_block (struct torrent_t * const torrent, const uint64_t block_number, const struct block_t * const block);

/**
 * Deallocates all necessary fields in a torrent_t structure. It also closes the downloaded_file_stream stream.
 * @param torrent is a torrent_t data structure.
 * @return 0 on success, or -1 and errno is set.
 */
int destroy_torrent (struct torrent_t * const torrent);


#endif // FILE_IO_H_
