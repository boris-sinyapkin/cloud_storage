#include "client.hpp"

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


void create_file(char *filename, char *data, ull fsize, int *error)
{
	ofstream fout;
	fout.open(filename, ofstream::binary);

	if (!fout)
	{
		(*error) = -1;
		return;
	}

	fout.write(data, fsize);
	fout.close();
}

/*Формирование файла для отправки*/
char *read_file(char *filename, ull *size, int *error)
{
	ifstream readf;

	readf.open(filename, ios::binary);

	if (!readf)
	{
		(*error) = -1;
		return NULL;
	}

	readf.seekg(0, ios_base::end);
	(*size) = readf.tellg();
	readf.seekg(0, ios_base::beg);

	char *file_buf = new char[(*size)];
	readf.read(file_buf, (*size));

	readf.close();

	return file_buf;
}
/*Длинна имени файла, размер файла, имя файла*/
void send_fileinfo(char *filename, unsigned short sizeOfName,ull sizeOfFile, ip::tcp::socket &sock)
{
	/*Содержимое пакета: длинна имени файла, размер файла, имя файла.*/
	unsigned int offset = 0;
	char strToSend[sizeof(unsigned short) + sizeof(ull)];

	memcpy(strToSend + offset, &sizeOfName, sizeof(unsigned short));
	offset += sizeof(unsigned short);

	memcpy(strToSend + offset, &sizeOfFile, sizeof(ull));
	offset += sizeof(ull);

	uint256_t key = change_keys(sock);

	endecrypt_pack(strToSend, offset, key);
	write(sock, buffer(strToSend, offset));

	/*Отправляем зашифрованное имя файла.*/
	write(sock, buffer(filename, sizeOfName));
}

