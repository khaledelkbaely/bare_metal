#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <ctype.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define internal static
#define global static

#define MAX(x,y) ((x) >= (y) ? (x) : (y))

internal void *xmalloc(size_t sz) {
	void *ptr = malloc(sz);
	if (ptr == NULL) {
		perror("malloc failed");
		exit(1);
	}
	return ptr;
}
internal void *xrealloc(void *ptr, size_t sz) {
	ptr = realloc(ptr, sz);
	if (ptr == NULL) {
		perror("realloc failed");
		exit(1);
	}
	return ptr;
}
internal void fatal(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	printf("Fatal: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	exit(1);
}

typedef struct {
	size_t len;
	size_t cap;
	char buf[0];
} BufHdr;

// #define offsetof(t,f) ((size_t)&(((t*)0)->f))
#define buf__hdr(b) ((BufHdr *)((char *)b - offsetof(BufHdr, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf__fit(b,n) (buf_len(b) + (n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), buf_len(b) + (n), sizeof(*(b)))))
#define buf_push(b,x) (buf__fit(b,1), (b)[buf_len(b)] = (x), buf__hdr(b)->len++)
#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)


internal void *buf__grow(void *ptr, size_t new_len, size_t elem_size) {
	size_t new_cap = MAX(1 + 2 * buf_cap(ptr), new_len);
	assert(new_len <= new_cap);
	size_t old_len = buf_len(ptr);
	BufHdr *new_hdr;
	if (ptr != NULL) {
		 new_hdr = xrealloc(buf__hdr(ptr), offsetof(BufHdr, buf) + new_cap * elem_size);
	} else {
		 new_hdr = xmalloc(offsetof(BufHdr, buf) + new_cap * elem_size);
	}
	new_hdr->len = old_len;
	new_hdr->cap = new_cap;
	return (void *)new_hdr->buf;
}
typedef struct InternStr {
	size_t len;
	char *str;
} InternStr;
global InternStr *interns;
internal const char *string_intern_range(const char *start, const char *end) {
	size_t len = end - start;
	for (i32 i = 0; i < buf_len(interns); i++) {
		if (interns[i].len == len && strncmp(interns[i].str, start, len) == 0) {
			return interns[i].str;
		}
	}
	char *new_intern = xmalloc((len + 1) * sizeof(char));
	memcpy(new_intern, start, len);
	new_intern[len] = '\0';
	InternStr x = (InternStr){len, new_intern};
	buf_push(interns, x);
	return new_intern;
}
internal const char *string_intern(const char *str) {
	return string_intern_range(str, str + strlen(str));
}
internal void test_string_intern() {
	char x[] = "hello";
	char y[] = "hello";
	assert(x != y);
	assert(string_intern(x) == string_intern(y));
	char z[] = "hello!";
	assert(string_intern(z) != string_intern(x));
}

typedef enum TokenKind {
	TOKEN_INT = 128,
	TOKEN_NAME,
} TokenKind;
typedef struct Token {
	TokenKind kind;
	const char *start;
	const char *end;
	union {
		i64 val;
		const char *name;
	};
} Token;
global Token token;
global char *stream;
internal const char *token_kind_name(TokenKind kind) {
	static char buf[256];
	switch(kind) {
		case TOKEN_NAME:
			sprintf(buf, "name");
			break;
		case TOKEN_INT:
			sprintf(buf, "integer");
			break;
		default:
			if (kind < 128 && isprint(kind)) {
				sprintf(buf, "%c", kind);
			} else {
				sprintf(buf, "<ASCII %d>", kind);
			}
			break;
	}
	return buf;
}
internal void next_token() {
	token.start = stream;
	switch (*stream) {
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
			i64 val = 0;
			while(isdigit(*stream)) {
				val *= 10;
				val += *stream - '0';
				stream++;
			}
			token.kind = TOKEN_INT;
			token.val = val;
			break;
		}
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k':
		case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v':
		case 'w': case 'x': case 'y': case 'z': case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
		case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z': case '_': { token.kind = TOKEN_NAME; while (isalnum(*stream) || *stream == '_') { stream++;
			}
			token.name = string_intern_range(token.start, stream);
			break;
		}
		default: {
			 token.kind = *stream++;
			 break;
		}
	}
	token.end = stream;
}
internal inline bool is_token(TokenKind kind) {
	return token.kind == kind;
}
internal inline bool is_token_name(const char *name) {
	return token.kind == TOKEN_NAME && token.name == name;
}
internal inline bool match_token(TokenKind kind) {
	if (is_token(kind)) {
		next_token();
		return true;
	}
	return false;
}
internal inline bool expect_token(TokenKind kind) {
	if (is_token(kind)) {
		next_token();
		return true;
	}
	fatal("expected token %s, got %s\n", token_kind_name(kind), token_kind_name(token.kind));
	return false;
}

