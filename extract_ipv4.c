#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* Only digits, '.', and ':' can be part of an address token.
   Every other character separates tokens. */
static int isTokenChar(char c)
{
    return isdigit((unsigned char)c) || c == '.' || c == ':';
}

/*
 * Parse a decimal number from s[*pos] within a token of length len.
 * Rules: 1..maxDigits digits, value <= maxValue, no leading zeros
 * unless the number is exactly "0". Digits are converted by hand.
 * On success returns 1, stores the value, and advances *pos past the digits.
 * Returns 0 on failure.
 */
static int parseNumber(const char *s, size_t len, size_t *pos,
                       int maxDigits, unsigned long maxValue,
                       unsigned long *value)
{
    size_t start = *pos;
    unsigned long v = 0;
    int n = 0;

    while (*pos < len && n < maxDigits && isdigit((unsigned char)s[*pos])) {
        v = v * 10 + (unsigned long)(s[*pos] - '0');
        (*pos)++;
        n++;
    }

    if (n == 0)
        return 0;                                   /* no digits */
    if (*pos < len && isdigit((unsigned char)s[*pos]))
        return 0;                                   /* too many digits */
    if (n > 1 && s[start] == '0')
        return 0;                                   /* leading zero */
    if (v > maxValue)
        return 0;                                   /* out of range */

    *value = v;
    return 1;
}

/*
 * The whole token s[0..len) must match
 *     octet '.' octet '.' octet '.' octet [ ':' port ]
 * exactly. There is no trimming or partial matching.
 */
static int matchToken(const char *s, size_t len,
                      unsigned long *addr, int *port)
{
    size_t pos = 0;
    unsigned long a = 0;
    unsigned long v;
    int i;

    for (i = 0; i < 4; i++) {
        if (i > 0) {
            if (pos >= len || s[pos] != '.')
                return 0;
            pos++;
        }
        if (!parseNumber(s, len, &pos, 3, 255UL, &v))
            return 0;
        a = (a << 8) | v;               /* first octet ends up most significant */
    }

    if (pos == len) {                   /* token ended: address with no port */
        *addr = a;
        *port = -1;
        return 1;
    }

    /* Anything after the fourth octet must be ':' followed by a fully
       valid port and then the end of the token. */
    if (s[pos] != ':')
        return 0;                       /* e.g. "10.0.0.1." */
    pos++;
    if (!parseNumber(s, len, &pos, 5, 65535UL, &v))
        return 0;                       /* e.g. "1.1.1.1:", "::80", ":080" */
    if (pos != len)
        return 0;                       /* e.g. "1.1.1.1:80:" */

    *addr = a;
    *port = (int)v;
    return 1;
}

//returns 1 if a valid address was found, 0 otherwise.
//On success: *outAddress holds the 32-bit value, and
//*outPort holds the port number, or -1 if no port was present.
//on failure: *outAddress is set to 0 and *outPort is set to -1.
int extractIPv4(const char* str, unsigned long* outAddress, int* outPort)
{
    size_t i = 0;
    size_t start;
    unsigned long addr;
    int port;

    if (outAddress != NULL)
        *outAddress = 0;
    if (outPort != NULL)
        *outPort = -1;
    if (str == NULL || outAddress == NULL || outPort == NULL)
        return 0;

    while (str[i] != '\0') {
        if (!isTokenChar(str[i])) {     /* garbage: skip it */
            i++;
            continue;
        }

        start = i;                      /* take the maximal token */
        while (str[i] != '\0' && isTokenChar(str[i]))
            i++;

        if (matchToken(str + start, i - start, &addr, &port)) {
            *outAddress = addr;
            *outPort = port;
            return 1;
        }
        /* Invalid token: keep scanning for a later one. */
    }

    return 0;
}

#ifdef TEST_EXTRACT

typedef struct {
    const char *input;
    int ok;
    unsigned long addr;
    int port;
} TestCase;