void send_file(char *filename, ip::tcp::socket &sock, uint256_t login_hash, int *error)
{
	ull sizeOfFile;
	string uniq_fname = uniq_filename(error);

	if (*error)	return;

	char *my_buf = NULL;
	char *encrypted = string_to_char(uniq_fname);

	/*Шифруем имя файла с помощью хэша от логина.*/
	uint128_t encr_size;
	char *encr_filename = encrypt_buffer(filename, strlen(filename) + 1, &encr_size, login_hash, error);

	if (*error)
	{
		delete [] encrypted;
		return;
	}

	namespace bf = boost::filesystem;

	try
	{
		file_copy(filename, encrypted);
	}
    catch(bf::filesystem_error const & e)
    {
    	delete [] encrypted;
    	(*error) = -1;
    	return;
    }

	/*Считаем CRC файла.*/
	uint32_t crc32 = GetCrc32(encrypted, error);

	if (*error)
	{
		boost::filesystem::remove(boost::filesystem::path(encrypted));
		delete [] encrypted;
		return;
	}

	/*Записываем в конец файла.*/
	ofstream fout;
	fout.open(encrypted, ofstream::binary | ofstream::app);

	if (!fout)
	{
		(*error) = -1;
		boost::filesystem::remove(boost::filesystem::path(encrypted));
		delete [] encrypted;
		return;
	}

	fout.write((char*)&crc32, sizeof(uint32_t));
	fout.close();

	unsigned char key_matrix[4][8];
	key_to_matrix(login_hash, key_matrix);

	/*Шифруем файл с помощью хэша от логина.*/
	encrypt(encrypted, key_matrix, error, defFILE);

	if (*error)
	{
		boost::filesystem::remove(boost::filesystem::path(encrypted));
		delete [] encrypted;
		return;
	}

	my_buf = read_file(encrypted, &sizeOfFile, error);

	if (*error)
	{
		boost::filesystem::remove(boost::filesystem::path(encrypted));
		delete[] my_buf;
		delete [] encrypted;
		return;
	}

	send_fileinfo(encr_filename, (unsigned short)encr_size, sizeOfFile, sock);

	/*sending file*/
	fflush(stdout);
	cout <<"	Uploading: [                    ]";

	unsigned cnt_hashtags = 0;
	for (int i = 0; i < 21; i++) putchar('\b');

	if (sizeOfFile >= 128)
	{
		unsigned parts = sizeOfFile / 16;
		unsigned mod_parts = parts % 20;
		unsigned div_parts = parts / 20 + 1;

		if (mod_parts >= 10)	div_parts++;

		for (ull cur_size = 0; cur_size < sizeOfFile; cur_size+=16)
		{
			if (!((cur_size / 16) % div_parts))
			{
				cout << "#";
				fflush(stdout);
				cnt_hashtags++;
			}

			write(sock, buffer(my_buf + cur_size, 16)); //send file
		}
	}
	else write(sock, buffer(my_buf, sizeOfFile)); //send file

	while (cnt_hashtags < 20)
	{
		cnt_hashtags++;
		cout << "#";
	}
	cout << endl;

	boost::filesystem::remove(boost::filesystem::path(encrypted));

	delete [] encrypted;
	delete[] my_buf;
	free(encr_filename);
}
void recieve_file(char *filename, ip::tcp::socket &sock, uint256_t login_hash, int *error)
{
	/*Шифруем имя файла с помощью хэша от логина.*/
	uint128_t encr_size;
	char *encr_filename = encrypt_buffer(filename, strlen(filename) + 1, &encr_size, login_hash, error);

	if (*error)
	{
		free(encr_filename);
		return;
	}

	uint256_t key = change_keys(sock);

	endecrypt_pack((char*)&encr_size, sizeof(size_t), key);
	write(sock, buffer((char*)&encr_size, sizeof(size_t)));
	write(sock, buffer(encr_filename, (size_t)encr_size));

	/*Проверяем, существует ли файл на сервере.*/
	uint256_t command, answer;
	key = change_keys(sock);
	recieve_answer(sock, &command, &answer, key);

	if (command == download_req && answer != _false && !(answer % 16))
	{
		/*Подтверждаем отправку файла.*/
		key = change_keys(sock);
		send_request(sock, (uint256_t)download_req, _true, key);

		/*Принимаем файл. answer = размер файла.*/
		char *filebuf = new char [(ull)answer];

		/*Скачивание файла.*/
		printf("	Downloading: [                    ]");
		for (int i = 0; i < 21; i++) putchar('\b');
		unsigned cnt_hashtags = 0;

		if ((ull)answer >= 128)
		{
			unsigned parts = (ull)answer / 16;
			unsigned mod_parts = parts % 20;
			unsigned div_parts = parts / 20 + 1;

			if (mod_parts >=10)	div_parts++;

			for (ull cur_size = 0; cur_size < (ull)answer; cur_size+=16)
			{
				if (!((cur_size / 16) % div_parts))
				{
					cout << "#";
					fflush(stdout);
					cnt_hashtags++;
				}

				read(sock, buffer(filebuf + cur_size, 16));
			}
		}
		else read(sock, buffer(filebuf, (ull)answer));

		while (cnt_hashtags < 20)
		{
			cnt_hashtags++;
			cout << "#";
		}
		cout << endl;

		/*Для проверки CRC*/
		string uniq_fname = uniq_filename(error);

		if (*error)
		{
			delete [] filebuf;
			free(encr_filename);
			return;
		}

		char *temp_file = string_to_char(uniq_fname);

		create_file(temp_file, filebuf, (ull)answer, error);

		if (*error)
		{
			delete [] filebuf;
			free(encr_filename);
			delete [] temp_file;
			return;
		}

		unsigned char key_matrix[4][8];
		key_to_matrix(login_hash, key_matrix);

		decrypt(temp_file, key_matrix, error, defFILE);

		delete [] filebuf;

		if (*error)
		{
			free(encr_filename);
			delete [] temp_file;
			return;
		}

		cout << "	CRC checking... " << endl;
		int result_check_crc = check_crc(temp_file, error);

		if (*error)
		{
			free(encr_filename);
			delete [] temp_file;
			return;
		}

		if (!result_check_crc)
		{
			cout << "	File \"" << filename << "\" successfully downloaded." << endl;

			namespace bf = boost::filesystem;

			try
			{
				if (bf::is_directory("Downloads") == false)
				{
					bf::create_directory("Downloads");
				}

				string dir = string("Downloads") + string("//");
				string temp_path = dir + string(filename);
				int copy_num = 0;

				while (bf::exists(temp_path) == true)
				{
					copy_num++;
					temp_path = dir +  string("(") + to_string(copy_num) + string(") ") +
																					string(filename);;
				}

				bf::copy_file(temp_file, temp_path);
			}
		    catch(bf::filesystem_error const & e)
		    {
		        std::cerr << "	" << e.what() << endl;
		    }
		}
		else
		{
			cout << "	Error. File was corrupted. Please, try again." << endl;
		}
		boost::filesystem::remove(temp_file);
		delete [] temp_file;
	}
	else if (command == download_req && answer == _false)
	{
		cout << "	File \"" << filename << "\" does not exist." << endl;
	}
	else
	{
		/*Отменяем отправку файла.*/
		key = change_keys(sock);
		send_request(sock, (uint256_t)download_req, _false, key);
		cout << "	Error. File was corrupted. Please, try again." << endl;
	}


	free(encr_filename);
}

