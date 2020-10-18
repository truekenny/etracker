#ifndef SC6_EXIT_CODE_H
#define SC6_EXIT_CODE_H

#define EXIT_CODE_PRINT_ERROR_NO  0
#define EXIT_CODE_PRINT_ERROR_YES 1

// @see https://www.geeksforgeeks.org/exit-codes-in-c-c-with-examples/

#define EXIT_CODE_CHDIR                          250
#define EXIT_CODE_CWD                            249
#define EXIT_CODE_SOCKET_TCP_CREATE              248
#define EXIT_CODE_REUSEADDR                      247
#define EXIT_CODE_BIND_TCP                       246
#define EXIT_CODE_TCP_CLIENT_THREAD              245
#define EXIT_CODE_SOCKET_TCP_END                 244
#define EXIT_CODE_SIZE_CONNECT_REQUEST           243
#define EXIT_CODE_SIZE_CONNECT_RESPONSE          242
#define EXIT_CODE_SIZE_ANNOUNCE_REQUEST          241
#define EXIT_CODE_SIZE_ANNOUNCE_HEAD_RESPONSE    240
#define EXIT_CODE_SIZE_ANNOUNCE_PEER_RESPONSE    239
#define EXIT_CODE_SOCKET_UDP_CREATE              238
#define EXIT_CODE_BIND_UDP                       237
#define EXIT_CODE_UDP_CLIENT_THREAD              236
#define EXIT_CODE_SOCKET_UDP_END                 235
#define EXIT_CODE_UNUSED_LIST                    234
#define EXIT_CODE_LIST_WRONG_LIMIT               233
#define EXIT_CODE_LIST_WRONG_HASH_LENGTH         232
#define EXIT_CODE_LIST_NOT_EMPTY                 231
#define EXIT_CODE_SEMAPHORE_INIT                 230
#define EXIT_CODE_SEMAPHORE_POST                 229
#define EXIT_CODE_SEMAPHORE_DESTROY              228
#define EXIT_CODE_GEOIP_MEMORY                   227
#define EXIT_CODE_GEOIP_WRONG_DATA               226
#define EXIT_CODE_GEOIP_TEST_FAILED              225
#define EXIT_CODE_ARGUMENT_UNKNOWN               224

void exitPrint(int exitCode, char *file, char printError);

#endif //SC6_EXIT_CODE_H
