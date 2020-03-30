#include "boost/random.hpp"
#include "boost/random/random_device.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

using namespace boost::multiprecision;
using namespace boost::asio;

cpp_int randomize();
cpp_int get_Key(cpp_int g, cpp_int a, cpp_int p);
void key_to_matrix(cpp_int key, unsigned char (&key_matrix)[4][8]);
void matrix_to_key(uint256_t *key, unsigned char key_matrix[4][8]);
void secure_request(ip::tcp::socket &sock, unsigned char (&key_matrix)[4][8]);
void Diffie_Hellman(ip::tcp::socket &sock, unsigned char (&key_matrix)[4][8]);
void send_uint2048(cpp_int num, ip::tcp::socket &sock);
void endecrypt_pack(char *pack, size_t pack_size, uint256_t key);
uint256_t change_keys(ip::tcp::socket &sock);