uint256_t convert_to_uint256t(uint8_t array[32])
{
	uint256_t result = (uint256_t)array[0];

	for (int i = 1; i < 32; i++)
	{
		result = (result << 8);
		result += array[i];
	}

	return result;
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

uint256_t string_sha(string l, string p)
{
	uint8_t *inputstr;
	uint8_t *hash;

	char *login = new char[l.size() + 1];
	char *pass = new char[p.size() + 1];

	copy(l.begin(), l.end(), login);
	copy(p.begin(), p.end(), pass);

	login[l.size()] = '\0';
	pass[p.size()] = '\0';
	hash = sha(inputstr, unite(&inputstr, login, pass));

	uint256_t retval = convert_to_uint256t(hash);

	delete [] login;
	delete [] pass;

	free(inputstr);
	free(hash);

	return retval;
}
uint256_t char_sha(char *l, char *r)
{
	uint8_t *inputstr;
	uint8_t *hash;

	hash = sha(inputstr, unite(&inputstr, l, r));

	uint256_t retval = convert_to_uint256t(hash);

	free(inputstr);
	free(hash);

	return retval;
}

string check_hash(string l, string p, ip::tcp::socket &sock, uint256_t *ahash)
{
	uint256_t command, answer;
	uint256_t auth_hash = string_sha(l, p);
	(*ahash) = auth_hash;

	/*Обмен ключами, шифрование*/
	uint256_t key = change_keys(sock);

	send_request(sock, (uint256_t)auth_req, auth_hash, key);

	/*Обмен ключами, расшифрование*/
	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	if (command == auth_req && answer == _false)
			return "Login: \"" + l + "\" not found.";
	else if (command == auth_req && answer == _true)
			return "Login successful.";

	return "";
}

void add_new_client(uint256_t auth_hash, ip::tcp::socket &sock)
{
	uint256_t command, answer;
	uint256_t key = change_keys(sock);

	/*Запрос на добавление нового пользователя*/
	send_request(sock, (uint256_t)reg_req, auth_hash, key);

	/*Обмен ключами, шифрование*/
	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	if (command == reg_req && answer == _true)
		cout << "	Registration successful." << endl;
}

uint256_t authentification(ip::tcp::socket &sock, uint256_t *login_hash)
{
	string login, pass;
	uint256_t auth_hash;

	while (true)
	{
		cout << "Enter your login: ";
		getline(cin, login);

		cout << "Enter your password: ";
		getline(cin, pass);

		string res_str = check_hash(login, pass, sock, &auth_hash);

		cout << "	" << res_str << endl;

		if (res_str == "Login: \"" + login + "\" not found.")
		{
			cout << "	Create new account? (Yes/No): ";
			string answer;
			getline(cin, answer);

			if (answer == "Yes")
				add_new_client(auth_hash, sock);
			else continue;

			break;
		}
		else if (res_str == "Login successful.")
		{
			break;
		}
	}

	(*login_hash) = string_sha(login, "");
	return auth_hash;

}


char *encrypt_buffer(char *buffer, uint128_t buf_size, uint128_t *encr_size, uint256_t key, int *error)
{
	FILE *temp_file;
	string uniq_fname = uniq_filename(error);

	if (*error)	return NULL;

	char *encrypted = string_to_char(uniq_fname);
	unsigned char matrix_key[4][8];

	key_to_matrix(key, matrix_key);

	temp_file = fopen(encrypted, "wb");

	if (!temp_file)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	fwrite(buffer, sizeof(char), (size_t)buf_size, temp_file);
	fclose(temp_file);

	encrypt(encrypted, matrix_key, error, defFILENAME);

	if (*error)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	temp_file = fopen(encrypted, "rb");

	if (!temp_file)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	fseek(temp_file, 0, SEEK_END);
	unsigned long long fsize = ftell(temp_file);
	fseek(temp_file, 0, SEEK_SET);

	(*encr_size) = fsize;
	char *result_buffer = (char*)malloc(sizeof(char)*fsize);
	fread(result_buffer, sizeof(char), fsize, temp_file);
	fclose(temp_file);

	boost::filesystem::remove(boost::filesystem::path(encrypted));

	delete [] encrypted;

	return result_buffer;
}
char *decrypt_buffer(char *buffer, uint128_t buf_size, uint256_t key, int *error)
{
	FILE *temp_file;
	string uniq_fname = uniq_filename(error);

	if (*error)	return NULL;

	char *encrypted = string_to_char(uniq_fname);
	unsigned char matrix_key[4][8];

	key_to_matrix(key, matrix_key);

	temp_file = fopen(encrypted, "wb");

	if (!temp_file)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	fwrite(buffer, sizeof(char), (size_t)buf_size, temp_file);
	fclose(temp_file);

	decrypt(encrypted, matrix_key, error, defFILENAME);

	if (*error)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	temp_file = fopen(encrypted, "rb");

	if (!temp_file)
	{
		delete [] encrypted;
		(*error) = -1;
		return NULL;
	}

	fseek(temp_file, 0, SEEK_END);
	unsigned long long fsize = ftell(temp_file);
	fseek(temp_file, 0, SEEK_SET);

	char *result_buffer = (char*)malloc(sizeof(char)*fsize);
	fread(result_buffer, sizeof(char), fsize, temp_file);
	fclose(temp_file);

	boost::filesystem::remove(boost::filesystem::path(encrypted));

	delete [] encrypted;

	return result_buffer;
}

void recieve_answer(ip::tcp::socket &sock, uint256_t *command_hash, uint256_t *answer, uint256_t key)
{
	uint512_t answer_buffer;

	read(sock, buffer(&answer_buffer, sizeof(uint512_t)));
	endecrypt_pack((char*)&answer_buffer, sizeof(answer_buffer), key);

	parse_pack(answer_buffer, command_hash, answer);
}
void send_request(ip::tcp::socket &sock, uint256_t command, uint256_t auth_hash, uint256_t key)
{
	uint512_t command_hash = make_pack(command, auth_hash);

	endecrypt_pack((char*)&command_hash, sizeof(command_hash), key);
	write(sock, buffer(&command_hash, sizeof(command_hash)));
}


void upload_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash)
{
	uint256_t answer;
	uint256_t command;

	if (!boost::filesystem::exists(filename))
	{
		cout << "	File \"" << filename << "\" does not exist." << endl;
		return;
	}
	else if (boost::filesystem::is_directory(filename))
	{
		cout << "	\"" << filename << "\" is folder." << endl;
		return;
	}

	uint256_t key = change_keys(sock);

	/*Посылем запрос на загрузку файла.*/
	send_request(sock, (uint256_t)upload_req, auth_hash, key);

	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	/*Проверка на существование пользователя*/
	if (command == upload_req && answer == _true)
	{
		int error = 0;
		send_file(filename, sock, login_hash, &error);

		if (error)
		{
			cout <<"	Error occured. Please, try again." << endl;
			return;
		}
	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}

	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	if (command == upload_req && answer == _true)
	{
		cout << "	File \"" << filename << "\" successfully uploaded." << endl;
	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}

}
void download_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash)
{

	uint256_t answer;
	uint256_t command;

	uint256_t key = change_keys(sock);
	/*Посылем запрос на загрузку файла.*/
	send_request(sock, (uint256_t)download_req, auth_hash, key);

	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	/*Проверка на существование пользователя*/
	if (command == download_req && answer == _true)
	{
		int error = 0;
		recieve_file(filename, sock, login_hash, &error);

		if (error)
		{
			cout <<"	Error occured. Please, try again." << endl;
			return;
		}
	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}

}
void delete_file(ip::tcp::socket &sock, uint256_t auth_hash, char *filename, uint256_t login_hash)
{
	uint256_t answer;
	uint256_t command;

	uint256_t key = change_keys(sock);

	/*Посылем запрос на удаление файла.*/
	send_request(sock, (uint256_t)del_req, auth_hash, key);

	key = change_keys(sock);

	recieve_answer(sock, &command, &answer, key);

	/*Проверка на существование пользователя*/
	if (command == del_req && answer == _true)
	{
		/*Отправляем имя файла.*/
		uint256_t key = change_keys(sock);
		int error = 0;
		send_filename(sock, filename, login_hash, key, &error);

		if (error)
		{
			cout <<"	Error occured. Please, try again." << endl;
			return;
		}

		/*Проверяем, существует ли файл на сервере.*/
		uint256_t command, answer;
		key = change_keys(sock);
		recieve_answer(sock, &command, &answer, key);

		if (command == del_req && answer == _true)
		{
			cout << "	File \"" << filename << "\" successfully deleted." << endl;
		}
		else
		{
			cout << "	File \"" << filename << "\" does not exist." << endl;
		}

	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}


}

