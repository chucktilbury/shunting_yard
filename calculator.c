/*
 * Shunting Yard algorithm.
 *
 * Convert an infix expression to a postfix expression and then solve it.
 * Includes unary operators, comparison operators, and exponent.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

/*
 * Global configuration flags.
 */
bool rpn_flag = false;
bool solve_flag = true;
bool verbo_flag = false;

typedef enum {
    // housekeeping tokens
    T_END_BUF,
    T_ERROR,
    // operators
    T_PLUS,     // '+'
    T_MINUS,    // '-'
    T_STAR,     // '*'
    T_SLASH,    // '/'
    T_PERC,     // '%'
    T_CARAT,    // '^'
    T_LT,       // '<'
    T_GT,       // '>'
    T_LTE,      // "<="
    T_GTE,      // ">="
    T_EQU,      // "=="
    T_NEQU,     // "!="
    T_EQUAL,    // '='
    T_OPAREN,   // '('
    T_CPAREN,   // ')'
    T_NOT,      // "not"
    T_AND,      // "and"
    T_OR,       // "or"
    // constructed tokens
    T_NUM,      // [0-9]+
    T_SYM,      // [a-zA-Z_]+
} TokType;

typedef enum {
    V_NUM,
    V_OP,
    V_SYM,
} ValType;

// Token
typedef struct {
    TokType type;
    const char* str;
} Token;

/*
 * Store the values. When this is treated like a stack, the values are
 * pushed and popped at the head. When this is treated like a queue, then
 * the values are added at the end and read from the start.
 */
typedef struct _value_ {
    ValType vtype;
    TokType ttype;
    double val;
    const char* name;
    struct _value_* next;
} Value;

/*
 * Simple queue of values can be read and written without destroying it or
 * push and pop it like a stack.
 */
typedef struct {
    Value* head;
    Value* tail;
    Value* crnt;
} ValueRepo;

// The current token.
Token tok = {-1, NULL};

typedef struct {
    char* buf;
    int cap;
    int len;
    int idx;
} InputBuffer;

InputBuffer* buffer;

/*
 * Create a value to store.
 */
Value* create_value(ValType vtype, TokType ttype, const char* name, int v) {

    Value* val = malloc(sizeof(Value));
    val->vtype = vtype;
    val->ttype = ttype;
    val->name = strdup(name);
    val->val = v;

    return val;
}

/*
 * Create a value repository.
 */
ValueRepo* create_repo() {

    ValueRepo* vr = malloc(sizeof(ValueRepo));
    vr->head = NULL;
    vr->crnt = NULL;
    vr->tail = NULL;

    return vr;
}

/*
 * Push a value to the head of the value repo.
 */
void push(ValueRepo* ptr, Value* val) {

    if(ptr != NULL) {
        val->next = ptr->head;
        if(val != NULL)
            ptr->head = val;
    }
}

/*
 * Pop a value from the head of the value repo. Returns the value that was
 * popped.
 */
Value* pop(ValueRepo* ptr) {

    if(ptr != NULL) {
        Value* tmp = ptr->head;
        if(ptr->head != NULL) {
            ptr->head = ptr->head->next;
            return tmp;
        }
    }

    return NULL;
}

/*
 * Peek at the value at the head of the repo.
 */
Value* peek(ValueRepo* ptr) {

    if(ptr != NULL)
        return ptr->head;

    return NULL;
}

/*
 * Add a value to the end of the value repository.
 */
void append(ValueRepo* ptr, Value* val) {

    if(ptr != NULL) {
        if(ptr->tail != NULL) {
            if(val != NULL)
                ptr->tail->next = val;
        }
        ptr->tail = val;
    }
}

/*
 * Reset the current pointer in the repo to be the head.
 */
Value* reset(ValueRepo* ptr) {

    if(ptr != NULL) {
        ptr->crnt = ptr->head;
        return ptr->crnt;
    }

    return NULL;
}

/*
 * Get the value at the current pointer and advance the pointer one notch.
 */
Value* get(ValueRepo* ptr) {

    if(ptr != NULL) {
        if(ptr->crnt != NULL) {
            Value* tmp = ptr->crnt;
            ptr->crnt = ptr->crnt->next;
            return tmp;
        }
    }

    return NULL;
}

