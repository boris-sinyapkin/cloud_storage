#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "AES_256.hpp"
#include <boost/filesystem.hpp>
#include <iostream>
#include "Tables.h"


#ifdef _WIN32
	#include <windows.h>
#else
	#include <unistd.h>
#endif

std::string uniq_filename(int *error)
{
	std::string reval = "";

	try
	{
		std::string prefix = boost::filesystem::temp_directory_path().string();
		std::string sFilesName = "%%%%-%%%%-%%%%-%%%%";
		std::string suffix = "";

		sFilesName = prefix + sFilesName + suffix;

		boost::filesystem::path temp = boost::filesystem::unique_path(sFilesName);

    	reval = temp.string();
	}
	catch(boost::filesystem::filesystem_error &fe)
	{
		(*error) = -1;
	}

    return reval;
}

char *string_to_char(std::string str)
{
	char *char_str = new char[str.length() + 1];
	copy(str.begin(), str.end(), char_str);
	char_str[str.size()] = '\0';

	return char_str;
}

void file_copy(char *filename, char *file_to_copy)
{
	std::string fromStr = std::string(filename);
	std::string toStr = std::string(file_to_copy);

	boost::filesystem::path to { toStr };
	boost::filesystem::path from { fromStr };

	boost::filesystem::copy_file(from, to);
}

void encrypt(char final_fname[256], unsigned char InputKey[4][8], int *error, int flag)
{
	std::string uniq_fname = uniq_filename(error);

	if (*error) return;

	char *encrypt_f = string_to_char(uniq_fname);

	FILE *decr_file = fopen(final_fname, "rb");
	FILE *encr_file = fopen(encrypt_f, "wb");

	if (!decr_file || !encr_file)
	{
		*error = -1;
		delete [] encrypt_f;
		return;
	}


	long fsize = filesize(decr_file);
	long encrypt_size = 0;
	unsigned long RoundKeys[64];
	unsigned char State[4][4];

	unsigned parts;
	unsigned mod_parts;
	unsigned div_parts = 0;
	unsigned cnt_hashtags = 0;

	Key_Expansion(InputKey, RoundKeys);
	decr_file = complement_file(decr_file, fsize, final_fname, error);


	if (!decr_file || (*error))
	{
		delete [] encrypt_f;
		return;
	}

	fsize = filesize(decr_file);

	if (flag == defFILE)
	{
		printf("	Encrypting: [                    ]");
		for (int i = 0; i < 21; i++) putchar('\b');
	}

	if (fsize > 128)
	{
		 parts = fsize / 16;
		 mod_parts = parts % 20;
		 div_parts = parts / 20;
		 cnt_hashtags = 0;

		if (mod_parts >= 10)	div_parts++;
	}

	long current_parts = 0;

	while (encrypt_size < fsize)
	{

		get_new_block(State, decr_file);
		encrypt_block(State, RoundKeys);
		output_block(State, encr_file);
		encrypt_size = ftell(decr_file);

		if (fsize > 128)
		{
			current_parts++;

			if (div_parts && current_parts % div_parts == 0)
			{
				printf("#");
				fflush(stdout);
				cnt_hashtags++;
			}
		}
	}

	if (flag == defFILE)
	{
		while (cnt_hashtags < 20)
		{
			printf("#");
			fflush(stdout);
			cnt_hashtags++;
		}
		printf("\n");
	}


	fclose(decr_file);
	fclose(encr_file);

	boost::filesystem::remove(final_fname);

	namespace bf = boost::filesystem;

	try
	{
		file_copy(encrypt_f, final_fname);
	}
    catch(bf::filesystem_error const & e)
    {
    	(*error) = -1;
    }

#ifdef _WIN32
	_unlink(encrypt_f);
#else
	unlink(encrypt_f);
#endif

	delete [] encrypt_f;

	return;
}

