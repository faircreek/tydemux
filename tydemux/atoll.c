#include <stdlib.h>
#include <assert.h>


#define Wchar char
#define CHAR_BIT     8

#define ISSPACE isspace

#define ULLONG_MAX   18446744073709551615ULL
#define LLONG_MAX    9223372036854775807LL
#define LLONG_MIN    (-LLONG_MAX - 1LL)
#define SET_FAIL(X)       ((void)(X)) /* Keep side effects. */



/* This is the main work fuction which handles both strtoll (sflag = 1) and
 * strtoull (sflag = 0). */

unsigned long long _stdlib_strto_ll(register const Wchar * str, Wchar ** endptr, int base, int sflag) {
    
	unsigned long long number;

	unsigned int n1;
	unsigned char negative, digit;

        assert(((unsigned int)sflag) <= 1);

        SET_FAIL(str);

    	while (ISSPACE(*str)) {             /* Skip leading whitespace. */
                	++str;
    	}

	/* Handle optional sign. */
	negative = 0;
	switch(*str) {
                case '-': negative = 1; /* Fall through to increment str. */
                case '+': ++str;
	}

    if (!(base & ~0x10)) {              /* Either dynamic (base = 0) or base 16. */
                base += 10;                             /* Default is 10 (26). */
                if (*str == '0') {
                        SET_FAIL(++str);
                        base -= 2;                      /* Now base is 8 or 16 (24). */
                        if ((0x20|(*str)) == 'x') { /* WARNING: assumes ascii. */
                                ++str;
                                base += base;   /* Base is 16 (16 or 48). */
                        }
                }

                if (base > 16) {                /* Adjust in case base wasn't dynamic. */
                        base = 16;

		}
    }

        number = 0;

    if (((unsigned)(base - 2)) < 35) { /* Legal base. */
                do {
                        digit = (((unsigned char)(*str - '0')) <= 9)
                                ? (*str - '0')
                                : ((*str >= 'A')
                                   ? (((0x20|(*str)) - 'a' + 10)) /* WARNING: assumes ascii. */
                                          : 40);

                        if (digit >= base) {
                                break;
                        }

                        SET_FAIL(++str);

#if 1
                        /* Optional, but speeds things up in the usual case. */
                        if (number <= (ULLONG_MAX >> 6)) {
                                number = number * base + digit;
                        } else
#endif
                        {
                                n1 = ((unsigned char) number) * base + digit;
                                number = (number >> CHAR_BIT) * base;

                                if (number + (n1 >> CHAR_BIT) <= (ULLONG_MAX >> CHAR_BIT)) {
                                        number = (number << CHAR_BIT) + n1;
                                } else {                /* Overflow. */
                                        number = ULLONG_MAX;
                                        negative &= sflag;
                                        //SET_ERRNO(ERANGE);
                                }
                        }

                } while (1);
        }


        {
                unsigned long long tmp = ((negative)
                                                                  ? ((unsigned long long)(-(1+LLONG_MIN)))+1
                                                                  : LLONG_MAX);
                if (sflag && (number > tmp)) {
                        number = tmp;
                        //SET_ERRNO(ERANGE);
                }
        }

        return negative ? (unsigned long long)(-((long long)number)) : number;
}
long long strtoll(const char * str,
                                  char ** endptr, int base)
{
    return (long long) _stdlib_strto_ll(str, endptr, base, 1);
}



long long atoll(const char *nptr)
{
        return strtoll(nptr, (char **) NULL, 10);
}
