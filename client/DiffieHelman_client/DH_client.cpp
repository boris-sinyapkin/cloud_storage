#include "DH_client.hpp"

#include <iostream>

using namespace boost::multiprecision;
using namespace boost::asio;
using namespace std;

typedef unsigned long long ull;
typedef boost::random::uniform_int_distribution<uint1024_t> distribytion_type;


cpp_int randomize()
{
	boost::random_device rd;
	distribytion_type dis;

	cpp_int value = dis(rd);

	value = (value << sizeof(uint1024_t)*8) | dis(rd);

	return value;
}


cpp_int get_Key(cpp_int a, cpp_int b, cpp_int c)
{
	namespace mp =  boost::multiprecision;

	return (mp::pow(cpp_int(a), (unsigned int)b)) % c;
}


void key_to_matrix(cpp_int key, unsigned char (&key_matrix)[4][8])
{
	cpp_int byte = 0xff;
	int byte_number = 31 * 8;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			byte = (key & (byte << byte_number)) >> byte_number;
			key_matrix[i][j] = (unsigned char)byte;

			byte = 0xff;
			byte_number = byte_number - 8;

		}
	}

}

void matrix_to_key(uint256_t *key, unsigned char key_matrix[4][8])
{
	uint256_t result_key = 0;


	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			result_key = result_key << 8;
			result_key = result_key + key_matrix[i][j];
		}
	}

	(*key) = result_key;
}

void send_uint2048(cpp_int num, ip::tcp::socket &sock)
{
	uint1024_t a = 0xff;
	uint1024_t high_word;
	uint1024_t low_word;

	int i = 0;
	while (i < 143)
	{
		a = (a << 8) | 0xff;
		i++;
	}

	low_word = (uint1024_t) (num & a);
	high_word = (uint1024_t) (num >> 144 * 8);

	try
	{
		write(sock, buffer(&low_word, sizeof(uint1024_t)), transfer_all());
		write(sock, buffer(&high_word, sizeof(uint1024_t)), transfer_all());
	}
	catch (boost::system::error_code &error)
	{
		cout << error.message() << endl;
	}
}

void endecrypt_pack(char *pack, size_t pack_size, uint256_t key)
{
	uint512_t temp = 0;
	uint512_t exp_key = 0;

	memcpy(&temp, pack, pack_size);

	size_t count_keys = pack_size / 32 + ((pack_size % 32) ? 0 : 1);

	/*Key Expansion*/
	for (size_t i = 0; i < count_keys; i++)
	{
		exp_key = exp_key << (sizeof(uint256_t) * 8);
		exp_key += key;
	}

	temp = temp ^ exp_key;

	memcpy(pack, &temp, pack_size);
	
}

cpp_int recieve_unint2048(ip::tcp::socket &sock)
{
	uint1024_t high_word, low_word;
	cpp_int num;

	read(sock, buffer(&low_word, sizeof(uint1024_t)), transfer_all());
	read(sock, buffer(&high_word, sizeof(uint1024_t)), transfer_all());

	num = high_word;
	num = (num << 144 * 8) | low_word;

	return num;
}

uint256_t change_keys(ip::tcp::socket &sock)
{
	unsigned char matrix_key[4][8];
	uint256_t key;

	Diffie_Hellman(sock, matrix_key);
	matrix_to_key(&key, matrix_key);

	return key;
}

void secure_request(ip::tcp::socket &sock, unsigned char (&key_matrix)[4][8])
{
	unsigned int a = (unsigned int)randomize() % 100; //2 байта
	cpp_int g = randomize();
	cpp_int p = randomize() % std::numeric_limits<uint1024_t>::max();

	uint1024_t A = (uint1024_t)get_Key(g, a, p);
	uint1024_t B;

	send_uint2048(g, sock);
	send_uint2048(p, sock);
	write(sock, buffer(&A, sizeof (uint1024_t)), transfer_all());

	read(sock, buffer(&B, sizeof(uint1024_t)),transfer_all());

	cpp_int key = get_Key(B, a, p) % std::numeric_limits<uint256_t>::max();

	key_to_matrix(key, key_matrix);

}

void Diffie_Hellman(ip::tcp::socket &sock, unsigned char (&key_matrix)[4][8])
{
	secure_request(sock, key_matrix);
}