/*
 * Convert a token value to a string for error messages and such.
 */
const char* tokToStr(TokType tok) {

    return (tok == T_END_BUF)? "END_BUF" :
        (tok == T_ERROR)? "ERROR" :
        (tok == T_PLUS)? "PLUS" :
        (tok == T_MINUS)? "MINUS" :
        (tok == T_STAR)? "STAR" :
        (tok == T_SLASH)? "SLASH" :
        (tok == T_PERC)? "PERC" :
        (tok == T_CARAT)? "CARAT" :
        (tok == T_LT)? "LT" :
        (tok == T_GT)? "GT" :
        (tok == T_LTE)? "LTE" :
        (tok == T_GTE)? "GTE" :
        (tok == T_EQU)? "EQU" :
        (tok == T_OPAREN)? "OPAREN" :
        (tok == T_CPAREN)? "CPAREN" :
        (tok == T_NEQU)? "NEQU" :
        (tok == T_EQUAL)? "EQUAL" :
        (tok == T_NOT)? "NOT" :
        (tok == T_AND)? "AND" :
        (tok == T_OR)? "OR" :
        (tok == T_NUM)? "NUM" :
        (tok == T_SYM)? "SYM" : "UNKNOWN";
}

/*
 * Create the input buffer.
 */
void create_buf() {

    buffer = malloc(sizeof(InputBuffer));
    buffer->cap = 1 << 3;
    buffer->idx = 0;
    buffer->len = 0;
    buffer->buf = malloc(buffer->cap);
}

/*
 * Reset the buffer length to zero, but do not free the memory.
 */
void reset_buf() {

    buffer->buf[0] = 0;
    buffer->idx = 0;
    buffer->len = 0;
}

/*
 * Consume a single character from the buffer and return it.
 */
int consume_char() {

    if(buffer->idx < buffer->len) {
        int ch = buffer->buf[buffer->idx];
        buffer->idx++;
        return ch;
    }

    return -1;  // end of buffer
}

/*
 * Get the current character from the buffer and return it.
 */
int read_char() {

    if(buffer->idx < buffer->len)
        return buffer->buf[buffer->idx];

    return -1;
}


/*
 * Add some string to the input buffer.
 */
void load_buf(const char* str) {

    int len = strlen(str);
    if(buffer->len+len+1 > buffer->cap) {
        while(buffer->len+len+1 > buffer->cap)
            buffer->cap <<= 1;
        buffer->buf = realloc(buffer->buf, buffer->cap);
    }

    memcpy(&buffer->buf[buffer->len], str, len);
    buffer->len += len;
}

/*
 * Read a symbol from the input.
 */
void read_symbol(char *buf) {

    int idx = 0;
    int ch = read_char();

    while(isalpha(ch) || ch == '_') {
        buf[idx++] = ch;
        consume_char();
        ch = read_char();
        if(0 > ch)
            break;
    }
}

/*
 * Read a number from the input.
 */
void read_number(char* buf) {

    int idx = 0;
    int ch = read_char();

    while(isdigit(ch) || ch == '.') {
        buf[idx++] = ch;
        consume_char();
        ch = read_char();
        if(0 > ch)
            break; // end of line
    }
}

/*
 * Convert a string to a double float.
 */
double str_to_num(const char* buf) {

    char* tmp;
    double num = strtod(buf, &tmp);

    if(*tmp != 0) {
        fprintf(stderr, "invalid floating point number: \"%s\"", buf);
        return 0;
    }

    return num;
}

/*
 * Read a single token from the input stream.
 */