void get_filelist(ip::tcp::socket &sock, uint256_t auth_hash, uint256_t login_hash)
{
	uint256_t key = change_keys(sock);
	send_request(sock, (uint256_t)filelist_req, auth_hash, key);

	key = change_keys(sock);
	uint256_t command, answer;
	recieve_answer(sock, &command, &answer, key);


	cout <<"	File list:" << endl;
	if (command == filelist_req && answer == _true)
	{
		char name_pack[64 + sizeof(unsigned char)];
		unsigned char name_length;
		char encr_filename[64];
		char *decr_filename;
		int error = 0;
		size_t fni = 0;

		while (true)
		{
			read(sock, buffer(name_pack, 64 + sizeof(unsigned char)));
			memcpy(&name_length, name_pack + 64, sizeof(unsigned char));

			if (name_length == 0) break;
			else
			{
				fni++;
				memcpy(encr_filename, name_pack, name_length);

				decr_filename = decrypt_buffer(encr_filename, name_length, login_hash, &error);

				if (error)
				{
					cout <<"	Error occured. Please, try again." << endl;
					free(decr_filename);
					return;
				}

				cout <<"	" << decr_filename << endl;
				free(decr_filename);
			}
		}

		if (!fni) cout <<"	Empty." << endl;

	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}

}

