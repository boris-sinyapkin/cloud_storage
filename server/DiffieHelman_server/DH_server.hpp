#include "boost/random.hpp"
#include "boost/random/random_device.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
using namespace boost::multiprecision;

cpp_int randomize();
cpp_int get_Key(cpp_int g, cpp_int a, cpp_int p);
void key_to_matrix(cpp_int key, unsigned char (&key_matrix)[4][8]);
void secure_answer(socket_ptr sock, unsigned char (&key_matrix)[4][8]);
void Diffie_Hellman(socket_ptr sock, unsigned char (&key_matrix)[4][8]);
cpp_int recieve_unint2048(socket_ptr sock);
void send_uint2048(cpp_int num, socket_ptr sock);
void matrix_to_key(uint256_t *key, unsigned char key_matrix[4][8]);
void endecrypt_pack(char *pack, size_t pack_size, uint256_t key);
uint256_t change_keys(socket_ptr sock);
