#include <iostream>
#include <fstream>
#include <ctime>

#include <boost/lexical_cast.hpp>
#include "boost/random.hpp"
#include "boost/random/random_device.hpp"
#include "boost/multiprecision/cpp_int.hpp"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/crc.hpp>
#include <boost/format.hpp>

#include "AES256/AES_256.hpp"
#include "DiffieHelman_client/DH_client.hpp"
#include "sha3/keccak.hpp"

#include <unistd.h>

typedef unsigned long long ull;

using namespace std;
using namespace boost;
using namespace boost::asio;


void recieve_answer(ip::tcp::socket &sock, uint256_t *command_hash, uint256_t *answer, uint256_t key);
void send_request(ip::tcp::socket &sock, uint256_t command, uint256_t auth_hash, uint256_t key);

void create_file(char *filename, char *data, ull fsize, int *error);

char *read_file(char *filename, ull *size, int *error);
void send_fileinfo(char *filename, unsigned short sizeOfName,ull sizeOfFile, ip::tcp::socket &sock);
void send_file(char *filename, ip::tcp::socket &sock, uint256_t login_hash, int *error);
void send_filename(ip::tcp::socket &sock, char *filename, uint256_t login_hash, uint256_t key, int *error);
void recieve_file(char *filename, ip::tcp::socket &sock, uint256_t login_hash, int *error);

int get_command(ip::tcp::socket &sock, uint256_t auth_hash, uint256_t login_hash);

void parse_pack(uint256_t *command_hash, uint256_t *info);
uint512_t make_pack(uint256_t command_hash, uint256_t info);

char *encrypt_buffer(char *buffer, uint128_t buf_size, uint128_t *encr_size, uint256_t key, int *error);
char *decrypt_buffer(char *buffer, uint128_t buf_size, uint256_t key, int *error);

uint256_t string_sha(string l, string p);
uint256_t char_sha(char *l, char *r);
uint256_t convert_to_uint256t(uint8_t array[32]);

void upload_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash);
void download_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash);
void delete_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash);
void rename_file(ip::tcp::socket &sock, uint256_t auth_hash, char *last_fname, char *new_fname, uint256_t login_hash);

int parse_servaddr(string command_line,  string &ip_str, int *port);

void add_new_client(uint256_t auth_hash, ip::tcp::socket &sock);
uint256_t authentification(ip::tcp::socket &sock, uint256_t *login_hash);
string check_hash(string login, string pass);

void get_filelist(ip::tcp::socket &sock, uint256_t auth_hash, uint256_t login_hash);

void show_command_list(void);

int check_crc(char *filename, int *error);
uint32_t GetCrc32(char *filename, int *error);
