#include <ctype.h>
#include <stddef.h>

/*
 * Parse one octet (0-255) starting at s.
 * Returns the number of characters consumed (1-3), or 0 on failure.
 * Rejects: no digits, 4+ digits, values > 255, leading zeros ("01", "007").
 */
static int parseOctet(const char *s, unsigned int *value)
{
    unsigned int v = 0;
    int n = 0;

    while (n < 3 && isdigit((unsigned char)s[n])) {
        v = v * 10 + (unsigned int)(s[n] - '0');
        n++;
    }

    if (n == 0)
        return 0;                       /* no digits at all */
    if (isdigit((unsigned char)s[n]))
        return 0;                       /* 4 or more digits */
    if (n > 1 && s[0] == '0')
        return 0;                       /* leading zero, e.g. "01" */
    if (v > 255)
        return 0;                       /* out of range, e.g. "300" */

    *value = v;
    return n;
}

/*
 * Parse a port (0-65535) starting at s.
 * Returns the number of characters consumed (1-5), or 0 on failure.
 */
static int parsePort(const char *s, int *port)
{
    long v = 0;
    int n = 0;

    while (n < 5 && isdigit((unsigned char)s[n])) {
        v = v * 10 + (s[n] - '0');
        n++;
    }

    if (n == 0)
        return 0;
    if (isdigit((unsigned char)s[n]))
        return 0;                       /* 6 or more digits */
    if (v > 65535)
        return 0;

    *port = (int)v;
    return n;
}

/*
 * Try to match "a.b.c.d" (optionally ":port") beginning exactly at s.
 * Returns 1 on a match and fills *addr / *port, otherwise returns 0.
 */
static int matchAt(const char *s, unsigned long *addr, int *port)
{
    const char *p = s;
    unsigned long a = 0;
    unsigned int octet;
    int i, n;

    for (i = 0; i < 4; i++) {
        if (i > 0) {
            if (*p != '.')
                return 0;
            p++;
        }
        n = parseOctet(p, &octet);
        if (n == 0)
            return 0;
        a = (a << 8) | octet;           /* first octet ends up most significant */
        p += n;
    }

    /* Reject a fifth dotted number, e.g. "1.2.3.4.5".
       A plain trailing period ("...at 10.0.0.1.") is still fine. */
    if (*p == '.' && isdigit((unsigned char)p[1]))
        return 0;

    *port = -1;
    if (*p == ':' && isdigit((unsigned char)p[1])) {
        if (parsePort(p + 1, port) == 0)
            return 0;                   /* colon + digits, but not a valid port */
    }

    *addr = a;
    return 1;
}

//returns 1 if a valid address was found, 0 otherwise.
//On success: *outAddress holds the 32-bit value, and
//*outPort holds the port number, or -1 if no port was present.
//on failure: *outAddress is set to 0 and *outPort is set to -1.
int extractIPv4(const char* str, unsigned long* outAddress, int* outPort)
{
    size_t i;
    unsigned long addr;
    int port;

    /* Set failure values up front so every early return is correct. */
    if (outAddress != NULL)
        *outAddress = 0;
    if (outPort != NULL)
        *outPort = -1;
    if (str == NULL || outAddress == NULL || outPort == NULL)
        return 0;

    for (i = 0; str[i] != '\0'; i++) {
        if (!isdigit((unsigned char)str[i]))
            continue;

        /* Only start at the beginning of a dotted-number run, so we don't
           pull "234.1.1.1" out of "1234.1.1.1" or "2.3.4.5" out of "1.2.3.4.5". */
        if (i > 0 && isdigit((unsigned char)str[i - 1]))
            continue;
        if (i > 1 && str[i - 1] == '.' && isdigit((unsigned char)str[i - 2]))
            continue;

        if (matchAt(str + i, &addr, &port)) {
            *outAddress = addr;
            *outPort = port;
            return 1;
        }
    }

    return 0;
}

#ifdef TEST_EXTRACT
#include <stdio.h>

int main(void)
{
    const char *tests[] = {
        "192.168.1.1",
        "connect to 192.168.1.1:8080 now",
        "server at 10.0.0.1.",
        "0.0.0.0",
        "255.255.255.255:65535",
        "addr=8.8.8.8:",
        "256.1.1.1",
        "1.2.3",
        "1.2.3.4.5",
        "1234.1.1.1",
        "01.2.3.4",
        "1.2.3.4:65536",
        "1.2.3.4:123456",
        "bad 999.1.1.1 then good 1.1.1.1:22",
        "no address here",
        "",
    };
    size_t i;

    for (i = 0; i < sizeof tests / sizeof tests[0]; i++) {
        unsigned long a;
        int port;
        int ok = extractIPv4(tests[i], &a, &port);
        printf("%-40s -> %d  addr=%lu.%lu.%lu.%lu (0x%08lX)  port=%d\n",
               tests[i], ok,
               (a >> 24) & 0xFF, (a >> 16) & 0xFF, (a >> 8) & 0xFF, a & 0xFF,
               a, port);
    }
    return 0;
}
#endif