void consume_token() {

    char buf[1024];
    int bidx = 0;
    int ch;
    bool finished = false;
    int ttype = T_END_BUF;

    memset(buf, 0, sizeof(buf));

    while(!finished) {
        ch = read_char();
        switch(ch) {
            case ' ':
            case '\t':
                consume_char();
                break;  // do nothing
            case '+':
                buf[bidx++] = ch;
                ttype = T_PLUS;
                finished = true;
                consume_char();
                break;
            case '-':
                buf[bidx++] = ch;
                ttype = T_MINUS;
                finished = true;
                consume_char();
                break;
            case '*':
                buf[bidx++] = ch;
                ttype = T_STAR;
                finished = true;
                consume_char();
                break;
            case '/':
                buf[bidx++] = ch;
                ttype = T_SLASH;
                finished = true;
                consume_char();
                break;
            case '%':
                buf[bidx++] = ch;
                ttype = T_PERC;
                finished = true;
                consume_char(buffer);
                break;
            case '^':
                buf[bidx++] = ch;
                ttype = T_CARAT;
                finished = true;
                consume_char();
                break;
            case '(':
                buf[bidx++] = ch;
                ttype = T_OPAREN;
                finished = true;
                consume_char();
                break;
            case ')':
                buf[bidx++] = ch;
                ttype = T_CPAREN;
                finished = true;
                consume_char();
                break;
            case '<':
                buf[bidx++] = ch;
                consume_char();
                ch = read_char();
                if(ch == '=') {
                    buf[bidx++] = ch;
                    consume_char();
                    ttype = T_LTE;
                }
                else
                    ttype = T_LT;
                finished = true;
                break;
            case '>':
                buf[bidx++] = ch;
                consume_char();
                ch = read_char();
                if(ch == '=') {
                    buf[bidx++] = ch;
                    consume_char();
                    ttype = T_GTE;
                }
                else
                    ttype = T_GT;

                finished = true;
                break;
            case '=':
                buf[bidx++] = ch;
                consume_char();
                ch = read_char();
                if(ch == '=') {
                    buf[bidx++] = ch;
                    consume_char();
                    ttype = T_EQU;
                }
                else
                    ttype = T_EQUAL;

                finished = true;
                break;
            case '!':
                buf[bidx++] = ch;
                consume_char();
                ch = read_char();
                if(ch == '=') {
                    buf[bidx++] = ch;
                    consume_char();
                    ttype = T_NEQU;
                }
                else
                    ttype = T_NOT;

                finished = true;
                break;
            default:
                // it's the end, a number or a symbol or unknown.
                if(ch == -1) {
                    if(bidx == 0)
                        ttype = T_END_BUF;
                    finished = true;
                }
                else {
                    if(isalpha(ch) || ch == '_') {
                        read_symbol(buf);
                        ttype = T_SYM;
                        finished = true;
                    }
                    else if(isdigit(ch)) {
                        read_number(buf);
                        finished = true;
                        ttype = T_NUM;
                    }
                    else {
                        consume_char();
                        fprintf(stderr, "Unhandled character ignored: '%c' (0x%02X)\n", ch, ch);
                        finished = true;
                        ttype = T_ERROR;
                    }
                }
        }

    }

    tok.type = ttype;
    if(tok.str != NULL)
        free((void*)tok.str);
    tok.str = strdup(buf);
}

/*
 * Return the precidence of the operator.
 */
int precedence(TokType op) {

    return
        (op == T_PLUS)? 5:  // '+'
        (op == T_MINUS)? 5: // '-'
        (op == T_STAR)? 6:  // '*'
        (op == T_SLASH)? 6: // '/'
        (op == T_PERC)? 6:  // '%'
        (op == T_CARAT)? 8: // '^'
        (op == T_LT)? 4:    // '<'
        (op == T_GT)? 4:    // '>'
        (op == T_LTE)? 4:   // "<="
        (op == T_GTE)? 4:   // ">="
        (op == T_EQU)? 3:   // "=="
        (op == T_NEQU)? 3:  // "!="
        (op == T_EQUAL)? 0: // '='
        (op == T_OPAREN)? 0: // '('
        (op == T_CPAREN)? 0: // ')'

        (op == T_NOT)? 7:   // "not" also unary '-'
        (op == T_AND)? 2:   // "and"
        (op == T_OR)? 1:    // "or"
        (op == T_NUM)? 10:  // [0-9]+
        (op == T_SYM)? 10:  // [a-zA-Z_]+
                       -1;  // unknown
}

void print_token() {

    printf("%s\t\"%s\"\n", tokToStr(tok.type), tok.str);
}

/*
 * Convert the input token stream to a postfix expression.
 */
ValueRepo* convert() {

    bool finished = false;
    ValueRepo* repo = create_repo();

    while(!finished) {
        consume_token();
        if(verbo_flag)
            print_token();
        if(tok.type == T_END_BUF)
            finished = true;
        else {
            switch() {

            }
            Value* val = create_value();
            append(repo, val);
        }
    }
}

