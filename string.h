#ifndef SC6_STRING_H
#define SC6_STRING_H

#define STRING_RESET   "\033[0m"
#define STRING_BLACK   "\033[30m"      /* Black */
#define STRING_RED     "\033[31m"      /* Red */
#define STRING_GREEN   "\033[32m"      /* Green */
#define STRING_YELLOW  "\033[33m"      /* Yellow */
#define STRING_BLUE    "\033[34m"      /* Blue */
#define STRING_MAGENTA "\033[35m"      /* Magenta */
#define STRING_CYAN    "\033[36m"      /* Cyan */
#define STRING_WHITE   "\033[37m"      /* White */
#define STRING_BOLD_BLACK   "\033[1m\033[30m"      /* Bold Black */
#define STRING_BOLD_RED     "\033[1m\033[31m"      /* Bold Red */
#define STRING_BOLD_GREEN   "\033[1m\033[32m"      /* Bold Green */
#define STRING_BOLD_YELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define STRING_BOLD_BLUE    "\033[1m\033[34m"      /* Bold Blue */
#define STRING_BOLD_MAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define STRING_BOLD_CYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define STRING_BOLD_WHITE   "\033[1m\033[37m"      /* Bold White */
#define STRING_BOLD "\e[1m"            /* Bold */
#define STRING_UNDERLINE "\e[4m"       /* Underline */

_Bool startsWith(const char *start, const char *string);

_Bool endsWith(const char *end, const char *string);

int printHex(char *string, unsigned int len);

#endif //SC6_STRING_H
