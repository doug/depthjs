#include <string.h>

char *strnstr(const char *s1, const char *s2, size_t len)
{
	size_t l1 = len, l2;

	l2 = strlen(s2);
	if (!l2)
	 return (char *)s1;
	while (l1 >= l2) {
	 l1--;
	 if (!memcmp(s1, s2, l2))
	 return (char *)s1;
	 s1++;
	}
	return NULL;
}
