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
	if (!(*dest=malloc(len+1)))
		return -1;
	va_start(ap, str);
	while ((tmp = va_arg(ap, char*))){
		len += strlen(tmp);
		if (!(*dest = realloc(*dest, len+1)))
			return -1;
	}
	va_end(ap);
	ptr = *dest;
	for (tmp=str; *tmp; tmp++) *ptr++ = *tmp;
	va_start(ap, str);
	while ((tmp = va_arg(ap, char*)))
		while (*tmp) *ptr++ = *tmp++;
	va_end(ap);
	*ptr = '\0';
	return 0;
}

