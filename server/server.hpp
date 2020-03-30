#include <iostream>
#include <string>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>

#include "DiffieHelman_server/DH_server.hpp"


typedef unsigned long long ull;

void recieve_answer(socket_ptr sock, uint256_t *command_hash, uint256_t *answer, uint256_t key);
void send_request(socket_ptr sock, uint256_t command, uint256_t auth_hash, uint256_t key);

/*Чтение информации о файле.*/
char *read_fileinfo(socket_ptr sock, ull *file_size);
/*Создание файла*/
void create_file(char *directory, char *data, ull fsize, char *filename);
/*Приняте файла от клиента*/
void recieve_handler(socket_ptr sock);
/*Принимаем зашифрованное имя.*/
char *recieve_encr_filename(socket_ptr sock, uint256_t key);
/*Сессия клиента*/
void client_session(socket_ptr sock);
/*Требуется для контроля соединения*/
void connection_loop(socket_ptr sock);
/*Аутентификация с проверкой логина и пароля пользователя*/
void authentification(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);
/*Разделение запроса на код команды и данные*/
void parse_pack(uint512_t pack, uint256_t *command_hash, uint256_t *info);
/*Создание запроса*/
uint512_t make_pack(uint256_t command_hash, uint256_t info);
/*Добавление нового клиента в БД*/
void add_new_client(uint256_t command_hash, uint256_t auth_hash);

void create_folder(uint256_t hash);

/*принятие файла от сервера*/
void recieve_file(socket_ptr sock, char *name_folder);
/*отправка файла клиенту*/
void send_file(socket_ptr sock, char *name_folder, char *filename);
char *read_file(std::string client_path, ull *size);

void download_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);
void upload_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);
void delete_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);

char *hash_to_str(uint256_t hash);
char *str_to_hash(char *str, size_t size);
char *fname_to_hashstr(char *fname, size_t dsize);

/*Выводит список файлов.*/
void get_filelist(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);

/*Проверка существования данного Id на сервере.*/
bool check_login_hash(char *hash_string, uint256_t command_hash, socket_ptr sock);

void rename_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock);
void client_session(socket_ptr sock, socket_ptr sock1);
