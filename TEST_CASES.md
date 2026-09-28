# Test Cases

Test cases for the final `extract_ipv4.c` (the Version 2 code). **Expected** comes from the assignment spec, not from the AI. **Actual** is what `extractIPv4` returned when the code was compiled and run.

Notation: `1 A.B.C.D, port P` means the function returned 1 with that address and port. `0` means rejected: the function returned 0 with `outAddress = 0` and `outPort = -1`.

## 1. Valid addresses in noisy text

| # | Input | Expected | Actual | Result |
|---|---|---|---|---|
| 1 | `192.168.1.1` | 1 192.168.1.1 (3232235777), port none | 1 192.168.1.1 (3232235777), port none | PASS |
| 2 | `fw: allow src=10.0.0.1:443 dst=x` | 1 10.0.0.1, port 443 | 1 10.0.0.1 (167772161), port 443 | PASS |
| 3 | `[172.16.0.5:8080]` | 1 172.16.0.5, port 8080 | 1 172.16.0.5 (2886729733), port 8080 | PASS |
| 4 | `12:30:45 host 9.9.9.9 up` | 1 9.9.9.9 (timestamp token is invalid and skipped) | 1 9.9.9.9 (151587081), port none | PASS |
| 5 | `bad 999.1.1.1 then good 1.1.1.1:22` | 1 1.1.1.1, port 22 (bad token skipped) | 1 1.1.1.1 (16843009), port 22 | PASS |

## 2. Ambiguous input

| # | Input | Expected | Actual | Result |
|---|---|---|---|---|
| 6 | `1.1.1.1 and 2.2.2.2` | 1 1.1.1.1, the first valid address (only one is extracted per line) | 1 1.1.1.1, port none | PASS |
| 7 | `x1.2.3.4y` | 1 1.2.3.4 (letters are garbage, so they separate tokens) | 1 1.2.3.4, port none | PASS |
| 8 | `1. 2.3.4` | 0 (the space splits it into `1.` and `2.3.4`, both invalid) | 0 | PASS |

## 3. Malformed input: wrong structure

| # | Input | Why it must be rejected | Actual | Result |
|---|---|---|---|---|
| 9 | `1.2.3` | only 3 octets | 0 | PASS |
| 10 | `1.2.3.4.5` | 5 octets | 0 | PASS |
| 11 | `1..2.3.4` | empty octet | 0 | PASS |
| 12 | `.1.2.3.4` | stray period before the address | 0 | PASS |
| 13 | `1.2.3.4.` | stray period after the address | 0 | PASS |
| 14 | `server at 10.0.0.1.` | sentence-ending period touches the address | 0 | PASS |
| 15 | `:1.2.3.4` | stray colon before the address | 0 | PASS |
| 16 | `1:2.3.4.5` | valid-looking `2.3.4.5` inside a longer malformed token (no partial matches) | 0 | PASS |
| 17 | `1.2.3.4:` | colon with no port | 0 | PASS |
| 18 | `1.2.3.4::80` | second colon | 0 | PASS |
| 19 | `1.2.3.4:80:` | trailing colon after the port | 0 | PASS |
| 20 | `1.2.3:4.5` | colon not immediately after the 4th octet | 0 | PASS |
| 21 | `no address here` | no candidate token | 0 | PASS |
| 22 | *(empty line)* | nothing to extract | 0 | PASS |

## 4. Numeric limits and leading zeros

| # | Input | Expected | Actual | Result |
|---|---|---|---|---|
| 23 | `0.0.0.0` | 1 0.0.0.0 (0) | 1 0.0.0.0 (0), port none | PASS |
| 24 | `255.255.255.255` | 1 max address (4294967295) | 1 255.255.255.255 (4294967295), port none | PASS |
| 25 | `255.255.255.255:65535` | 1, max port 65535 | 1 255.255.255.255, port 65535 | PASS |
| 26 | `1.2.3.4:0` | 1, port 0 (a single `0` is allowed) | 1 1.2.3.4, port 0 | PASS |
| 27 | `256.1.1.1` | 0: octet > 255 | 0 | PASS |
| 28 | `1.2.3.300` | 0: octet > 255 | 0 | PASS |
| 29 | `1.2.3.1000` | 0: 4-digit octet | 0 | PASS |
| 30 | `1.2.3.99999999999999999999` | 0: huge octet (overflow check) | 0 | PASS |
| 31 | `1.2.3.4:65536` | 0: port > 65535 | 0 | PASS |
| 32 | `1.2.3.4:123456` | 0: 6-digit port | 0 | PASS |
| 33 | `1.2.3.4:4294967376` | 0: port that would wrap around to 80 in 32-bit math | 0 | PASS |
| 34 | `01.2.3.4` | 0: leading zero in an octet | 0 | PASS |
| 35 | `1.2.3.00` | 0: `00` octet | 0 | PASS |
| 36 | `1.2.3.4:080` | 0: leading zero in the port | 0 | PASS |
| 37 | `1.2.3.4:00` | 0: `00` port | 0 | PASS |

**Result: 37 / 37 pass.**

## 5. Interactive program (output format and END)

Build and run: `gcc extract_ipv4.c -o ipv4.exe`, then `.\ipv4.exe`

```
Enter a line of text (or END to quit): router 192.168.0.10 up
Extracted IPv4 address: 192.168.0.10 (decimal value: 3232235530, port: none)
Enter a line of text (or END to quit): fw [10.0.0.1:443] allow
Extracted IPv4 address: 10.0.0.1 (decimal value: 167772161, port: 443)
Enter a line of text (or END to quit): 1:2.3.4.5
No valid IPv4 address found in the input.
Enter a line of text (or END to quit): end
No valid IPv4 address found in the input.
Enter a line of text (or END to quit): END
Program terminated.
```

Lowercase `end` does not quit, because END is case-sensitive. The output format matches the spec exactly.

## 6. The AI's built-in tests

The Version 2 code also includes the AI's own 26-case self-checking test runner. Build it with `gcc -DTEST_EXTRACT extract_ipv4.c -o test.exe`, then run `.\test.exe`. Result: 26/26 passed (full output in `version2/analysis.md`).