int main(void)
{
    const TestCase tests[] = {
        /* original 16 cases (two expectations changed by the spec) */
        { "192.168.1.1",                        1, 0xC0A80101UL, -1 },
        { "connect to 192.168.1.1:8080 now",    1, 0xC0A80101UL, 8080 },
        { "server at 10.0.0.1.",                0, 0, -1 },
        { "0.0.0.0",                            1, 0x00000000UL, -1 },
        { "255.255.255.255:65535",              1, 0xFFFFFFFFUL, 65535 },
        { "addr=8.8.8.8:",                      0, 0, -1 },
        { "256.1.1.1",                          0, 0, -1 },
        { "1.2.3",                              0, 0, -1 },
        { "1.2.3.4.5",                          0, 0, -1 },
        { "1234.1.1.1",                         0, 0, -1 },
        { "01.2.3.4",                           0, 0, -1 },
        { "1.2.3.4:65536",                      0, 0, -1 },
        { "1.2.3.4:123456",                     0, 0, -1 },
        { "bad 999.1.1.1 then good 1.1.1.1:22", 1, 0x01010101UL, 22 },
        { "no address here",                    0, 0, -1 },
        { "",                                   0, 0, -1 },
        /* new cases for the whole-token, colon, and port leading-zero rules */
        { ".1.1.1.1",                           0, 0, -1 },
        { "1.1.1.1:80:",                        0, 0, -1 },
        { "1.1.1.1:",                           0, 0, -1 },
        { "1.1.1.1::80",                        0, 0, -1 },
        { "1.1.1.1:080",                        0, 0, -1 },
        { "1.1.1.1:00",                         0, 0, -1 },
        { "1.1.1.1:0",                          1, 0x01010101UL, 0 },
        { "ip=10.0.0.1, next",                  1, 0x0A000001UL, -1 },
        { "10.0.0.1. then 10.0.0.2:443",        1, 0x0A000002UL, 443 },
        { "x1.2.3.4y",                          1, 0x01020304UL, -1 },
    };
    size_t i, failures = 0;
    size_t count = sizeof tests / sizeof tests[0];

    for (i = 0; i < count; i++) {
        unsigned long a;
        int port;
        int ok = extractIPv4(tests[i].input, &a, &port);
        int pass = ok == tests[i].ok && a == tests[i].addr && port == tests[i].port;
        if (!pass)
            failures++;
        printf("%s  %-36s -> %d  addr=0x%08lX  port=%d\n",
               pass ? "PASS" : "FAIL", tests[i].input, ok, a, port);
    }
    printf("\n%lu/%lu passed\n", (unsigned long)(count - failures),
           (unsigned long)count);
    return failures == 0 ? 0 : 1;
}

#else

#define LINE_MAX_LEN 1024

int main(void)
{
    char line[LINE_MAX_LEN];

    for (;;) {
        size_t len;
        unsigned long addr;
        int port;

        printf("Enter a line of text (or END to quit): ");
        fflush(stdout);

        if (fgets(line, sizeof line, stdin) == NULL)
            break;                              /* EOF or read error */

        len = strlen(line);
        if (len > 0 && line[len - 1] != '\n' && !feof(stdin)) {
            /* Line too long for the buffer: discard the rest of it
               instead of treating it as a new line of input. */
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            printf("Input line too long (max %d characters). Please try again.\n",
                   LINE_MAX_LEN - 2);
            continue;
        }

        /* Strip trailing newline (and '\r' from Windows line endings). */
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (strcmp(line, "END") == 0)
            break;

        if (extractIPv4(line, &addr, &port)) {
            printf("Extracted IPv4 address: %lu.%lu.%lu.%lu (decimal value: %lu, port: ",
                   (addr >> 24) & 0xFFUL, (addr >> 16) & 0xFFUL,
                   (addr >> 8) & 0xFFUL, addr & 0xFFUL, addr);
            if (port >= 0)
                printf("%d)\n", port);
            else
                printf("none)\n");
        } else {
            printf("No valid IPv4 address found in the input.\n");
        }
    }

    printf("Program terminated.\n");
    return 0;
}

#endif
