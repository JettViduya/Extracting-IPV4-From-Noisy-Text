# AI Disclosure

**Student:** Jett Viduya
**Assignment:** Extracting an IPv4 address from noisy text (C)

## 1. Tools used

| Tool | Version | Used for | Dates |
|---|---|---|---|
| Claude (Anthropic), chat | Claude Opus 5.5 | Generating all of the code (`extract_ipv4.c`) | September 27–28, 2026 |
| Claude (Anthropic), separate chat | Claude Opus 5.5 | Reviewing the generated code with me, planning my follow-up prompt, and helping word my analysis documents | September 27–28, 2026 |

I kept these separate on purpose: one chat was the "code generator" I was evaluating, and the other was a second opinion while I reviewed its output.

## 2. Prompts

The exact prompts are copied word for word in each version folder:

- **Prompt 1** is in [`version1/analysis.md`](version1/analysis.md). I asked for the function with the required prototype and the library restrictions, but I did not list the strict validation rules.
- **Prompt 2** is in [`version2/analysis.md`](version2/analysis.md). I pointed out the two wrong results, stated the missing rules (whole-token matching, colon rule, port leading zeros), and asked for the required interactive `main`.

## 3. Code attribution

| File | Written by |
|---|---|
| `extract_ipv4.c` (top level, final submission) | AI-generated (Version 2 output, unchanged) |
| `version1/extract_ipv4.c` | AI-generated (Version 1 output, unchanged) |
| `version2/extract_ipv4.c` | AI-generated (Version 2 output, unchanged) |
| `version1/analysis.md`, `version2/analysis.md` | Me, with wording help from the second Claude chat. The AI outputs inside them are copied verbatim. |

**Modifications:** I made no manual edits to the code. Every fix came from re-prompting the AI after testing showed what was wrong. The only code I added was three test cases to the AI's `tests[]` list in a local copy for my own testing (see section 4). That copy is not the submitted file.

## 4. What went wrong and how I caught it

**Version 1** compiled cleanly, and the AI said it "runs clean on all 16 test cases." When I ran its demo myself, two of its own cases were wrong according to the spec:

- `server at 10.0.0.1.` returned 1, but a period touching the address should reject it.
- `addr=8.8.8.8:` returned 1, but a colon without a valid port should reject the whole match.

These weren't random bugs. The AI stated both as design decisions ("A plain period... is fine", "A colon with no digits after it just means no port was given"). Its test runner didn't catch them because it only printed results and never compared them to expected values. So "runs clean" only meant nothing crashed. Reading the code, I also found that the port had no leading-zero check, and that there was no interactive `main`, only a demo. The AI also suggested *loosening* rules (for example removing the leading-zero check), which would have moved the code further from the spec.

**Version 2:** after I gave it the exact rules, the AI rewrote the matching to split the line into whole tokens (maximal runs of digits, `.` and `:`) and accept a token only if the grammar consumes all of it. That fixed every boundary bug at once. It merged the octet and port parsing into one function, so the port got the leading-zero rule. It added the required input loop and output format, and its test runner now checks expected values. My run: 26/26 passed. I also added my own cases (`1:2.3.4.5`, `1.1.1.9999999999`, `12:30:45 host 9.9.9.9`) and tested the interactive program. Results are in [`TEST_CASES.md`](TEST_CASES.md) (37 cases covering malformed input, ambiguous input and numeric limits, all passing).

## 5. Verification statement

- I understand every line of the submitted code, including how `extractIPv4` splits the input into tokens, how `matchToken` requires the whole token to match, and how `parseNumber` enforces digit count, leading zeros and range without library conversion functions.
- I compiled and tested the code myself. The results are recorded in [`TEST_CASES.md`](TEST_CASES.md) and the `version1/` and `version2/` folders.
- **Known limitations** that I chose not to change:
  - The input buffer is 1024 bytes, so a line longer than 1022 characters is rejected with a message, even if it contains a valid address. The spec doesn't specify a line length.
  - If input ends (EOF) without `END`, the program still prints `Program terminated.`, but on the same line as the prompt. This is cosmetic.
  - When a line contains more than one valid address, the first one is returned.
