#ifndef CAT_H
#define CAT_H
int cat(char **dest, char *str, ...);
int catn(char **dest, int len, char *str, ...);
char *cati(char *str, ...);
void freecati (void);
#endif
