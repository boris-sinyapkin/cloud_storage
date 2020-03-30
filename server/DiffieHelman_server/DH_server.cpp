#include "DH_server.hpp"

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
typedef boost::random::uniform_int_distribution<uint1024_t> distribytion_type;
using namespace boost::multiprecision;
using namespace boost::asio;
using namespace std;


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
			byte_number  = byte_number - 8;

		}
	}

}

uint256_t change_keys(socket_ptr sock)
{
	unsigned char matrix_key[4][8];
	uint256_t key;

	Diffie_Hellman(sock, matrix_key);
	matrix_to_key(&key, matrix_key);

	return key;
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

cpp_int recieve_unint2048(socket_ptr sock)
{
	uint1024_t high_word, low_word;
	cpp_int num;

	read(*sock, buffer(&low_word, sizeof(uint1024_t)), transfer_all());
	read(*sock, buffer(&high_word, sizeof(uint1024_t)), transfer_all());


	num = high_word;
	num = (num << 144 * 8) | low_word;

	return num;
}

void send_uint2048(cpp_int num, socket_ptr sock)
{
	cpp_int a = 0xff;
	uint1024_t high_word;
	uint1024_t low_word;

	int i = 0;
	while (i < 143)
	{
		a = (a << 8) | 0xff;
		i++;
	}

	low_word = (uint1024_t)(num & a);
	high_word = (uint1024_t)(num >> 144 * 8);

	try
	{
		write(*sock, buffer(&low_word, sizeof(uint1024_t)), transfer_all());
		write(*sock, buffer(&high_word, sizeof(uint1024_t)), transfer_all());
	}
	catch (boost::system::error_code &error)
	{
		cout << error.message() << endl;
	}
}

void secure_answer(socket_ptr sock, unsigned char (&key_matrix)[4][8])
{

	uint1024_t A;

	try
	{
		unsigned int b = (unsigned int)randomize() % 100;

		cpp_int g = recieve_unint2048(sock);
		cpp_int p = recieve_unint2048(sock);
		read(*sock, buffer(&A, sizeof(uint1024_t)), transfer_all());


		uint1024_t B = (uint1024_t)get_Key(g, b, p);
		write(*sock, buffer(&B, sizeof(uint1024_t)), transfer_all());

		cpp_int key = get_Key(A, b, p) % std::numeric_limits<uint256_t>::max();

		key_to_matrix(key, key_matrix);

	}
	catch (boost::system::error_code &error)
	{
		cout << error.message() << endl;

	}
}

void Diffie_Hellman(socket_ptr sock, unsigned char (&key_matrix)[4][8])
{
	secure_answer(sock, key_matrix);
}

