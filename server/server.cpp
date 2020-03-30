#include "server.hpp"

//command hashes
cpp_int filelist_req =  0xF19D7634;
cpp_int reg_req =  0xCFAAC144;
cpp_int upload_req = 0x5F3B6FAB;
cpp_int download_req = 0x12CA7F1B;
cpp_int auth_req = 0x60AFB0A4;
cpp_int del_req =  0x676AB801;
cpp_int rename_req = 0x312AA1CF;

//answers
uint256_t _true =   0xA2CCDFF1;
uint256_t _false =  0x1CFBACC4;

using namespace std;
using namespace boost::asio;
//using namespace boost::filesystem;

typedef unsigned long long ull;

void add_new_client(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	create_folder(auth_hash);

	uint512_t answer = make_pack(command_hash, _true);

	uint256_t key = change_keys(sock);
	endecrypt_pack((char*)&answer, sizeof(answer), key);
	write(*sock, buffer(&answer, sizeof(uint512_t)));
}

char *hash_to_str(uint256_t hash)
{
	uint256_t temp_auth_hash = hash;
	uint256_t temp_ascii=0;
	char *name_folder = new char [65];

	for(int i=63; i>=0; i--)
	{
		temp_ascii = temp_auth_hash % 0x10;

		memcpy(&name_folder[i],&(temp_ascii),sizeof(char));

		if(temp_ascii>=10)
			name_folder[i]=name_folder[i]+'A'-10;
		else
			name_folder[i]=name_folder[i]+'0';

		temp_auth_hash=temp_auth_hash >> 4;
	}

	name_folder[64] ='\0';

	return name_folder;
}
char *str_to_hash(char *str, size_t size)
{
	char *hash = new char [size + 1];

	for (size_t i = 0; i < size; i++)
		hash[i] = ((str[2 * i] - ((str[2 * i] < 'A') ? '0' : ('A' - 10)))
						<< 4) + str[2 * i + 1] - ((str[2 * i + 1] < 'A') ? '0' : ('A' - 10));

	hash[size] = '\0';
	return hash;
}

char *fname_to_hashstr(char *fname, size_t size)
{
	size_t dsize = size << 1;
	char *result = new char [dsize + 1];

	for (size_t i = 0; i < size; i++)
	{
		result[2 * i] = (fname[i] & 0xf0) >> 4;
		result[2 * i + 1] = fname[i] & 0x0f;
	}

	for (size_t i = 0; i < dsize; i++)
		result[i] += (result[i] < 10) ? '0' : ('A' - 10);

	result[dsize] = '\0';

	return result;
}

void create_folder(uint256_t hash)
{
	char *name_folder;
	name_folder = hash_to_str(hash);

	boost::filesystem::path dstFolder = name_folder;
	boost::filesystem::create_directory(dstFolder);

	delete [] name_folder;

	return;
}
void create_file(char *directory, char *data, ull fsize, char *filename)
{
	char *hash_fname = fname_to_hashstr(filename, strlen(filename));

	string dir_str(directory);
	string fname_str(hash_fname);
	string path = dir_str + "//" + fname_str;

	ofstream fout;

	fout.open(path, ofstream::binary);
	fout.write(data, fsize);
	fout.close();

	delete [] hash_fname;
}

void recieve_answer(socket_ptr sock, uint256_t *command_hash, uint256_t *answer, uint256_t key)
{
	uint512_t answer_buffer;

	read(*sock, buffer(&answer_buffer, sizeof(uint512_t)));
	endecrypt_pack((char*)&answer_buffer, sizeof(answer_buffer), key);

	parse_pack(answer_buffer, command_hash, answer);
}
void send_request(socket_ptr sock, uint256_t command, uint256_t auth_hash, uint256_t key)
{
	uint512_t command_hash = make_pack(command, auth_hash);

	endecrypt_pack((char*)&command_hash, sizeof(command_hash), key);
	write(*sock, buffer(&command_hash, sizeof(command_hash)));
}

void recieve_handler(socket_ptr sock)
{
	while (true)
	{
		uint256_t info;
		uint256_t command;
		uint512_t request;

		/*Обмен ключами, расшифрование*/
		uint256_t key = change_keys(sock);

		/*Принимаем запрос с коммандой*/
		recieve_answer(sock, &command, &info, key);

		/*Обработка комманд*/
		if (command == auth_req)
			authentification(command, info, sock);
		else if (command == reg_req)
			add_new_client(command, info, sock);
		else if (command == upload_req)
			upload_file(command, info, sock);
		else if (command == download_req)
			download_file(command, info, sock);
		else if (command == filelist_req)
			get_filelist(command, info, sock);
		else if (command == del_req)
			delete_file(command, info, sock);
		else if (command == rename_req)
			rename_file(command, info, sock);
	}

}