void send_filename(ip::tcp::socket &sock, char *filename, uint256_t login_hash, uint256_t key, int *error)
{
	uint128_t encr_size;
	char *encr_filename = encrypt_buffer(filename, strlen(filename) + 1, &encr_size, login_hash, error);

	if (*error)
	{
		free(encr_filename);
		return;
	}


	endecrypt_pack((char*)&encr_size, sizeof(size_t), key);
	write(sock, buffer((char*)&encr_size, sizeof(size_t)));
	write(sock, buffer(encr_filename, (size_t)encr_size));

	free(encr_filename);
}

void rename_file(ip::tcp::socket &sock, uint256_t auth_hash, char *last_fname, char *new_fname, uint256_t login_hash)
{
	uint256_t answer;
	uint256_t command;

	uint256_t key = change_keys(sock);
	/*Посылем запрос на переименование файла.*/
	send_request(sock, (uint256_t)rename_req, auth_hash, key);

	key = change_keys(sock);
	recieve_answer(sock, &command, &answer, key);

	/*Проверка на существование пользователя*/
	if (command == rename_req && answer == _true)
	{
		/*Проверяем существование файла с таким названием.*/
		key = change_keys(sock);
		int error = 0;
		send_filename(sock, last_fname, login_hash, key, &error);

		if (error)
		{
			cout <<"	Error occured. Please, try again." << endl;
			return;
		}

		key = change_keys(sock);
		recieve_answer(sock, &command, &answer, key);

		if (command == rename_req && answer == _true)
		{
			/*Если существует - отправляем новое.*/
			key = change_keys(sock);
			int error = 0;
			send_filename(sock, new_fname, login_hash, key, &error);

			if (error)
			{
				cout <<"	Error occured. Please, try again." << endl;
				return;
			}

			/*Проверяем, существует ли переименнованый файл.*/
			key = change_keys(sock);
			recieve_answer(sock, &command, &answer, key);

			if (command == rename_req && answer == _true)
			{
				cout << "	File \"" << last_fname << "\" successfully renamed to \""<< new_fname <<"\"." << endl;
			}
			else if (command == rename_req && answer == _false)
			{
				cout << "	File with this name already exists." << endl;
			}
		}
		else
		{
			cout << "	File \"" << last_fname << "\" does not exist." << endl;
		}
	}
	else
	{
		cout <<"	Error occured. Please, try again." << endl;
		return;
	}


}

