#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <boost/format.hpp>

#define defFILE (1)
#define defFILENAME (0)

void printf_matrix(unsigned char A[4][4]);

long filesize(FILE *file);
void get_new_block(unsigned char State[4][4], FILE *file);
void output_block(unsigned char State[4][4], FILE *encr_file);

//Преобразует столбец матрицы в двойное слово
unsigned long get_dword(unsigned char A[4][4], int index);
unsigned long get_dword_for_256(unsigned char A[4][8], int index);
//Преобразует строку матрицы в двойное слово
unsigned long get_string_dword(unsigned char A[4][4], int index);

FILE* complement_file(FILE *file, int file_size, char final_fname[256], int *error);

unsigned char multiply(unsigned char a, unsigned char b);
// dword в столбец
void dword_to_matrix(unsigned long dword, unsigned char A[4][4], int index);
// dword в строку
void dword_string_to_matrix(unsigned long dword, unsigned char A[4][4], int index);
//Шифрование блока
void encrypt_block(unsigned char State[4][4], unsigned long RoundKeys[64]);
//Расшифрение блока
void decrypt_block(unsigned char State[4][4], unsigned long RoundKeys[64]);

//Циклический свдиг вправо.
void RotWord(unsigned long *dword);

//Замена байтов матрицы State S-Box
void SubBytes(unsigned char State[4][4]);
//Замена байтов матрицы State InvS-Box
void InvSubBytes(unsigned char State[4][4]);

//Замена байтов 4 байтного слова  S-Box
void SubWord(unsigned long  *dword);
//Замена байтов 4 байтного слова  InvS-Box
void InvSubWord(unsigned long  *dword);

//Умножение столбца матрицы State на фиксированную матрицу C
void MixColumns(unsigned char State[4][4]);
//Умножение столбца матрицы State на фиксированную матрицу C
void InvMixColumns(unsigned char State[4][4]);

void InvShiftRows(unsigned char State[4][4]);

void file_copy(char *filename, char *file_to_copy);

unsigned long g(unsigned long key, int round_num);
//Расширение входного ключа на 10 раундов
//void Key_Expansion(unsigned char InputKey[4][4], unsigned long RoundKeys[44]);
void Key_Expansion(unsigned char InputKey[4][8], unsigned long RoundKeys[64]);
// State XOR RoundKey
void AddRoundKey(unsigned char State[4][4], unsigned long RoundKeys[64], int num_key);

//Удаление лишних нулей, создание конечного расшифрованного файла.
void delete_excess_zeroes(FILE *decr_file, char final_fname[], int *error);

//Шифрование текста с помощью AES-128
void encrypt(char final_fname[256], unsigned char InputKey[4][8], int *error, int flag);
//Расшифрование текста с помощью AES-128
void decrypt(char file_fname[256], unsigned char InputKey[4][8], int *error, int flag);
std::string uniq_filename(int *error);
char *string_to_char(std::string str);
