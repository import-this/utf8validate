#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>

// Last code point in range
#define BYTE1_LAST_CODE_POINT 0x007F
#define BYTE2_LAST_CODE_POINT 0x07FF
#define BYTE3_LAST_CODE_POINT 0xFFFF
#define BYTE4_LAST_CODE_POINT 0x10FFFF

// Prefix for header (leading) byte of n-byte encoding.
#define BYTE1_ENC_HEADER_PREFIX 0x0000          // 0xxxxxxx
#define BYTE2_ENC_HEADER_PREFIX 0x00C0          // 110xxxxx
#define BYTE3_ENC_HEADER_PREFIX 0x00E0          // 1110xxxx
#define BYTE4_ENC_HEADER_PREFIX 0x00F0          // 11110xxx

// Prefix for tail (continuation) bytes of encoding (same for all n-byte encodings).
#define ENC_TAIL_PREFIX 0x0080                  // 10xxxxxx

// Masks for getting prefixes in header (leading) bytes.
#define BYTE1_ENC_HEADER_PREFIX_MASK 0x0080     // 10000000
#define BYTE2_ENC_HEADER_PREFIX_MASK 0x00E0     // 11100000
#define BYTE3_ENC_HEADER_PREFIX_MASK 0x00F0     // 11110000
#define BYTE4_ENC_HEADER_PREFIX_MASK 0x00F8     // 11111000
// Masks for getting suffixes in header (leading) bytes.
#define BYTE2_ENC_HEADER_SUFFIX_MASK (~BYTE2_ENC_HEADER_PREFIX_MASK)
#define BYTE3_ENC_HEADER_SUFFIX_MASK (~BYTE3_ENC_HEADER_PREFIX_MASK)
#define BYTE4_ENC_HEADER_SUFFIX_MASK (~BYTE4_ENC_HEADER_PREFIX_MASK)

// Mask for getting the prefix in tail (continuation) bytes.
#define ENC_TAIL_PREFIX_MASK 0x00C0             // 11000000
// Mask for getting the suffix in tail (continuation) bytes.
#define ENC_TAIL_SUFFIX_MASK (~ENC_TAIL_PREFIX_MASK)

#define get_tail_bits(tail_byte) (tail_byte & ENC_TAIL_SUFFIX_MASK)


bool is_valid_tail_byte(const int tail_byte) {
    return ((tail_byte & ENC_TAIL_PREFIX_MASK) == ENC_TAIL_PREFIX);
}

/**
 * Checks if the specified UTF-8 2-byte character is overlong (oversized, i.e. non-minimal).
 * @param codepoint The UTF-8 character to check
 * @return true if the encoding is overlong
 */
bool is_overlong_encoding2(const int_fast32_t codepoint) {
    return codepoint <= BYTE1_LAST_CODE_POINT;
}
/**
 * Checks if the specified UTF-8 3-byte character is overlong (oversized, i.e. non-minimal).
 * @param codepoint The UTF-8 character to check
 * @return true if the encoding is overlong
 */
bool is_overlong_encoding3(const int_fast32_t codepoint) {
    return codepoint <= BYTE2_LAST_CODE_POINT;
}
/**
 * Checks if the specified UTF-8 4-byte character is overlong (oversized, i.e. non-minimal).
 * @param codepoint The UTF-8 character to check
 * @return true if the encoding is overlong
 */
bool is_overlong_encoding4(const int_fast32_t codepoint) {
    return codepoint <= BYTE3_LAST_CODE_POINT;
}

int main() {
    unsigned long long one_byte_chars = 0, two_or_more_byte_chars = 0;
    int_fast32_t codepoint;     // The final character
    int byte;

    // https://stackoverflow.com/a/34240796/1751037
    while ((byte = getchar()) != EOF) {
        const int header_byte = byte;
        if ((header_byte & BYTE1_ENC_HEADER_PREFIX_MASK) == BYTE1_ENC_HEADER_PREFIX) {              // 1-byte UTF-8 characters
            assert(0x0000 <= header_byte && header_byte <= 0x007F);
            ++one_byte_chars;
        } else {
            if ((header_byte & BYTE2_ENC_HEADER_PREFIX_MASK) == BYTE2_ENC_HEADER_PREFIX) {          // 2-byte UTF-8 characters
                codepoint = (header_byte & BYTE2_ENC_HEADER_SUFFIX_MASK);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                if (is_overlong_encoding2(codepoint)) goto ERROR_OVERLONG_ENCODING;
            } else if ((header_byte & BYTE3_ENC_HEADER_PREFIX_MASK) == BYTE3_ENC_HEADER_PREFIX) {   // 3-byte UTF-8 characters
                codepoint = (header_byte & BYTE3_ENC_HEADER_SUFFIX_MASK);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                // UTF-16 surrogate halves
                if (codepoint >= 0xD800 && codepoint <= 0xDFFF) goto ERROR_INVALID_CODE_POINT;
                if (is_overlong_encoding3(codepoint)) goto ERROR_OVERLONG_ENCODING;
            } else if ((header_byte & BYTE4_ENC_HEADER_PREFIX_MASK) == BYTE4_ENC_HEADER_PREFIX) {   // 4-byte UTF-8 characters
                codepoint = (header_byte & BYTE4_ENC_HEADER_SUFFIX_MASK);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                if ((byte = getchar()) == EOF) goto ERROR_INVALID_CODE_POINT;
                if (!is_valid_tail_byte(byte)) goto ERROR_INVALID_TAIL_BYTE;
                codepoint = (codepoint << 6) | get_tail_bits(byte);

                if (codepoint > BYTE4_LAST_CODE_POINT) goto ERROR_INVALID_CODE_POINT;
                if (is_overlong_encoding4(codepoint)) goto ERROR_OVERLONG_ENCODING;
            } else {
                goto ERROR_INVALID_HEADER_BYTE;
            }
            ++two_or_more_byte_chars;
        }
    }
    printf("Found %llu ASCII and %llu multi-byte UTF-8 characters.\n", one_byte_chars, two_or_more_byte_chars);
    return 0;

ERROR_INVALID_HEADER_BYTE:
    fprintf(stderr, "Invalid UTF-8 header byte: 0x%02X\n", byte);
    return 1;
ERROR_INVALID_TAIL_BYTE:
    fprintf(stderr, "Invalid UTF-8 tail byte: 0x%02X\n", byte);
    return 2;
ERROR_INVALID_CODE_POINT:
    fprintf(stderr, "Invalid UTF-8 code point: U+%04"PRIXFAST32"\n", codepoint);
    return 3;
ERROR_OVERLONG_ENCODING:
    fprintf(stderr, "Overlong UTF-8 code point: U+%04"PRIXFAST32"\n", codepoint);
    return 4;
}