int get_command(ip::tcp::socket &sock, uint256_t auth_hash, uint256_t login_hash)
{
	string command_line;
	vector<string> tokens;
	string current_token;

	cout << '>' ;
	getline(cin, command_line);

	split(tokens, command_line, is_any_of(" "));

	current_token = tokens.front();

	if ((current_token == "upload" || current_token == "download" || current_token == "delete")
			&& tokens.size() == 2)
	{
		current_token = tokens.operator[](1);

		if (boost::filesystem::windows_name(current_token)&&
				current_token.find('?') == string::npos &&
				current_token.find('*') == string::npos)
		{
			if (current_token.length() > 64)
			{
				cout << "	Filename \""<< current_token << "\"is too long. Max length is 64." << endl;
				return 1;
			}
		}
		else
		{
			cout << "	Invalid filename: \"" << current_token << "\""<< endl;
			return 1;
		}

		char *filename = string_to_char(current_token);

		if (tokens.front() == "upload")
			upload_file(sock, auth_hash, filename, login_hash);
		else if (tokens.front() == "download")
			download_file(sock, auth_hash, filename, login_hash);
		else if (tokens.front() == "delete")
			delete_file(sock, auth_hash, filename, login_hash);

		delete[] filename;
	}
	else if (current_token == "rename" && tokens.size() == 3)
	{
		bool fst_check = boost::filesystem::windows_name(tokens.operator[](1)) &&
							tokens.operator[](1).find('?') == string::npos &&
							tokens.operator[](1).find('*') == string::npos;
		bool snd_check = boost::filesystem::windows_name(tokens.operator[](2)) &&
							tokens.operator[](2).find('?') == string::npos &&
							tokens.operator[](2).find('*') == string::npos;

		if (fst_check && snd_check)
		{
			if (tokens.operator[](1).length() > 64)
			{
				cout << "	Filename \""<< tokens.operator[](1) << "\"is too long. Max length is 64." << endl;
				return 1;
			}
			if (tokens.operator[](2).length() > 64)
			{
				cout << "	Filename \""<< tokens.operator[](2) << "\"is too long. Max length is 64." << endl;
				return 1;
			}
		}
		else
		{
			cout << "	Invalid filename(s): ";
			if (!fst_check) cout << "\"" << tokens.operator[](1) << "\"";
			if (!snd_check) cout << " \""<< tokens.operator[](2) << "\"";
			cout << endl;

			return 1;
		}


		current_token = tokens.operator[](1);

		char *last_filename = string_to_char(current_token);

		current_token = tokens.operator[](2);

		char *new_filename = string_to_char(current_token);

		rename_file(sock, auth_hash, last_filename, new_filename, login_hash);

		delete [] last_filename;
		delete [] new_filename;
	}
	else if (current_token == "filelist" && tokens.size() == 1)
	{
		get_filelist(sock, auth_hash, login_hash);
	}
	else if (current_token == "exit" && tokens.size() == 1)
	{
		return -1;
	}
	else if (current_token == "help" && tokens.size() == 1)
	{
		show_command_list();
	}
	else
		cout << "	Invalid command. Please, enter the \"help\" to see the commands list." << endl;

	return 1;
}

