#include <stdarg.h>
#include <string.h>

#ifdef __WIN32__
#include <windows.h>
#include <io.h>
#else
#include <stdlib.h>
#include <unistd.h>
#endif 

int cat(char **dest, char *str, ...){
	va_list ap;
	int len;
	char *ptr, *tmp;
	
	if (*dest != NULL)
		free(*dest);
	
	len = strlen(str);
	va_start(ap, str);
	while ((tmp = va_arg(ap, char*)))
		len += strlen(tmp);
	va_end(ap);

	if (!(*dest=malloc(len+1)))
		return -1;

	ptr = *dest;
	for (tmp=str; *tmp; tmp++) *ptr++ = *tmp;
	va_start(ap, str);
	while ((tmp = va_arg(ap, char*)))
		while (*tmp) *ptr++ = *tmp++;
	va_end(ap);
	*ptr = '\0';
	return 0;
}

static struct {
  char *dest;
  size_t buflen;
} catmem;

/* cats all args together and return a pointer to the catted string. Uses a 
   single memory, next call of cat overwrites the last string */
char *cati(char *str, ...) {
  va_list ap;
  size_t len=0;
  char *ptr, *tmp;

  if (!str) return (char*)NULL;

  len = strlen(str);

  va_start(ap, str);
  while ((tmp = va_arg(ap, char*)))
    len += strlen(tmp);
  va_end(ap);

  if (len+1 >= catmem.buflen) {
    if (!(tmp = realloc(catmem.dest, len+1))) 
      return (char*)NULL;
	else { 
      catmem.buflen = len+1;
      catmem.dest = tmp;
    }
  }
  
  ptr = catmem.dest;

  for (tmp=str; *tmp; tmp++) *ptr++ = *tmp;

  va_start(ap, str);
  while ((tmp = va_arg(ap, char*))){
    while (*tmp) *ptr++ = *tmp++;
  }
  va_end(ap);
  *ptr = '\0';
  return catmem.dest; 
}

/* Free the memory allocated by cat */
void freecati (void) {
  free (catmem.dest);
  catmem.dest = 0;
  catmem.buflen = 0;
}