void decrypt(char file_fname[256], unsigned char InputKey[4][8], int *error, int flag)
{
	std::string uniq_fname = uniq_filename(error);

	if (*error) return;

	char *decrypt_f = string_to_char(uniq_fname);

	FILE *decr_file = fopen(decrypt_f, "wb");
	FILE *encr_file = fopen(file_fname, "rb");

	if (!decr_file || !encr_file)
	{
		*error = -1;
		delete [] decrypt_f;
		return;
	}

	unsigned char State[4][4];

	long fsize = filesize(encr_file);
	long decrypt_size = 0;

	unsigned long RoundKeys[64];

	unsigned parts;
	unsigned mod_parts;
	unsigned div_parts;
	unsigned cnt_hashtags = 0;

	Key_Expansion(InputKey, RoundKeys);
	
	if (flag == defFILE)
	{
		printf("	Decrypting: [                    ]");
		for (int i = 0; i < 21; i++) putchar('\b');
	}

	if (fsize > 128)
	{
		 parts = fsize / 16;
		 mod_parts = parts % 20;
		 div_parts = parts / 20;
		 cnt_hashtags = 0;

		if (mod_parts >= 10)	div_parts++;
	}

	long current_parts = 0;

	while (decrypt_size < fsize)
	{
		get_new_block(State, encr_file);
		decrypt_block(State, RoundKeys);
		output_block(State, decr_file);
		decrypt_size = ftell(encr_file);

		if (fsize > 128)
		{
			current_parts++;

			if (div_parts && current_parts % div_parts == 0)
			{
				printf("#");
				fflush(stdout);
				cnt_hashtags++;
			}
		}
	}

	if (flag == defFILE)
	{
		while (cnt_hashtags < 20)
		{
			printf("#");
			fflush(stdout);
			cnt_hashtags++;
		}
		printf("\n");
	}


	fclose(decr_file);
	fclose(encr_file);


	decr_file = fopen(decrypt_f, "rb");
	delete_excess_zeroes(decr_file, file_fname, error);
	fclose(decr_file);

#ifdef _WIN32
	_unlink(decrypt_f);
#else
	unlink(decrypt_f);
#endif

	delete [] decrypt_f;
}

void printf_matrix(unsigned char A[4][4])
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			//printf(" 0x%2x ", A[i][j]);
			printf(" %x ", A[i][j]);
		}
		printf("\n");
	}
	printf("\n");
}

//����������� ����� �����.
void RotWord(unsigned long  *dword)
{
	unsigned long b = 0xff000000;

	b &= *dword;
	b >>= 24;
	*dword &= 0x00ffffff;
	*dword <<= 8;
	*dword |= b;
}
//����������� ����� ������.
void RotWord_right(unsigned long  *dword)
{
	unsigned long b = 0x000000ff;

	b &= *dword;
	b <<= 24;
	*dword &= 0xffffff00;
	*dword >>= 8;
	*dword |= b;
}

unsigned char multiply(unsigned char a, unsigned char b)
{
	switch (b)
	{
	case 2:
		return table_2[a];
	case 3:
		return table_3[a];
	case 14:
		return table_14[a];
	case 11:
		return table_11[a];
	case 13:
		return table_13[a];
	case 9:
		return table_9[a];
	default:
		return a;
	}
}

void SubWord(unsigned long  *dword)
{
	unsigned long i;
	unsigned long j;
	unsigned long tmp_dword = *dword;
	unsigned long result_dword = 0;

	for (int k = 0; k < 4; k++)
	{
		i = 0xf0000000;
		j = 0x0f000000;
		i &= tmp_dword;
		j &= tmp_dword;
		i >>= 28;
		j >>= 24;
		result_dword <<= 8;
		result_dword+=S_Box[i][j];
		tmp_dword <<= 8;
	}

	for (i = 0; i < 4; i++)
		RotWord(&result_dword);

	*dword = result_dword;
}
void InvSubWord(unsigned long  *dword)
{
	unsigned long i;
	unsigned long j;
	unsigned long tmp_dword = *dword;
	unsigned long result_dword = 0;

	for (int k = 0; k < 4; k++)
	{
		i = 0xf0000000;
		j = 0x0f000000;
		i &= tmp_dword;
		j &= tmp_dword;
		i >>= 28;
		j >>= 24;
		result_dword <<= 8;
		result_dword += InvS_Box[i][j];
		tmp_dword <<= 8;
	}

	for (i = 0; i < 4; i++)
		RotWord(&result_dword);

	*dword = result_dword;
}

