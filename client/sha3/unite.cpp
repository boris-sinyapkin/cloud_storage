#include "keccak.hpp"

size_t unite(uint8_t **inputstr, char *login, char *pass) {
	size_t i = 0;
	int ilogin = 0;
	int i_p = 0;

	*inputstr = (uint8_t *) malloc(strlen(login) + strlen(pass));

	while (login[ilogin] || pass[i_p]) {
		if (login[ilogin])
			(*inputstr)[i++] = (uint8_t) (login[ilogin++]);
		if (pass[i_p])
			(*inputstr)[i++] = (uint8_t) (pass[i_p++]);
	}

	return i;
}