void rename_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	char *name_folder = hash_to_str(auth_hash);

	if (check_login_hash(name_folder, command_hash, sock) == false)
	{
		delete [] name_folder;
		return;
	}

	/*Принимаем старое имя файла.*/
	uint256_t key = change_keys(sock);
	char *encr_last_filename = recieve_encr_filename(sock, key);

	char *hash_str_lf = fname_to_hashstr(encr_last_filename, strlen(encr_last_filename));

	/*Проверяем существование файла.*/
	if (boost::filesystem::exists(string(name_folder)+ "//" + string(hash_str_lf)))
	{
		key = change_keys(sock);
		send_request(sock, command_hash, _true, key);

		/*Принимаем новое имя*/
		key = change_keys(sock);
		char *encr_new_filename = recieve_encr_filename(sock, key);
		char *hash_str_nf = fname_to_hashstr(encr_new_filename, strlen(encr_new_filename));

		key = change_keys(sock);

		if (boost::filesystem::exists(string(name_folder)+ "//" + string(hash_str_nf)))
		{
			send_request(sock, command_hash, _false, key);
		}
		else
		{
			send_request(sock, command_hash, _true, key);

			boost::filesystem::rename(string(name_folder)+ "//" + string(hash_str_lf),
									  string(name_folder)+ "//" + string(hash_str_nf));
		}

		delete [] encr_new_filename;
		delete [] hash_str_nf;
	}
	else
	{
		key = change_keys(sock);
		send_request(sock, command_hash, _false, key);
	}

	delete [] encr_last_filename;
	delete [] hash_str_lf;
}

char *recieve_encr_filename(socket_ptr sock, uint256_t key)
{
	/*Принимаем размер имени файла и само имя.*/
	size_t encr_filename_size;
	read(*sock, buffer((char*)&encr_filename_size, sizeof(size_t)));
	endecrypt_pack((char*)&encr_filename_size, sizeof(size_t), key);

	char *encr_filename = new char[encr_filename_size + 1];
	read(*sock, buffer(encr_filename, encr_filename_size));
	encr_filename[encr_filename_size] = '\0';

	return encr_filename;
}

void get_filelist(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	char *name_folder = hash_to_str(auth_hash);

	if (check_login_hash(name_folder, command_hash, sock) == false)
	{
		delete [] name_folder;
		return;
	}

	boost::filesystem::path client_path(name_folder);

	namespace bf = boost::filesystem;
	char name_pack[64 + sizeof(unsigned char)];

	for (auto i = bf::directory_iterator(client_path); i != bf::directory_iterator(); i++)
	{

	    string fstring = i->path().filename().string();
	    size_t fname_length = fstring.length();

	    char *fname = new char [fname_length + 1];
		copy(fstring.begin(), fstring.end(), fname);
		fname[fname_length] = '\0';

		char *encr_filename = str_to_hash(fname, fname_length >> 1);

		fname_length = fname_length >> 1;

		memcpy(name_pack, encr_filename, fname_length);
		memcpy(name_pack + 64, &fname_length, sizeof(unsigned char));

		write(*sock, buffer(name_pack, 64 + sizeof(unsigned char)));

		delete [] fname;
		delete [] encr_filename;
	 }

	memset(name_pack, 0, 64 + sizeof(unsigned char));
	write(*sock, buffer(name_pack, 64 + sizeof(unsigned char)));
}

bool check_login_hash(char *hash_string, uint256_t command_hash, socket_ptr sock)
{
	uint512_t answer;

	bool status;

	if(boost::filesystem::exists(hash_string)) //is_directory(name_folder)
	{
		answer = make_pack(command_hash, _true);
		status = true;
	}
	else
	{
		answer = make_pack(command_hash, _false);
		status = false;
	}

	uint256_t key = change_keys(sock);

	/*Отправка запроса о существовании такого клиента*/
	endecrypt_pack((char*)&answer, sizeof(answer), key);
	write(*sock, buffer(&answer, sizeof(answer)));

	return status;
}

char *read_fileinfo(socket_ptr sock, ull *file_size)
{
	size_t fileinfo_size = sizeof(unsigned short) + sizeof(ull);
	char fileinfo[fileinfo_size];

	unsigned short fn_length;
	ull fsize;

	uint256_t key = change_keys(sock);

	read(*sock, buffer(fileinfo, fileinfo_size));
	endecrypt_pack(fileinfo, fileinfo_size, key);

	memcpy(&fn_length,	fileinfo, sizeof(unsigned short));
	memcpy(&fsize, fileinfo + sizeof(unsigned short), sizeof(ull));

	char *filename = new char[fn_length + 1];
	read(*sock, buffer(filename, fn_length));

	filename[fn_length] = '\0';

	(*file_size) = fsize;

	return filename;
}
void recieve_file(socket_ptr sock, char *name_folder)
{
	ull fsize;
	char *filename = read_fileinfo(sock, &fsize);

	char *data = new char[fsize];
	read(*sock, buffer(data, fsize));

	create_file(name_folder, data, fsize, filename);

	delete [] data;
	delete [] filename;
}
void send_file(socket_ptr sock, char *name_folder, char *filename)
{
	string dir_str(name_folder);
	string fname_str(filename);
	string path = dir_str + "//" + fname_str;

	uint256_t key = change_keys(sock);

	if (boost::filesystem::exists(path))
	{
		ull fsize;
		char *filebuf = read_file(path, &fsize);

		send_request(sock, (uint256_t)download_req, (uint256_t)fsize, key);

		/*Принятие подтверждения*/
		uint256_t command, answer;
		key = change_keys(sock);

		recieve_answer(sock, &command, &answer, key);

		if (command == download_req && answer == _true)
		{
			/*Отправка файла.*/
			write(*sock, buffer(filebuf, fsize));
		}

		delete [] filebuf;
	}
	else /*Файла не существует*/
	{
		send_request(sock, (uint256_t)download_req, _false, key);
	}

}

