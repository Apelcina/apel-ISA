#ifndef APEL_ASSEMBLER_TEXT_UTILS_H
#define APEL_ASSEMBLER_TEXT_UTILS_H

/* Trims leading/trailing whitespace in place (mutates s) and returns a
   pointer to the trimmed start. */
char *trim(char *s);

/* Truncates line at the first '#', mutating it in place. */
void strip_comment(char *line);

#endif
