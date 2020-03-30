#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifndef HASH_LEN
#define HASH_LEN (32)
#endif

#ifndef RSIZE
#define RSIZE (136)
#endif

#ifndef KECCAK_ROUNDS
#define KECCAK_ROUNDS 24
#endif

#ifndef ROTL
#define ROTL(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

typedef union {
	uint8_t byte[200];
	uint64_t qword[25];
} state;

size_t unite(uint8_t **inputstr, char *login, char *pass);

uint8_t *sha(uint8_t *str, size_t str_size);

void keccak(uint64_t st[25]);