char *read_file(string client_path, ull *size)
{
	ifstream readf;

	readf.open(client_path, ios::binary);

	readf.seekg(0, ios_base::end);
	(*size) = readf.tellg();
	readf.seekg(0, ios_base::beg);

	char *file_buf = new char[(*size)];
	readf.read(file_buf, (*size));

	readf.close();

	return file_buf;
}

void download_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	char *name_folder = hash_to_str(auth_hash);

	if (check_login_hash(name_folder, command_hash, sock) == false)
	{
		delete [] name_folder;
		return;
	}

	uint256_t key = change_keys(sock);

	/*Принимаем размер имени файла и само имя.*/
	char *encr_filename = recieve_encr_filename(sock, key);

	/*Преобразуем имя файла в хэш - строку.*/
	char *hash_str = fname_to_hashstr(encr_filename, strlen(encr_filename));

	/*Отправка файла клиенту. Выход, если такого не существует.*/
	send_file(sock, name_folder, hash_str);

	delete [] encr_filename;
	delete [] name_folder;
	delete [] hash_str;
}
void upload_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	uint256_t command;
	uint512_t answer;

	char *name_folder = hash_to_str(auth_hash);

	if (check_login_hash(name_folder, command_hash, sock) == false)
	{
		delete [] name_folder;
		return;
	}

	/*Принятие зашифрованного файла*/
	recieve_file(sock, name_folder);

	/*Отправка подтверждения.*/
	uint256_t key = change_keys(sock);

	answer = make_pack(command_hash, _true);
	endecrypt_pack((char*)&answer, sizeof(answer), key);
	write(*sock, buffer(&answer, sizeof(answer)));

	delete [] name_folder;
}
void delete_file(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	uint256_t command;
	uint512_t answer;

	char *name_folder = hash_to_str(auth_hash);

	if (check_login_hash(name_folder, command_hash, sock) == false)
	{
		delete [] name_folder;
		return;
	}

	uint256_t key = change_keys(sock);

	/*Принимаем размер имени файла и само имя.*/
	char *encr_filename = recieve_encr_filename(sock, key);

	/*Преобразуем имя файла в хэш - строку.*/
	char *hash_str = fname_to_hashstr(encr_filename, strlen(encr_filename));

	string dir_str(name_folder);
	string fname_str(hash_str);
	string path = dir_str + "//" + fname_str;

	key = change_keys(sock);

	/*Если такого файла не существует, отправялем ошибку.*/
	if (boost::filesystem::exists(path) == true)
	{
		boost::filesystem::remove(path);
		send_request(sock, command_hash, _true, key);
	}
	else
	{
		send_request(sock, command_hash, _false, key);
	}

	delete [] hash_str;
	delete [] encr_filename;
	delete [] name_folder;
}

void parse_pack(uint512_t pack, uint256_t *command_hash, uint256_t *info)
{
	uint256_t a = 0xff;
	for (uint i = 1; i < sizeof(uint256_t); i++) a = (a << 8) | 0xff;

	(*info) = (uint256_t)(pack & a);
	(*command_hash) = (uint256_t)(pack >> sizeof(uint256_t) * 8);
}
uint512_t make_pack(uint256_t command_hash, uint256_t info)

{
	uint512_t answer = command_hash;
	answer = answer << sizeof(uint256_t) * 8 | info;

	return answer;
}

void authentification(uint256_t command_hash, uint256_t auth_hash, socket_ptr sock)
{
	char *client_folder = hash_to_str(auth_hash);
	uint512_t answer = command_hash;

	if (boost::filesystem::exists(client_folder))
		answer = (answer << sizeof(uint256_t)*8) | _true;
	else
		answer = (answer << sizeof(uint256_t)*8) | _false;

	/*Обмен ключами*/
	uint256_t key = change_keys(sock);
	endecrypt_pack((char*)&answer, sizeof(uint512_t), key);
	write(*sock, buffer(&answer, sizeof(uint512_t)));

	delete [] client_folder;
}

void client_session(socket_ptr sock)//, socket_ptr connectionSock)
{
	try
	{
		recieve_handler(sock);
	}
	catch (exception &e)
	{
		string ip_string = sock.get()->remote_endpoint().address().to_string();
		unsigned short port = sock.get()->remote_endpoint().port();

		cout << "Client disconnected: " << ip_string << ":" << port << endl;
		sock->close();
	}
}



