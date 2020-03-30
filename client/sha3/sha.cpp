#include "keccak.hpp"

uint8_t *sha(uint8_t *str, size_t str_size) {

	state sha3;
	uint8_t *hash;
	size_t hash_size = HASH_LEN;
	int ptr = 0;

	for (int i = 0; i < 25; i++)
		sha3.qword[i] = 0;

	for (size_t i = 0; i < str_size; i++) {
		sha3.byte[ptr++] ^= str[i];
		if (ptr >= RSIZE) {
			keccak(sha3.qword);
			ptr = 0;
		}
	}

	sha3.byte[ptr] ^= 0x06;
	sha3.byte[RSIZE - 1] ^= 0x80;
	keccak(sha3.qword);

	hash = (uint8_t *) malloc(sizeof(hash_size) * hash_size);

	for (int i = 0; i < HASH_LEN; i++) {
		hash[i] = sha3.byte[i];
	}

	return hash;
}