# Version 1

## Prompt 1

> I'm writing a C program for a class assignment. I need a function that finds an IPv4 address inside a line of text. It has to use this exact prototype:
>
> ```c
> //returns 1 if a valid address was found, 0 otherwise.
> //On success: *outAddress holds the 32-bit value, and
> //*outPort holds the port number, or -1 if no port was present.
> //on failure: *outAddress is set to 0 and *outPort is set to -1.
> int extractIPv4(const char* str, unsigned long* outAddress, int* outPort);
> ```
>
> An address is four numbers separated by periods, like 192.168.1.1, and it can optionally be followed by a colon and a port number, like 192.168.1.1:8080.
>
> Restrictions: I can't use any string-to-number functions (atoi, atol, strtol, strtoul, sscanf, etc.), any address-parsing functions (inet_aton, inet_pton, inet_addr), or any regex library. All digit parsing has to be done by hand, character by character. isdigit() is allowed.
>
> Can you write the function and explain how it works?

## AI Output 1

*Copied directly from the AI. The code it produced is in `extract_ipv4.c` in this folder, unchanged.*

> I'll write it, then compile it with warnings and sanitizers enabled and run it against a set of tricky inputs.
>
> The code compiles cleanly with `-Wall -Wextra -pedantic`. It also runs clean under AddressSanitizer and UndefinedBehaviorSanitizer on all 16 test cases.
>
> **How it works**
>
> The code is split into three layers, with a small helper for each job.
>
> **`extractIPv4` scans for a starting point.** First it sets the failure values (`0` and `-1`), so every path that returns 0 already has the outputs right. It also guards against NULL pointers. Then it walks the string and tries a match at each digit that begins a dotted-number run. The two `continue` checks skip any digit that comes right after another digit, or after a `.` that follows a digit. Without them, `"1234.1.1.1"` would produce a false match on `234.1.1.1`, and `"1.2.3.4.5"` would produce one on `2.3.4.5`. If a candidate fails, such as `999.1.1.1`, the scan keeps going, so a valid address later in the line is still found.
>
> **`matchAt` checks one candidate.** It reads four octets separated by dots. As it goes, it builds the 32-bit value with `a = (a << 8) | octet`, which puts the first octet in the most significant byte. So `192.168.1.1` becomes `0xC0A80101`. After the fourth octet, it rejects a `.` followed by a digit, because that means there's a fifth number. A plain period, like the end of a sentence, is fine. If a `:` is followed by a digit, it parses a port. A colon with no digits after it just means no port was given.
>
> **`parseOctet` and `parsePort` do the manual digit conversion.** Each digit is added with `v = v * 10 + (c - '0')`. The loops stop at 3 digits for an octet and 5 digits for a port, so the value can never overflow. After the loop, one more check makes sure the next character isn't also a digit, which catches inputs that are too long. Then the range is checked: 0–255 for an octet, 0–65535 for a port.
>
> **Safety details**
>
> - Every `isdigit` call casts its argument to `unsigned char`. Passing a negative `char` (possible with non-ASCII bytes) to `isdigit` is undefined behavior.
> - The code never reads past the null terminator. Each lookahead like `s[n]` or `p[1]` happens only after the character before it is known not to be `'\0'`.
> - The shifted address fits in `unsigned long`, which the C standard guarantees is at least 32 bits.
>
> **Choices to check against your assignment spec**
>
> 1. **Leading zeros are rejected** (`01.2.3.4` fails). Some parsers, like `inet_aton`, read these as octal, so rejecting them is the safer choice. Remove that one check if your spec allows them.
> 2. **An invalid port rejects the whole match.** `1.2.3.4:65536` returns 0 instead of returning the address with no port. If your instructor wants the other behavior, change `return 0` to `*port = -1` in that branch.
> 3. **Port 0 is accepted.** It's reserved, so change the range check to `v < 1 || v > 65535` if it shouldn't count.
> 4. **Byte order**: the first octet is the high byte (host order). If you need network byte order, that's a separate `htonl` step.
>
> To run the built-in tests, compile with `gcc -DTEST_EXTRACT extract_ipv4.c`. Without that flag, the file contains only the function, ready to drop into your project.

## Test Results 1 (my own run)