void SubBytes(unsigned char State[4][4])
{
	int i = 0;
	unsigned long dword;

	for (i = 0; i < 4; i++)
	{
		dword = get_dword(State, i);
		SubWord(&dword);
		dword_to_matrix(dword, State, i);
	}
}
void InvSubBytes(unsigned char State[4][4])
{
	int i = 0;
	unsigned long dword;

	for (i = 0; i < 4; i++)
	{
		dword = get_dword(State, i);
		InvSubWord(&dword);
		dword_to_matrix(dword, State, i);
	}
}

void ShiftRows(unsigned char State[4][4])
{
	unsigned long dword;

	for (int i = 0; i < 4; i++)
	{
		dword = get_string_dword(State, i);
		for (int j = 0; j < i; j++)
		{
			RotWord(&dword);
		}
		dword_string_to_matrix(dword, State, i);
	}

}
void InvShiftRows(unsigned char State[4][4])
{
	unsigned long dword;

	for (int i = 0; i < 4; i++)
	{
		dword = get_string_dword(State, i);
		for (int j = 0; j < i; j++)
		{
			RotWord_right(&dword);
		}
		dword_string_to_matrix(dword, State, i);
	}

}

void MixColumns(unsigned char State[4][4])
{
	unsigned char C[4][4] =
	{
		{ 0x02, 0x03, 0x01, 0x01 },
		{ 0x01, 0x02, 0x03, 0x01 },
		{ 0x01, 0x01, 0x02, 0x03 },
		{ 0x03, 0x01, 0x01, 0x02 }
	};
	unsigned char Temp_array[4][4];
	unsigned char s = 0;
	unsigned char res;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			s = 0;
			for (int k = 0; k < 4; k++)
			{
				res = multiply(State[k][i], C[j][k]);
				s ^= res;
			}
			Temp_array[j][i] = s;
		}
	}

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			State[i][j] = Temp_array[i][j];
}
void InvMixColumns(unsigned char State[4][4])
{
	unsigned char C[4][4] =
	{
		{ 0x0e, 0x0b, 0x0d, 0x09 },
	{ 0x09, 0x0e, 0x0b, 0x0d },
	{ 0x0d, 0x09, 0x0e, 0x0b },
	{ 0x0b, 0x0d, 0x09, 0x0e }
	};
	unsigned char Temp[4][4];
	unsigned char s = 0;

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			s = 0;
			for (int k = 0; k < 4; k++)
				s ^= multiply(State[k][i], C[j][k]);
			Temp[j][i] = s;
		}
	}

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 4; j++)
			State[i][j] = Temp[i][j];
}

void AddRoundKey(unsigned char State[4][4], unsigned long RoundKeys[64], int num_key)
{
	unsigned long dword_state;

	for (int i = 0; i < 4; i++)
	{
		dword_state = get_dword(State, i);
		dword_state ^= RoundKeys[i + num_key];
		dword_to_matrix(dword_state, State, i);
	}
}

long filesize(FILE *file)
{
	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	fseek(file, 0, SEEK_SET);

	return size;
}
void get_new_block(unsigned char State[4][4], FILE *file)
{

	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			State[j][i] = (unsigned char)getc(file);
		}
	}
}

//����������� ������� ������� � ������� �����
unsigned long get_dword(unsigned char A[4][4], int index)
{
	unsigned long  dword = 0;

	for (int i = 0; i < 4; i++)
	{
		dword = dword << 8;
		dword += A[i][index];
	}

	return dword;
}
unsigned long get_dword_for_256(unsigned char A[4][8], int index)
{
	unsigned long  dword = 0;

	for (int i = 0; i < 4; i++)
	{
		dword = dword << 8;
		dword += A[i][index];
	}

	return dword;
}
//����������� ������ ������� � ������� �����
unsigned long get_string_dword(unsigned char A[4][4], int index)
{
	unsigned long  dword = 0;

	for (int i = 0; i < 4; i++)
	{
		dword = dword << 8;
		dword += A[index][i];
	}

	return dword;
}
// dword � �������
void dword_to_matrix(unsigned long dword, unsigned char A[4][4], int index)
{
	unsigned long temp_dword;
	for (int i = 3; i >-1; i--)
	{
		temp_dword=0x00ff;
		temp_dword &= dword;
		dword = dword >> 8;
		A[i][index] = (unsigned char)temp_dword;
	}
}
// dword � ������
void dword_string_to_matrix(unsigned long dword, unsigned char A[4][4], int index)
{
	unsigned long temp_dword;
	for (int i = 3; i >-1; i--)
	{
		temp_dword = 0x00ff;
		temp_dword &= dword;
		dword = dword >> 8;
		A[index][i] = (unsigned char)temp_dword;
	}

}