/*
 * Solve the expression and print the result.
 */
int solve(ValueRepo* expr) {
}

/*
 * Print a variable.
 */
void print_var(Value* val) {

    switch(val->vtype) {
        case V_SYM:
            printf("SYMBOL: \"%s\" %0.3f\n", val->name, val->val);
            break;
        case V_NUM:
            printf("NUMBER: %03f\n", val->val);
            break;
        case V_OP:
            printf("  OPER: %s\n", tokToStr(val->ttype));
            break;
        default:
            fprintf(stderr, "Invalid var type: %d", val->vtype);
            exit(1);
    }
}

/*
 * Show all of the values in the value repo.
 */
void show_vars(ValueRepo* repo) {

    printf("\nAll variables:\n");
    if(repo != NULL) {
        Value* val = repo->head;
        while(val != NULL) {
            print_var(val);
            val = val->next;
        }
    }
    else
        printf("\tlist is empty\n");
    fputc('\n', stdout);
}

/*
 * Destroy the whole value repo.
 */
void free_repo(ValueRepo* repo) {

}

/*
 * Parse the second word in the string and return a pointer to it.
 */
const char* parse_var(const char* line) {

}

/*
 * Look up a variable by name and return its value.
 */
double get_var(const char* name) {

}

/*
 * Show the help text.
 */
void show_help() {

    printf("Infix to RPN calculator\n");
    printf("\t?|.h|.help  - this text\n");
    printf("\t.v|.verbo - verbose mode toggle\n");
    printf("\t.r|.rpn   - show the rpn string\n");
    printf("\t.s|.solve - toggle the solver flag\n");
    printf("\t.a|.vars  - show the vars table\n");
    printf("\t.p|.print var - show the value of a variable\n\n");
    printf("example:\n");
    printf("var1 = 12\n");
    printf("var2 = 2\n");
    printf("var3 = 7\n");
    printf("var4 = (var3 + var1) * var2\n");
    printf(".p var4\n");
    printf("var4 = 38\n");
}

/*
 * Main entry.
 */
int main() {

    char* line = NULL;
    bool finished = false;
    ValueRepo* values = NULL;
    ValueRepo* expr = NULL;

    create_buf();

    while(!finished) {

        if(line != NULL) {
            free(line);
            line = NULL;
        }

        line = readline("enter an expression: ");
        if(line && *line) {
            if(line[0] == '?') {
                show_help();
                continue;
            }
            else if(!strcmp(line, "q")) {
                finished = true;
                printf("quit\n");
            }
            else if(line[0] == '.' || line[0] == '/') {
                if(!strcmp(&line[1], "quit")) {
                    finished = true;
                    printf("quit\n");
                }
                else if(line[1] == 'h' || !strcmp(&line[1], "help"))
                    show_help();
                else if(line[1] == 'a' || !strcmp(&line[1], "vars"))
                    show_vars(values);
                else if(line[1] == 'r' || !strcmp(&line[1], "rpn")) {
                    rpn_flag = rpn_flag? false: true;
                    printf("rpn flag: %s\n", rpn_flag? "true": "false");
                }
                else if(line[1] == 's' || !strcmp(&line[1], "solve")) {
                    solve_flag = solve_flag? false: true;
                    printf("solve flag: %s\n", solve_flag? "true": "false");
                }
                else if(line[1] == 'v' || !strcmp(&line[1], "verbo")) {
                    verbo_flag = verbo_flag? false: true;
                    printf("verbose flag: %s\n", verbo_flag? "true": "false");
                }
                else if(line[1] == 'p' || !strcmp(&line[1], "print")) {
                    const char* vname = parse_var(line);
                    printf("%s = %0.3f\n", vname, get_var(vname));
                }
                else {
                    printf("unknown command: %s\n", line);
                    show_help();
                }
                continue;
            }
            add_history(line);
        }

        if(!finished) {
            reset_buf();
            load_buf(line);

            if(expr != NULL)
                free_expr(expr);

            expr = convert();
            if(solve_flag)
                solve(expr);
        }
    }

    return 0;
}
