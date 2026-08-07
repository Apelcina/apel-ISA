#include "text_utils.h"
#include <ctype.h>
#include <string.h>

char *trim(char *s) {
    while (isspace((unsigned char)*s)) {
        s++;
    }
    if (*s == '\0') {
        return s;
    }
    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
    return s;
}

void strip_comment(char *line) {
    char *hash = strchr(line, '#');
    if (hash) {
        *hash = '\0';
    }
}