FILE* complement_file(FILE *file, int file_size, char final_fname[256], int *error)
{
	fclose(file);
	file = fopen(final_fname, "ab");
	char count_zero;

	if (!file)
	{
		(*error) = -1;
		return 0;
	}

	if ((count_zero=(char)(file_size % 16)) != 15)
	{
		count_zero = 15 - count_zero;
		for (int i = 0; i < count_zero; i++)
			fputc(0x00, file);

	}
	else count_zero = 0;

	fputc(count_zero, file);
	fclose(file);

	file = fopen(final_fname, "rb");

	return file;
}

void encrypt_block(unsigned char State[4][4], unsigned long RoundKeys[64])
{
	int round = 0;

	AddRoundKey(State, RoundKeys, 0);
	for (round = 1; round <= 13; round++)
	{
		SubBytes(State);
		ShiftRows(State);
		MixColumns(State);
		AddRoundKey(State, RoundKeys, round * 4);
	}

	SubBytes(State);
	ShiftRows(State);
	AddRoundKey(State, RoundKeys, round * 4);
}
void decrypt_block(unsigned char State[4][4], unsigned long RoundKeys[64])
{
	int round = 0;

	AddRoundKey(State, RoundKeys, 56);

	for (round = 1; round <= 13; round++)
	{
		InvSubBytes(State);
		InvShiftRows(State);
		AddRoundKey(State, RoundKeys, 56 - round * 4);
		InvMixColumns(State);
	}

	InvSubBytes(State);
	InvShiftRows(State);
	AddRoundKey(State, RoundKeys, 0);
}

unsigned long g(unsigned long key, int round_num)
{
	unsigned long rcon_value = rcon[round_num / 4 + 1] << 24;
	
	RotWord(&key);
	SubWord(&key);
	key = key ^ rcon_value;

	return key;
}
void Key_Expansion(unsigned char InputKey[4][8], unsigned long RoundKeys[64])
{
	int i;

	for (i = 0; i < 8; i++)
		RoundKeys[i] = get_dword_for_256(InputKey, i);

	for (i = 0; i < 56; i += 8)
	{
		RoundKeys[i + 8] = RoundKeys[i] ^ g(RoundKeys[i + 7], i);
		RoundKeys[i + 9] = RoundKeys[i + 8] ^ RoundKeys[i + 1];
		RoundKeys[i + 10] = RoundKeys[i + 9] ^ RoundKeys[i + 2];
		RoundKeys[i + 11] = RoundKeys[i + 10] ^ RoundKeys[i + 3];
		RoundKeys[i + 12] = RoundKeys[i + 11] ^ RoundKeys[i + 4];
		RoundKeys[i + 13] = RoundKeys[i + 12] ^ RoundKeys[i + 5];
		RoundKeys[i + 14] = RoundKeys[i + 13] ^ RoundKeys[i + 6];
		RoundKeys[i + 15] = RoundKeys[i + 14] ^ RoundKeys[i + 7];
	}

}
void output_block(unsigned char State[4][4], FILE *encr_file)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			fwrite(&State[j][i], sizeof(unsigned char), 1, encr_file);
		}
	}
}

void delete_excess_zeroes(FILE *decr_file, char final_fname[], int *error)
{
	int cnt_zeroes;
	long fsize;
	FILE *final_file;

	fsize = filesize(decr_file);
	fseek(decr_file, -1, SEEK_END);
	cnt_zeroes = fgetc(decr_file);
	fseek(decr_file, 0, SEEK_SET);

#ifdef _WIN32
	_unlink(final_fname);
#else
	unlink(final_fname);
#endif

	final_file = fopen(final_fname, "wb");

	if (!final_file)
	{
		(*error) = -1;
		return;
	}


	char *filebuf = new char[fsize - cnt_zeroes - 1];

	fread(filebuf, sizeof(unsigned char), fsize - cnt_zeroes - 1, decr_file);
	fwrite(filebuf, sizeof(unsigned char), fsize - cnt_zeroes - 1, final_file);
	fclose(final_file);

	delete [] filebuf;
}