int check_crc(char *filename, int *error)
{
	uint32_t current_crc32, dload_crc32;

	ifstream fin;
	fin.open(filename, ifstream::binary);

	if (!fin)
	{
		(*error) = -1;
		return 1;
	}
	else (*error) = 0;

	fin.seekg(0, ios::end);
	size_t fsize = fin.tellg();

	/*Записываем пришедший crc*/
	fin.seekg(-1*sizeof(uint32_t), ios::end);
	fin.read((char*)&dload_crc32, sizeof(uint32_t));

	/*Стираем пришедший crc*/
	char *file_data = new char [fsize - sizeof(uint32_t)];
	fin.seekg(0, ios::beg);
	fin.read(file_data, fsize - sizeof(uint32_t));
	fin.close();

	ofstream fout;
	fout.open(filename, ofstream::binary);

	if (!fout)
	{
		(*error) = -1;
		delete [] file_data;
		return 1;
	}
	else (*error) = 0;

	fout.write(file_data, fsize - sizeof(uint32_t));
	delete [] file_data;
	fout.close();

	int error_code;
	/*Считаем текущий crc и сравниваем с пришедшим.*/
	current_crc32 = GetCrc32(filename, &error_code);

	if (error_code)
	{
		(*error) = -1;
		return 1;
	}
	else (*error) = 0;

	if (current_crc32 == dload_crc32) return 0;
	else return -1;
}
uint32_t GetCrc32(char *filename, int *error)
{
	boost::crc_32_type result;

	ifstream fin;
	fin.open(filename, ifstream::binary);

	if (!fin)
	{
		(*error) = -1;
		return 1;
	}

	fin.seekg(0, ios::end);
	size_t fsize = fin.tellg();
	fin.seekg(0, ios::beg);

	char *data = new char [fsize];
	fin.read(data, fsize);
	fin.close();

    result.process_bytes(data, fsize);
    delete [] data;

    (*error) = 0;
    return result.checksum();
}

int parse_servaddr(string command_line,  string &ip_str, int *port)
{
	vector <string> tokens;

	if (command_line.find(" ") == string::npos && command_line.find(":") != string::npos)
	{
			boost::split(tokens, command_line, is_any_of(":"));

			if (tokens.operator [](1) == "" || tokens.operator [](0) == "")
			{
				return -1;
			}

			boost::system::error_code error_code;
			boost::asio::ip::address::from_string(tokens.operator [](0), error_code);

			if (!error_code)
			{
				string port_string = tokens.operator [](1);

				if (port_string.length() <= 5)
				{
					try
					{
						(*port) =  boost::lexical_cast<int>(port_string);
					}
					catch(bad_lexical_cast &e)
					{
						return -1;
					}

					if ((*port) >= 0 && (*port) <= USHRT_MAX)
					{
						ip_str = tokens.operator [](0);
						return 1;
					}
				}
			}
	}

	return -1;
}

void show_command_list(void)
{
	cout << "	Command list:" << endl;
	cout << "	upload [filename] - uploading file to database" << endl;
	cout << "	download [filename] - downloading file from database" << endl;
	cout << "	delete [filename] - deleting file from database" << endl;
	cout << "	rename [last_filename] [new_filename] - rename file in database" << endl;
	cout << "	filelist - show files, stored on database" <<  endl;
	cout << "	exit - quit client session" << endl;
}