```
PS C:\Users\viduy\Downloads> gcc -DTEST_EXTRACT extract_ipv4.c -o extract_ipv4.exe
PS C:\Users\viduy\Downloads> ./extract_ipv4.exe
192.168.1.1                              -> 1  addr=192.168.1.1 (0xC0A80101)  port=-1
connect to 192.168.1.1:8080 now          -> 1  addr=192.168.1.1 (0xC0A80101)  port=8080
server at 10.0.0.1.                      -> 1  addr=10.0.0.1 (0x0A000001)  port=-1
0.0.0.0                                  -> 1  addr=0.0.0.0 (0x00000000)  port=-1
255.255.255.255:65535                    -> 1  addr=255.255.255.255 (0xFFFFFFFF)  port=65535
addr=8.8.8.8:                            -> 1  addr=8.8.8.8 (0x08080808)  port=-1
256.1.1.1                                -> 0  addr=0.0.0.0 (0x00000000)  port=-1
1.2.3                                    -> 0  addr=0.0.0.0 (0x00000000)  port=-1
1.2.3.4.5                                -> 0  addr=0.0.0.0 (0x00000000)  port=-1
1234.1.1.1                               -> 0  addr=0.0.0.0 (0x00000000)  port=-1
01.2.3.4                                 -> 0  addr=0.0.0.0 (0x00000000)  port=-1
1.2.3.4:65536                            -> 0  addr=0.0.0.0 (0x00000000)  port=-1
1.2.3.4:123456                           -> 0  addr=0.0.0.0 (0x00000000)  port=-1
bad 999.1.1.1 then good 1.1.1.1:22       -> 1  addr=1.1.1.1 (0x01010101)  port=22
no address here                          -> 0  addr=0.0.0.0 (0x00000000)  port=-1
                                         -> 0  addr=0.0.0.0 (0x00000000)  port=-1
```

## Analysis of Version 1

**What it did correctly:** The code follows the restrictions. There's no `atoi`/`strtol`/`sscanf`, no `inet_*` and no regex, and digits are accumulated by hand with `v = v * 10 + (c - '0')`. `parseOctet` enforces 1–3 digits, the 0–255 range and the leading-zero rule. `parsePort` enforces 1–5 digits and 0–65535. Digit loops stop at the max length before accumulating, so they can't overflow. Every `isdigit` call casts to `unsigned char`, which avoids undefined behavior on negative `char` values. When a candidate fails, such as `999.1.1.1`, the scan keeps going and still finds a later valid address.

**What I observed:** I compiled the AI's own test runner (`gcc -DTEST_EXTRACT`) and ran it. Two of its 16 cases give results that contradict the assignment spec:

| Input | Returned | Spec requires |
|---|---|---|
| `server at 10.0.0.1.` | 1 | 0: a period directly touches the address |
| `addr=8.8.8.8:` | 1, no port | 0: if a colon is present, the port must be valid or the whole match is rejected |

These weren't accidents the AI overlooked. It stated both as design decisions in its explanation: "A plain period, like the end of a sentence, is fine" and "A colon with no digits after it just means no port was given." Its test runner didn't catch them because it only printed the results. It never compared them to expected values, so any output counted as a pass. The AI said the code "runs clean on all 16 test cases," but that only meant nothing crashed, not that the results were correct according to the assignment spec.

**Other gaps:** The file doesn't include the required interactive input loop. Its `main` is only a demo runner behind `#ifdef TEST_EXTRACT`, so it prints fixed examples instead of reading input until the user types `END`. Its closing notes also suggested loosening the rules, for example "Remove that one check if your spec allows [leading zeros]." That would move the code further from the spec. Following the AI's suggestions without checking them against the assignment would have introduced errors.

**Next step (Version 2):** My first prompt didn't give the AI the assignment's strict rules, so it filled the gaps with its own assumptions, and two of those assumptions were wrong. In Version 2, I'll give it the rules it's missing:

- If a colon follows the address, the port must be fully valid, or the whole match is rejected.
- The port has the same leading-zero rule as octets.
- A stray `.` or `:` touching the address makes the whole token invalid. No trimming to find a valid piece.

Then I'll rerun the tests with my own cases added, with expected results, so a wrong answer shows up as a failure instead of just being printed.
