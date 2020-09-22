#ifndef SC6_BASIC_H
#define SC6_BASIC_H

_Bool hasBasic(char *buffer, char *search);

struct block *randomString(unsigned char size);

void getAuthorizationHeader(struct block *authorizationHeader);

#endif //SC6_BASIC_H