internal void print_token(Token token) {
	switch (token.kind) {
		case TOKEN_NAME:
			printf("TOKEN NAME: %.*s\n", token.end - token.start, token.start);
			break;
		case TOKEN_INT:
			printf("TOKEN INT: %d\n", token.val);
			break;
		default:
			printf("TOKEN '%c'\n", token.kind);
			break;
	}
}
internal void test_lex() {
	char *source = "*XY(XY)123asd";
	stream = source;
	next_token();
	while (token.kind) {
		print_token(token);
		next_token();
	}
}
typedef struct sexpr {
	char *str;
	size_t len;
} sexpr;
sexpr parse_expr();
sexpr parse_primary() {
	if (is_token(TOKEN_INT)) {
		sexpr result;
		char *number_buf = malloc(64 * sizeof(char));
		result.str = number_buf;
		char *curr = number_buf;
		i64 val = token.val;
		do {
			*curr++ = (val % 10) + '0';
			val /= 10;
		} while (val > 0);
		result.len = (size_t)(curr - number_buf);
		*curr-- = 0;
		char *left = number_buf;
		char *right = curr;
		while (left < right) {
			char temp = *right;
			*right-- = *left;
			*left++ = temp;
		}
		next_token();
		return result;
	} else if (match_token('(')) {
		sexpr e = parse_expr();
		e.len += 7;
		char *buf = malloc(e.len * sizeof(char));
		sprintf(buf, "(group %s)", e.str);
		free(e.str);
		e.str = buf;
		expect_token(')');
		return e;
	} else {
		// error
	}
	return (sexpr){"", 0};
}
sexpr parse_unary() {
	if (match_token('-')) {
		sexpr e = parse_unary();
		e.len = e.len + 5;
		char *buf = malloc(e.len * sizeof(char));
		sprintf(buf, "(- %s)", e.str);
		free(e.str);
		e.str = buf;
		return e;
	} else if (match_token('~')) {
		sexpr e = parse_unary();
		e.len = e.len + 5;
		char *buf = malloc(e.len * sizeof(char));
		sprintf(buf, "(~ %s)", e.str);
		free(e.str);
		e.str = buf;
		return e;
	} else {
		return parse_primary();
	}
}

sexpr parse_factor() {
	sexpr left = parse_unary();
	while (is_token('*') || is_token('/')) {
		char op = token.kind;
		next_token();

		sexpr right = parse_unary();

		left.len += 6 + right.len;
		char *buf = malloc(left.len * sizeof(char));
		sprintf(buf, "(%c %s %s)", op, left.str, right.str);
		free(left.str);
		free(right.str);
		left.str = buf;
	}
	return left;
}

sexpr parse_term() {
	sexpr left = parse_factor();
	while (is_token('+') || is_token('-')) {
		char op = token.kind;
		next_token();

		sexpr right = parse_factor();

		left.len += 6 + right.len;
		char *buf = malloc(left.len * sizeof(char));
		sprintf(buf, "(%c %s %s)", op, left.str, right.str);
		free(left.str);
		free(right.str);
		left.str = buf;
	}
	return left;
}
sexpr parse_expr() {
	return parse_term();
}

void test_parse() {
#define test(s) stream = s; next_token();
	test("0");
	printf("%s\n", parse_expr());
	test("1");
	printf("%s\n", parse_expr());
	test("-1");
	printf("%s\n", parse_expr());
	test("~1");
	printf("%s\n", parse_expr());
	test("123");
	printf("%s\n", parse_expr());
	test("-123");
	printf("%s\n", parse_expr());
	test("~123");
	printf("%s\n", parse_expr());
	test("1*2");
	printf("%s\n", parse_expr());
	test("4/2");
	printf("%s\n", parse_expr());
	test("1*2*3");
	printf("%s\n", parse_expr());
	test("1/2/3");
	printf("%s\n", parse_expr());
	test("1*2/3");
	printf("%s\n", parse_expr());
	test("1+2");
	printf("%s\n", parse_expr());
	test("4-2");
	printf("%s\n", parse_expr());
	test("1+2+3");
	printf("%s\n", parse_expr());
	test("1-2-3");
	printf("%s\n", parse_expr());
	test("1+2-3");
	printf("%s\n", parse_expr());
	test("1*2-3");
	printf("%s\n", parse_expr());
	test("1*2/3");
	printf("%s\n", parse_expr());
	test("1*2+3*4");
	printf("%s\n", parse_expr());
	test("1*2-3*4");
	printf("%s\n", parse_expr());
	test("1*2*3-4*5/2");
	printf("%s\n", parse_expr());
	test("1*(2-3)*4");
	printf("%s\n", parse_expr());
#undef test
}
int main() {
	test_string_intern();
	test_lex();
	test_parse();
}
