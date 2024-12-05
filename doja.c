/// @author Michael Burgess <dou@michaelburgess.co.uk>

/// This file is here to preview some initial work developing 
/// a second-order probabilistic programming langauge 

/// This is not the development version (which is local, and not released)
///... but a quick snapshot of some progress to provide some insight into the code. 

/// This may not compile, as it's in the middle of rewrities. 

/*

EXAMPLE:

const xs = [0, 1, 2, 3]
const ys = [0, 1, 4, 6]

struct Person
    /// @brief This is struct definition
    /// @note This will be remembered on the scope
    .name : String 

trait Stringy
    fn to_string()

impl Person : Stringy
    fn to_string() = name 

fn main() :=
    demo()
    log(help(#Person)) // returns all @'d comments
    log({city: "London"})
    log(Person {.name = "Michael"})
    log(for( x <- range(1, 3) ) yield 10)
    log(infer(model(), #MCMC).take(3))

const fn demo() :=
    loop 
        i <- range(3)
    in 
        match i 
            if 1 => log("Yay")
            else => log("Nay")

loop fn model() :=
    let 
        f = fn(x, a, b) -> a * x + b 
        m = sample(normal(0, 2))
        c = sample(normal(0, 2))
        sigma = sample(gamma(1, 1))
    in
        log("x =", f(10, m, c))
        observe(normal(f(10, m, c), sigma));
        return [m, c]


fn moveSemantics() :=
    let 
        a = [1, 2, 3] 
        b = ref a 
        c = move a 
    in 
        log(a)
*/


/// -------------------- HEADERS --------------------
/// @ref cStdLibDependenciesIncludes
/// @ref ConfigConstantsHeader
/// @ref InternalStandardLibraryHeader
/// @ref ErrorHeader
/// @ref MemoryManagementHeader
/// @ref LexerHeader
/// @ref LexerApiHeader
/// @ref ParserAstHeader
/// @ref ParserApiHeader
/// @ref InterpreterDovHeader
/// @ref InterpreterDovApiHeader
/// @ref InterpreterStaticAllocations
/// @ref InterpreterApiHeader
/// @ref InterpreterNativeSupportHeader

#pragma region cStdLibDependenciesIncludes

#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#pragma endregion

#pragma region ConfigConstantsHeader

/// CONFIG
#define DOUBT_VERSION "Version: v0.1"
#define DOUBT_AUTHOR "Author: Michael J. Burgess (dou@michaelburgess.co.uk)"
#define DOUBT_LICENSE_SHORT                                                                                                          \
    "License (short): "                                                                                                              \
    "\n    * All non-commercial uses permitted if and only if author & copyright notice maintained."                                 \
    "\n    * Commercial use permitted if and only if included in novel product & author & copyright notice maintained"               \
    "\n        * A novel product is one which employs this software *as-is* to create a novel product"                               \
    "\n        * E.g., Use of this software as a host programming environment to develop new programs is permitted. "                \
    "\n        * Modifications or transformations to the source creates a *derivative* product, and so no commercial is permitted. " \
    "\n    * Verbatim resale not permitted. Omitting copyright notice & authorship not permitted in any circumstance."               \
    "\n    * No warranty provided. Use at your own risk."                                                                            \
    "\n    * These terms should be read as implying a broad commercial and non-commercial scope to freely use this software."        \
    "\n    * This is source-available, not open-source sofware: "                                                                    \
    "\n        * The author will not freely support or maintain this software.  "                                                    \
    "\n        * For paid support, please contact the author.  "

#define DOUBT_LICENCE_LONG " @todo Please contact the author"

#define MEM_LOCAL_SIZE (2 * 1024 * 1024)     // 2 Mb
#define MEM_GLOBAL_SIZE (4 * 1024 * 1024)    // 4 Mb
#define MEM_INTERNAL_SIZE (8 * 1024 * 1024)  // 8 Mb
#define MEM_HEAP_MIN_LEN 128
#define MEM_HEAP_MIN_SIZE (16 * 1024 * 1024)  // 16 Mb
/// @todo move all non-trivial arrays to MEM_INTERNAL so size is known via these consts

#define TRACER_MAX_DEPTH 256

#define LEX_MAX_TOKENS (1024 * 1024)  /// @todo make this dynamic
#define LEX_MAX_INDENTS 32
#define LEX_CAPTURE_COMMENTS false      /// @todo this
#define PARSER_MAX_NODES (1024 * 1024)  /// @todo make this dynamic
#define PARSER_MIN_NODE_CAPACITY 16

/// @brief Compiler/interpreter build varieties:
#define TOGGLE_INTERP_CHECKTYPES 1  /// @todo if on, no identifier should change its type
#define TOGGLE_INTERP_TRACER 1

/// @todo
/// @brief Turns the interpreter into a compiler
///         which traces execution of the intrepreted program
///         and emits C as a compilation target
#define TOGGLE_INTERP_COMPILER 0

#pragma endregion

#pragma region InternalStandardLibraryHeader

#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)

#define debugger                            \
    puts(error_withloc("Debugger: HERE!")); \
    abort()

typedef struct CliOption {
    const char *name;
    const char *value;
    const char or_else[32];
    bool is_set;
} CliOption;

void cli_parse_args(int argc, char **argv, CliOption *out_opts, size_t num_opts);
#define cli_parse_opts(opts) (cli_parse_args(argc, argv, opts, sizeof(opts) / sizeof(CliOption)))
#define cli_opt_flag(name) {name, NULL, "", false}
#define cli_opt_default(name, or_else) {name, NULL, or_else, false}
#define cli_opt_get(options, index) (options[index].is_set ? options[index].value : options[index].or_else)

typedef struct Str {
    const char *cstr;
    unsigned int len;
    void *(*allocator)(unsigned int size);
} Str;

#define S str_new

Str str_new(const char *cstr);
Str str_chr(const char chr);
Str str_concat(Str left, Str right);
Str str_eq(Str left, Str right);
Str str_eq_chr(Str left, char right);
Str str_copy(Str str);                                          // Creates a copy of the given string.
Str str_sub(Str str, unsigned int start, unsigned int length);  // Extracts a substring from a string.
int str_index_of(Str str, char chr);                            // Finds the first occurrence of a character in a string.
int str_last_index_of(Str str, char chr);                       // Finds the last occurrence of a character in a string.
Str str_replace(Str str, char target, char replacement);        // Replaces all occurrences of a character with another.
Str str_trim(Str str);                                          // Trims leading and trailing whitespace from a string.
Str str_upper(Str str);                                         // Converts all characters in the string to uppercase.
Str str_lower(Str str);                                         // Converts all characters in the string to lowercase.
int str_cmp(Str left, Str right);
int str_cmp_natural(Str left, Str right);
Str str_append_chr(Str str, char chr);                             // Appends a character to the end of the string.
Str str_append_cstr(Str str, const char *cstr);                    // Appends a C-string to the end of the string.
int str_contains(Str str, char chr);                               // Checks if a character exists in the string.
Str *str_split(Str str, char delimiter, unsigned int *out_count);  // Splits the string by a delimiter into an array of `Str`.
Str *str_lines(Str str, char delimiter, unsigned int *out_count);
void str_free(Str str);  // Frees the memory allocated for the string.

const char *cstr_maybe_dup(const char *s);
const char *cstr_dup(const char *s);
int cstr_eq(const char *s1, const char *s2);
int cstr_cmp_nat(const char *a, const char *b);
int cstr_len(const char *s);
void cstr_copy(char *dest, const char *src);
void cstr_ncopy(char *dest, const char *src, int len);

#pragma endregion

#pragma region ErrorHeader

/// @brief
typedef enum LogLevel {
    LOG_LEVEL_DEFAULT,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_ASSERT,
    LOG_LEVEL_LEN,
} LogLevel;

typedef enum DouError {
    ERROR_ZERO,
    ERROR_UNRECOVERABLE,
    ERROR_WARNING,
    ERROR_ADVICE,
    ERROR_DEPRECATED,
    ERROR_NUM_ERRORS,
} DouError;

static char *LogLevelStr[] = {
    [LOG_LEVEL_DEFAULT] = "DEFAULT",
    [LOG_LEVEL_DEBUG] = "DEBUG",
    [LOG_LEVEL_INFO] = "INFO",
    [LOG_LEVEL_WARNING] = "WARNING",
    [LOG_LEVEL_ERROR] = "ERROR",
    [LOG_LEVEL_ASSERT] = "ASSERT",
    [LOG_LEVEL_LEN] = "<LOG_LEVEL_ERROR>"};

typedef struct TraceEntry {
    const char *fn_name;
    const char *argument;
    const char *file;
    int line;
} TraceEntry;

typedef struct CallStack {
    TraceEntry entries[TRACER_MAX_DEPTH];
    int top;
} CallStack;

CallStack _glo_parser_traces = {.top = -1};
CallStack _glo_interp_traces = {.top = -1};

#define watchpoint(var, cond) ((cond) ? (abort(), var) : var)

/// @todo: presumably should be calling tracer_pop()

void tracer_pop(CallStack *traces);
void tracer_print_stack(CallStack *traces, int depth);
void tracer_push(CallStack *traces, const char *fn_name, const char *arg, const char *file, int line);

#ifndef NO_DEBUG
// #define error_print_location() (fprintf(stderr, "ERROR in %s @ %s:%d:\n\t", __func__, __FILE__, __LINE__))
#define error_withloc(message) \
    "ERROR @ " __FILE__ ":" _STR(__LINE__) ": " message
#else
#define error_add_location(message) (message)
#endif

#define parser_trace(arg) (tracer_push(&_glo_parser_traces, __func__, arg, __FILE__, __LINE__));
#define parser_tracer_print_stack() (tracer_print_stack(&_glo_parser_traces, 12));
#define parser_clear_trace() (_glo_parser_traces.top = -1)

#define interp_trace(arg) (tracer_push(&_glo_interp_traces, __func__, arg, __FILE__, __LINE__));
#define interp_tracer_print_stack() (tracer_print_stack(&_glo_interp_traces, 12));
#define interp_clear_trace() (_glo_interp_traces.top = -1)

#ifdef NO_DEBUGGING
#define LOG(level, format, ...) ((void)0)
#else
#define LOG(level, format, ...) error_log_message(level, __func__, format, __VA_ARGS__)
#endif

#ifdef NO_DEBUGGING
#define ASSERT(condition, message) ((void)0)
#else
#define ASSERT(condition, message)                                               \
    do {                                                                         \
        if (!(condition)) {                                                      \
            error_log_message(LOG_LEVEL_ASSERT, __func__, "(%s) (%s:%d). %s.\n", \
                              #condition, __FILE__, __LINE__, message);          \
            abort();                                                             \
        }                                                                        \
    } while (0)
#endif

//// ADDITIONS

#define LOG_WITH_CONTEXT(level, format, context, ...) \
    error_log_message(level, __func__, "%s | " format, context, __VA_ARGS__)

typedef void (*DebugHook)(const char *message, void *user_data);
DebugHook _glo_debug_hook = NULL;

void register_debug_hook(DebugHook hook) {
    _glo_debug_hook = hook;
}

#pragma endregion

#pragma region MemoryManagementHeader

typedef struct Arena {
    char *data;
    unsigned int capacity;
    unsigned int size;
    unsigned int num_live;
} Arena;

typedef enum GCError {
    GC_ZERO = 0,
    GC_SUCCESS,
    GC_ERR_HEAP_FULL,
    GC_ERR_INVALID_POINTER,
    GC_ERR_INVALID_TYPE,
    GC_NUM_TYPES
} GCError;

typedef enum HeapObjectType {
    HEAP_ZERO = 0,
    HEAP_ALIVE,     // Object is alive and accessible
    HEAP_DEAD,      // Object marked for deletion
    HEAP_PINNED,    // Object is pinned and cannot be moved
    HEAP_NUM_TYPES  // Number of heap object types
} HeapObjectType;

typedef struct HeapObject {
    HeapObjectType type;
    uint32_t size;            // Size of the object in bytes
    void *data;               // Pointer to the actual object
    bool marked;              // GC marking flag
    struct HeapObject *next;  // For chaining in the free list
} HeapObject;

typedef struct Heap {
    HeapObject *free_list;      // Linked list of free objects
    HeapObject **objects;       // Dynamic array of allocated objects
    uint32_t capacity;          // Total capacity of the heap
    uint32_t size;              // Current size of allocated objects
    uint32_t gc_threshold;      // Threshold for triggering garbage collection
    uint32_t generation_count;  // Track GC cycles
} Heap;

///@todo change to use global heap
Heap *dou_dyn_init(uint32_t initial_capacity, uint32_t gc_threshold);
GCError dou_dyn_destroy(Heap *heap);
void *dou_dyn_new(Heap *heap, uint32_t size);
GCError dou_dyn_free(Heap *heap, void *ptr);
void dou_dyn_print(const Heap *heap);
void dou_dyn_debug_dump(const Heap *heap);

void dou_dyn_gc_run(Heap *heap);
void dou_dyn_gc_mark(Heap *heap, HeapObject *root);
void dou_dyn_gc_sweep(Heap *heap);
void dou_dyn_gc_mark_and_sweep(Heap *heap);

// static HeapObject* allocate_heap_object(Heap *heap, uint32_t size);
static HeapObject *dou_dyn_find(Heap *heap, void *ptr);

Arena *_glo_arena_interp_current = NULL;

static void *_glo_unmanaged = NULL;
static Heap _glo_dyn_heap = {0};
static Arena _glo_arena_global = {0};
static Arena _glo_arena_local = {0};
static Arena _glo_arena_internal = {0};
static Arena _glo_compiler = {0};

#define arena_is_owner(arena, ptr) ((ptr > arena.data) && (ptr <= arena.data + arena.size))

void *arena_new(Arena *arena, unsigned int size);
void arena_init(Arena *arena, unsigned int capacity);

#define interp_allocator() (_glo_arena_interp_current)

#define interp_use_stack_arena() (_glo_arena_interp_current = &_glo_arena_local)
#define interp_use_heap_arena() (_glo_arena_interp_current = &_glo_arena_global)

#define interp_new(size) arena_new(_glo_arena_interp_current, size);
#define interp_free(ptr) (NULL)

#define dou_new(size) arena_new(&_glo_arena_internal, size);
#define dou_free(ptr) (NULL)

#pragma endregion

#pragma region LexerHeader

typedef enum TokenType {
    TT_INDENT = 0,
    TT_DEDENT,
    TT_END,
    TT_TAG,
    TT_IDENTIFIER,
    TT_TYPE,
    TT_ANNOTATION,
    TT_KEYWORD,
    TT_NUMBER,
    TT_BRACKET,
    TT_STRING,
    TT_ASSIGN,
    TT_OP,
    TT_SEP,
    TT_DISCARD,
    TT_IGNORE,
    TT_DEREF,
    TT_COMMENT
} TokenType;

const char *_glo_keywords[] = {
    "const", "fn", "for", "yield", "loop",
    "in", "if", "else", "done", "let",
    "mut*", "mut", "pub", "priv", "mod",
    "try", "return", "match", "mod",
    "where", "use", "as", "dyn",
    "trait", "struct", "macro"};

static const char *TokenTypeStr[] = {
    [TT_INDENT] = "INDENT",
    [TT_DEDENT] = "DEDENT",
    [TT_END] = "END",
    [TT_TAG] = "TAG",
    [TT_IDENTIFIER] = "IDENTIFIER",
    [TT_TYPE] = "TYPE",
    [TT_ANNOTATION] = "TYPE_ANNOTATION",
    [TT_KEYWORD] = "KEYWORD",
    [TT_NUMBER] = "NUMBER",
    [TT_BRACKET] = "BRACKET",
    [TT_STRING] = "STRING",
    [TT_ASSIGN] = "ASSIGN",
    [TT_OP] = "OP",
    [TT_SEP] = "SEP",
    [TT_DISCARD] = "DISCARD",
    [TT_IGNORE] = "IGNORE",
    [TT_DEREF] = "DEREF",
    [TT_COMMENT] = "COMMENT",
};

typedef struct Token {
    TokenType type;
    const char *value;
    int line;
    int col;
} Token;

int _glo_num_keywords = sizeof(_glo_keywords) / sizeof(_glo_keywords[0]);

Token _glo_tokens[LEX_MAX_TOKENS] = {0};
int _glo_num_tokens = 0;

int _glo_indents[LEX_MAX_INDENTS] = {0};
int _glo_num_indents = 0;

#pragma endregion

#pragma region LexerApiHeader

Token *lex_lines(const char **lines, int num_lines, const char *indent);
void lex_comment(const char *lex_line, int *pos, int trim_len, int line_no, int col);
void lex_print_tokens(Token *tokens, int start, int end, int marker);
void lex_dedent(int indentation, int line_no);
void lex_indent(int indentation, int line_no);
void lex_empty_line(int line_no);
void lex_word(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *supressNL);
void lex_number(const char *lex_line, int *pos, int trim_len, int line_no, int col);
void lex_bracket(const char *lex_line, int *pos, int line_no, int col);
void lex_string(const char *lex_line, int *pos, int trim_len, int line_no, int col);
void lex_assign(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *suppressNL);
void lex_op(const char *lex_line, int *pos, int trim_len, int line_no, int col);
void lex_separator(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *suppressNL);
void lex_newline(int line_no);
void lex_remaining_dedent(int num_lines);
bool lex_is_keyword(const char *s);

#pragma endregion

#pragma region ParserAstHeader

typedef struct Dov Dov;  // forward decl for Doubt-Value type Dov

typedef enum AstType {
    AST_ZERO,
    AST_DOV,
    AST_EXP,
    AST_BOP,
    AST_UOP,
    AST_ID,
    AST_NUM,
    AST_STR,
    AST_TAG,
    AST_CALL,
    AST_METHOD_CALL,
    AST_MEMBER_ACCESS,
    AST_BLOCK,
    AST_FN,
    AST_FN_MACRO,
    AST_FN_ANON,
    AST_FN_GEN,
    AST_STRUCT,
    AST_STRUCT_INIT,
    AST_TRAIT,
    AST_IF,
    AST_FOR,
    AST_LOOP,
    AST_LET,
    AST_BINDING,
    AST_RETURN,
    AST_MATCH,
    AST_DICT,
    AST_VEC,
    AST_USE,
    AST_MOD,
    AST_MUTATION,
    AST_CONST,
    AST_YIELD,
    AST_IGNORE,
    AST_NUM_TYPES,
} AstType;

const char *AstTypeStr[] = {
    [AST_ZERO] = "<AST_ERROR>",
    [AST_DOV] = "DOV(PARTIAL-EVAL)",
    [AST_EXP] = "EXP",
    [AST_BOP] = "BOP",
    [AST_UOP] = "UOP",
    [AST_ID] = "ID",
    [AST_NUM] = "NUM",
    [AST_STR] = "STR",
    [AST_TAG] = "TAG",
    [AST_CALL] = "CALL",
    [AST_METHOD_CALL] = "METHOD_CALL",
    [AST_MEMBER_ACCESS] = "MEMBER_ACCESS",
    [AST_BLOCK] = "BLOCK",
    [AST_FN] = "FN",
    [AST_FN_MACRO] = "FN_MACRO",
    [AST_FN_ANON] = "FN_ANON",
    [AST_FN_GEN] = "FN_GEN",
    [AST_STRUCT] = "STRUCT",
    [AST_STRUCT_INIT] = "STRUCT_INIT",
    [AST_TRAIT] = "TRAIT",
    [AST_IF] = "IF",
    [AST_FOR] = "FOR",
    [AST_LOOP] = "LOOP",
    [AST_LET] = "LET",
    [AST_BINDING] = "BINDING",
    [AST_RETURN] = "RETURN",
    [AST_MATCH] = "MATCH",
    [AST_DICT] = "DICT",
    [AST_VEC] = "VEC",
    [AST_USE] = "USE",
    [AST_MOD] = "MOD",
    [AST_MUTATION] = "MUTATION",
    [AST_CONST] = "CONST",
    [AST_YIELD] = "YIELD",
    [AST_IGNORE] = "IGNORE",
    [AST_NUM_TYPES] = "<AST_ERROR>"};

typedef struct AstNode AstNode;

typedef struct DovPtr {
    uint64_t payload : 61;  // Payload holds the pointer or value
    uint8_t type : 3;       // Type field (3 bits allow up to 8 types)
} DovPtr;

struct AstNode {
    AstType type;
    int line;
    int col;
    union {
        DovPtr dov;  /// @todo for compile-time partial eval of AST
        struct AstIdentifier {
            const char *name;
            const char *type;
        } id;
        struct AstDoubleValue {
            double value;
        } num;
        struct AstStringValue {
            const char *value;
        } str;
        struct {
            const char *name;
        } tag;
        struct AstBOP {
            const char *op;
            AstNode *left;
            AstNode *right;
        } bop;
        struct AstUOP {
            const char *op;
            AstNode *operand;
        } uop;
        struct AstFnCall {
            AstNode *callee;  // Changed from const char* to AstNode*
            AstNode **args;
            int num_args;
        } call;
        struct AstMethodCall {
            AstNode *target;
            const char *method;
            AstNode **args;
            int num_args;
        } method_call;
        struct AstMemberAccess {
            AstNode *target;
            const char *property;
        } member_access;
        struct AstBlock {
            AstNode **statements;
            int num_statements;
        } block;
        struct AstFnDefinition {
            const char *name;
            struct {
                const char *param_name;
                char **tags;
                int num_tags;
            } *params;
            int num_params;
            AstNode *body;
            // bool is_loop; /// todo: ensure this is removed
            bool is_macro;
            bool is_generator;  // Add this flag
        } fn;
        struct AstFnAnonDefinition {
            struct {
                const char *param_name;
                const char **tags;  /// @todo is this const?
                int num_tags;
            } *params;
            int num_params;
            AstNode *body;
        } fn_anon;
        struct AstStructDefinition {
            const char *name;
            struct {
                const char *name;
                int is_static;
                const char *type;
            } *fields;
            int num_fields;
        } struct_def;
        struct AstStructLiteral {
            const char *struct_name;
            struct {
                const char *name;
                AstNode *value;
            } *fields;
            int num_fields;
        } struct_init;
        struct AstTraitDefinition {
            const char *name;
            struct {
                const char *name;
                const char **params;  /// @todo is this const?
                int num_params;
                AstNode *body;
            } *methods;
            int num_methods;
        } trait;
        struct AstIf {
            AstNode *condition;
            AstNode *body;
            AstNode *else_body;
        } if_stmt;
        struct AstLoop {
            struct {
                const char *identifier;
                const char *assign_op;
                AstNode *value;
                bool is_stream;

            } *bindings;
            int num_bindings;
            AstNode *body;
            AstNode *condition;
            bool yields;
        } loop;
        struct AstLet {
            struct {
                const char *identifier;
                AstNode *expression;
            } *bindings;
            int num_bindings;
            AstNode *body;  /// @todo rename this
        } let_stmt;
        struct AstBinding {
            const char *name;
            AstNode *expression;
            const char *assign_op;
        } binding;  /// @todo  :  go over all the inline bindings and replace with this
        struct AstReturn {
            AstNode *value;
        } return_stmt;
        struct AstMatch {
            const char *variable;
            struct {
                AstNode *test;
                AstNode *value;
                const char *kind;
            } *cases;
            int num_cases;
        } match;
        struct AstDict {
            struct {
                const char *key;
                AstNode *value;
            } *properties;
            int num_properties;
        } dict;
        struct AstVec {
            AstNode **elements;
            int num_elements;
        } vec;  /// @todo  : change to vec
        struct AstUse {
            const char **module_path;
            int num_module_paths;
            const char *alias;
        } use_stmt;
        struct AstMutation {
            AstNode *target;
            AstNode *value;
            bool is_broadcast;
        } mutation;
        struct AstConst {
            const char *name;
            AstNode *value;
        } const_stmt;
        struct AstYield {
            AstNode *value;
        } yield_stmt;
        struct AstIgnore {
        } ignore_stmt;
        struct AstExpression {
            AstNode *expression;
        } exp_stmt;
        struct AstModule {
            const char *name;
        } mod_stmt;
    };
};
#pragma endregion

#pragma region ParserApiHeader

typedef struct Parser {
    AstNode *ast;
    Token *tokens;  /// @todo  : move the global tokens to here
    int num_tokens;
    int index;
    const char **lines;
    int num_lines;
    int depth;
} Parser;

#define parser_print_tokens(parser) lex_print_tokens(parser->tokens, 0, parser->num_tokens, -1)

Parser *parse(Parser *parser);
AstNode *parse_statement(Parser *parser);
void parser_print_ast(AstNode *ast);
void parser_print_ast_node(AstNode *ast, int depth);
Parser *parser_ast_from_source(Parser *parser, const char **lines, int num_lines, const char *indent);
void parser_error(Parser *parser, Token *token, const char *message);
Token *parser_peek(Parser *parser, int offset);
Token *parser_consume(Parser *parser, const char *value, TokenType type, const char *message);
AstNode *parser_expression(Parser *parser, int precedence);
AstNode *parser_expression_part(Parser *parser);
AstNode *parser_keyword_part(Parser *parser, bool in_expr);
AstNode *parser_identifier(Parser *parser);
AstNode *parser_macro(Parser *parser);
AstNode *parser_loop(Parser *parser);
AstNode *parser_for(Parser *parser);
AstNode *parser_if(Parser *parser);
void parser_bindings_loop(Parser *parser, AstNode *node);
AstNode *parser_block(Parser *parser);
AstNode *parser_bracketed(Parser *parser);
AstNode *parser_dereference(Parser *parser);
AstNode *parser_unary(Parser *parser);
AstNode *parser_literal(Parser *parser);
AstNode *parser_consumed_call(Parser *parser, AstNode *calleeNode, int max_args);
AstNode *parser_struct_literal(Parser *parser);
AstNode *parser_match(Parser *parser);
AstNode *parser_vec(Parser *parser);
AstNode *parser_dict(Parser *parser);
AstNode *parser_return(Parser *parser);
AstNode *parser_yield(Parser *parser);
AstNode *parser_ignore(Parser *parser);
AstNode *parser_trait(Parser *parser);
AstNode *parser_struct(Parser *parser);
AstNode *parser_use(Parser *parser);
AstNode *parser_let(Parser *parser);
AstNode *parser_const(Parser *parser);
AstNode *parser_keyword(Parser *parser, const char *keyword, AstType type);
AstNode *parser_function(Parser *parser, bool is_loop);
AstNode *parser_anonymous_function(Parser *parser);
AstNode *parser_mutation(Parser *parser);
int parser_get_precedence(Token *token);

#pragma endregion


////



#pragma region InternalStandardLibraryImpl
/// @todo move all of these unsafe cstr_ fns to safe strings w/ lengths

/// @brief Assuming we keep around the internal precompiler (lex+parse) arena
///         for the interpreter to use, then we don't need to copy data from it.
/// @todo   It seems this is a reasonable strategy from parser AST -> interp_
///             but not from lexer Tokens -> parser ...
///             since the user does not benefit from having tokens in memory
/// @param s
/// @return
const char *cstr_maybe_dup(const char *s) {
    if (arena_is_owner(_glo_arena_internal, s)) {
        return s;
    } else {
        return cstr_dup(s);
    }
}

const char *cstr_dup(const char *s) {
    int len = 0;
    while (s[len]) len++;
    char *dup = (char *)dou_new(len + 1);
    if (!dup) return NULL;
    for (int i = 0; i <= len; i++) {
        dup[i] = s[i];
    }
    return dup;
}

char *cstr_ndup(const char *s, unsigned int len) {
    char *dup = (char *)dou_new(len + 1);
    if (!dup) return NULL;
    for (int i = 0; i < len; i++) {
        dup[i] = s[i];
    }
    dup[len] = '\0';
    return dup;
}

char *cstr_fmt(const char *format, ...) {
    va_list args;
    va_start(args, format);

    va_list args_copy;
    va_copy(args_copy, args);
    int len = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);
    if (len < 0) {
        va_end(args);
        return NULL;
    }
    char *formatted = (char *)dou_new(len + 1);  // +1 for null terminator
    if (!formatted) {
        va_end(args);
        return NULL;
    }
    vsnprintf(formatted, len + 1, format, args);
    va_end(args);
    return formatted;
}

int cstr_eq(const char *s1, const char *s2) {
    if (s1 == NULL || s2 == NULL) return 0;
    return strcmp(s1, s2) == 0;
}

int cstr_cmp_nat(const char *a, const char *b) {
    while (*a && *b) {
        if (isdigit((unsigned char)*a) && isdigit((unsigned char)*b)) {
            char *end_a;
            char *end_b;
            unsigned long num_a = strtoul(a, &end_a, 10);
            unsigned long num_b = strtoul(b, &end_b, 10);

            if (num_a != num_b) {
                return (num_a > num_b) - (num_a < num_b);
            }

            a = end_a;
            b = end_b;
        } else {
            if (*a != *b) {
                return (unsigned char)*a - (unsigned char)*b;
            }
            a++;
            b++;
        }
    }

    if (*a == '\0' && *b == '\0') {
        return 0;
    }
    return (*a == '\0') ? -1 : 1;
}

int cstr_len(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void cstr_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void cstr_ncopy(char *dest, const char *src, int len) {
    int i = 0;
    while (len--) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

char **cstr_lines(const char *input, int *bro_out_numlines) {
    *bro_out_numlines = 0;
    for (const char *p = input; *p != '\0'; p++) {
        if (*p == '\n') {
            (*bro_out_numlines)++;
        }
    }

    if (input[strlen(input) - 1] != '\n') {
        (*bro_out_numlines)++;
    }

    char **lines = (char **)dou_new((*bro_out_numlines) * sizeof(char *));

    if (!lines) return NULL;

    const char *start = input;
    int line_index = 0;

    for (const char *p = input; *p != '\0'; p++) {
        if (*p == '\n' || *(p + 1) == '\0') {
            int length = (*p == '\n') ? (p - start) : (p - start + 1);
            char *line = (char *)dou_new(length + 1);
            if (!line) return NULL;  // nb. some lines remain allocated

            cstr_ncopy(line, start, length);
            lines[line_index++] = line;

            start = p + 1;
        }
    }

    return lines;
}

void cli_parse_args(int argc, char *argv[], CliOption *out_opts, size_t num_opts) {
    for (int i = 1; i < argc; ++i) {
        char *arg = argv[i];
        if (arg[0] == '-' && arg[1] == '-') {
            arg += 2;
            for (size_t j = 0; j < num_opts; ++j) {
                if (cstr_eq(out_opts[j].name, arg) && !out_opts[j].is_set) {
                    out_opts[j].is_set = true;
                    out_opts[j].value = argv[++i];
                }
            }
        } else if (arg[0] == '-') {
            arg += 1;
            for (size_t j = 0; j < num_opts; ++j) {
                if (cstr_eq(out_opts[j].name, arg) && !out_opts[j].is_set) {
                    out_opts[j].is_set = true;
                }
            }
        }
    }
}

void cli_print_opts(const CliOption options[], int num_options) {
    printf("Command-Line Options:\n");
    printf("----------------------------\n");
    for (int i = 0; i < num_options; i++) {
        printf("Option: %-10s | Set: %-5s | Value: %s\n",
               options[i].name,
               options[i].is_set ? "Yes" : "No",
               (options[i].is_set ? options[i].value : options[i].or_else));
    }
    printf("----------------------------\n");
}

#pragma endregion

#pragma region ErrorImpl

void error_to_user(DouError type) {
    // TODO
    //  switch(type) {
    //      case ERROR_ZERO:
    //      break;
    //  }
}

void error_log_message(LogLevel level, const char *func, const char *format, ...) {
    if (level < 0 || level >= LOG_LEVEL_LEN) {
        level = LOG_LEVEL_ERROR;
    }

    fprintf(stderr, "[%-7s] %20s: ", LogLevelStr[level], func);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
}

///

void tracer_print_stack(CallStack *traces, int depth) {
    fprintf(stderr, "Traceback (most recent call last):\n");
    for (int i = traces->top; i >= 0 && depth--; i--) {
        fprintf(stderr, "   %s:%d\t\tat %s( %s )\n",
                traces->entries[i].file,
                traces->entries[i].line,
                traces->entries[i].fn_name,
                traces->entries[i].argument == NULL ? "(null)" : traces->entries[i].argument);
    }

    if (depth > 0) {
        fprintf(stderr, "   <-- TRACEBACK FINISHED EARLY -->\n");
    }
}
void tracer_push(CallStack *traces, const char *fn_name, const char *arg, const char *file, int line) {
    if (traces->top > TRACER_MAX_DEPTH - 1) {
        fprintf(stderr, "Traceback overflow: cycling tracer history!\n");
        // tracer_print_stack(traces, 10);
        traces->top = -1;
    }

    traces->top++;
    traces->entries[traces->top].fn_name = fn_name;
    traces->entries[traces->top].argument = arg;
    traces->entries[traces->top].file = file;
    traces->entries[traces->top].line = line;
}

// Pop a function from the call stack
void tracer_pop(CallStack *traces) {
    if (traces->top >= 0) {
        traces->top--;
    } else {
        fprintf(stderr, "Traceback underflow: Mis_matched push/pop calls!\n");
    }
}

#pragma endregion

#pragma region MemoryManagementImpl

Heap *dou_dyn_init(uint32_t initial_capacity, uint32_t gc_threshold) {
    Heap *heap = (Heap *)malloc(sizeof(Heap));
    if (!heap) {
        return NULL;
    }

    heap->capacity = initial_capacity;
    heap->size = 0;
    heap->gc_threshold = gc_threshold;
    heap->generation_count = 0;

    heap->free_list = NULL;
    heap->objects = (HeapObject **)malloc(sizeof(HeapObject *) * heap->capacity);
    if (!heap->objects) {
        free(heap);
        return NULL;
    }
    memset(heap->objects, 0, sizeof(HeapObject *) * heap->capacity);

    return heap;
}

GCError dou_dyn_destroy(Heap *heap) {
    if (!heap) {
        return GC_ERR_INVALID_POINTER;
    }

    for (uint32_t i = 0; i < heap->size; ++i) {
        if (heap->objects[i]) {
            free(heap->objects[i]->data);
            free(heap->objects[i]);
        }
    }

    free(heap->objects);
    free(heap);

    return GC_SUCCESS;
}

void *dou_dyn_new(Heap *heap, uint32_t size) {
    if (!heap) {
        return NULL;
    }

    if (heap->size >= heap->gc_threshold) {
        dou_dyn_gc_run(heap);
    }

    HeapObject *obj = NULL;

    if (heap->free_list) {
        // Reuse object from free list
        obj = heap->free_list;
        heap->free_list = heap->free_list->next;
    } else {
        // Allocate new HeapObject if capacity allows
        if (heap->size >= heap->capacity) {
            // Attempt to expand the heap
            uint32_t new_capacity = heap->capacity * 2;
            HeapObject **new_objects = realloc(heap->objects, sizeof(HeapObject *) * new_capacity);
            if (!new_objects) {
                return NULL;  // Heap expansion failed
            }
            // Initialize new slots
            memset(new_objects + heap->capacity, 0, sizeof(HeapObject *) * (new_capacity - heap->capacity));
            heap->objects = new_objects;
            heap->capacity = new_capacity;
        }

        obj = (HeapObject *)malloc(sizeof(HeapObject));
        if (!obj) {
            return NULL;
        }
        obj->type = HEAP_ALIVE;
        obj->marked = false;
        obj->next = NULL;
        heap->objects[heap->size++] = obj;
    }

    obj->type = HEAP_ALIVE;
    obj->size = size;
    obj->marked = false;
    obj->next = NULL;

    obj->data = malloc(size);
    if (!obj->data) {
        obj->type = HEAP_DEAD;
        return NULL;
    }

    return obj->data;
}

GCError dou_dyn_free(Heap *heap, void *ptr) {
    if (!heap || !ptr) {
        return GC_ERR_INVALID_POINTER;
    }

    HeapObject *obj = dou_dyn_find(heap, ptr);
    if (!obj) {
        return GC_ERR_INVALID_POINTER;
    }

    if (obj->type == HEAP_DEAD) {
        return GC_ERR_INVALID_POINTER;
    }

    obj->type = HEAP_DEAD;

    // Add to free list
    obj->next = heap->free_list;
    heap->free_list = obj;

    return GC_SUCCESS;
}

void dou_dyn_print(const Heap *heap) {
    if (!heap) {
        printf("Heap is NULL\n");
        return;
    }

    printf("Heap State:\n");
    printf("Capacity: %u\n", heap->capacity);
    printf("Size: %u\n", heap->size);
    printf("GC Threshold: %u\n", heap->gc_threshold);
    printf("Generation Count: %u\n", heap->generation_count);
    printf("Free List: %p\n", (void *)heap->free_list);

    // Count types
    uint32_t alive = 0, dead = 0, pinned = 0;
    for (uint32_t i = 0; i < heap->size; ++i) {
        if (heap->objects[i]) {
            switch (heap->objects[i]->type) {
                case HEAP_ALIVE:
                    alive++;
                    break;
                case HEAP_DEAD:
                    dead++;
                    break;
                case HEAP_PINNED:
                    pinned++;
                    break;
                default:
                    break;
            }
        }
    }

    printf("HEAP_ALIVE: %u, HEAP_DEAD: %u, HEAP_PINNED: %u\n", alive, dead, pinned);
}

void dou_dyn_debug_dump(const Heap *heap) {
    if (!heap) {
        printf("Heap is NULL\n");
        return;
    }

    printf("Heap Debug Dump:\n");
    printf("Capacity: %u, Size: %u, GC Threshold: %u, Generation Count: %u\n",
           heap->capacity, heap->size, heap->gc_threshold, heap->generation_count);
    printf("Free List:\n");
    HeapObject *current = heap->free_list;
    while (current) {
        printf("  HeapObject at %p, size: %u\n", (void *)current, current->size);
        current = current->next;
    }

    printf("Allocated Objects:\n");
    for (uint32_t i = 0; i < heap->size; ++i) {
        HeapObject *obj = heap->objects[i];
        if (obj) {
            printf("  [%u] HeapObject at %p: type=%d, size=%u, data=%p, marked=%d\n",
                   i, (void *)obj, obj->type, obj->size, obj->data, obj->marked);
        }
    }
}

void dou_dyn_gc_run(Heap *heap) {
    if (!heap) return;
    LOG(LOG_LEVEL_INFO, "Running Garbage Collector (gen %d)", heap->generation_count);
    dou_dyn_gc_mark_and_sweep(heap);
    heap->generation_count++;
}

void dou_dyn_gc_mark_and_sweep(Heap *heap) {
    // In a real scenario, you would have a list of root objects to start marking
    // For simplicity, let's assume there are no roots and sweep all
    // Alternatively, implement a root set and mark from there

    // Example: If you have a root list, iterate and mark
    // for (each root) {
    //     dou_dyn_gc_mark(heap, root);
    // }

    dou_dyn_gc_sweep(heap);
}

void dou_dyn_gc_mark(Heap *heap, HeapObject *root) {
    if (!heap || !root || root->marked) return;

    root->marked = true;
    // Here you would recursively mark all objects reachable from root->data
    // This requires knowledge of how Dov objects reference each other
    // For simplicity, this is omitted
}

void dou_dyn_gc_sweep(Heap *heap) {
    if (!heap) return;

    for (uint32_t i = 0; i < heap->size;) {
        HeapObject *obj = heap->objects[i];
        if (!obj) {
            i++;
            continue;
        }

        if (obj->marked) {
            obj->marked = false;  // Reset for next GC cycle
            i++;
        } else if (obj->type == HEAP_DEAD) {
            // Already marked for deletion
            // Remove from objects array by swapping with the last
            free(obj->data);
            free(obj);
            heap->objects[i] = heap->objects[heap->size - 1];
            heap->objects[heap->size - 1] = NULL;
            heap->size--;
        } else {
            // Unreachable object, mark as dead
            obj->type = HEAP_DEAD;
            // Add to free list
            obj->next = heap->free_list;
            heap->free_list = obj;
            // Remove from objects array
            heap->objects[i] = heap->objects[heap->size - 1];
            heap->objects[heap->size - 1] = NULL;
            heap->size--;
        }
    }
}

static HeapObject *dou_dyn_find(Heap *heap, void *ptr) {
    if (!heap || !ptr) return NULL;

    for (uint32_t i = 0; i < heap->size; ++i) {
        if (heap->objects[i] && heap->objects[i]->data == ptr) {
            return heap->objects[i];
        }
    }
    return NULL;
}

void arena_init(Arena *arena, unsigned int capacity) {
    arena->capacity = capacity;
    arena->size = 0;
    arena->data = (char *)malloc(arena->capacity);
}

void *arena_new(Arena *arena, unsigned int size) {
    ASSERT(arena->capacity > 0, "Arena capacity must be positive");

    if (arena->size + size > arena->capacity) {
        int new_capacity = arena->capacity * 2;
        while (arena->size + size > new_capacity) {
            new_capacity *= 2;
        }

        char *new_data = (char *)realloc(arena->data, new_capacity);
        if (!new_data) {
            return NULL;
        }
        arena->data = new_data;
        arena->capacity = new_capacity;
    }
    void *ptr = &arena->data[arena->size];
    arena->size += size;
    return ptr;
}

void arena_free_all(Arena *arena) {
    free(arena->data);
    free(arena);
}

void dou_setup_allocators() {
    arena_init(&_glo_arena_internal, MEM_INTERNAL_SIZE);
    arena_init(&_glo_arena_local, MEM_LOCAL_SIZE);
    arena_init(&_glo_arena_global, MEM_GLOBAL_SIZE);

    _glo_arena_interp_current = &_glo_arena_local;
}

void *dou_realloc(void *ptr, int old_size, int new_size) {
    /// @todo  : is this needed if dou_new() is itself reallocating?

    Arena *arena = &_glo_arena_internal;

    ASSERT(arena->capacity > 0, "Arena has no capacity!");
    intptr_t offset = (char *)ptr - arena->data;

    ASSERT(offset >= 0 && offset + old_size <= arena->size, "Invalid pointer for realloc!");

    if (offset + new_size <= arena->size) {
        arena->size = offset + new_size;
        return ptr;
    }

    if (arena->size - old_size + new_size > arena->capacity) {
        int new_capacity = arena->capacity * 2;
        while (arena->size - old_size + new_size > new_capacity) {
            new_capacity *= 2;
        }
        char *new_data = (char *)realloc(arena->data, new_capacity);
        ASSERT(new_data != NULL, "Out of memory");
        if (!new_data) {
            return NULL;
        }
        arena->data = new_data;
        arena->capacity = new_capacity;
        ptr = &arena->data[offset];
    }

    arena->size = offset + new_size;

    return ptr;
}

void *dou_maybe_grow(void *allocated, unsigned int *out_capacity, unsigned int element_size, unsigned int size) {
    if (size == *out_capacity) {
        void *result = dou_realloc(allocated, *out_capacity * element_size, *out_capacity * element_size * 2);
        *out_capacity = *out_capacity * 2;

        ASSERT(result != NULL, "Out of memory");
        return result;
    } else {
        return allocated;
    }
}

void interp_free_all() {
    /// @todo  :
}

#pragma endregion

#pragma region LexerApiImpl

void lex_print_tokens(Token *tokens, int start, int end, int marker) {
    printf("\n Tokens (%d printed):\n", end - start);
    for (int i = start; (i < _glo_num_tokens) && (i < end); i++) {
        Token token = tokens[i];
        printf("   %3d: ", i + 1);
        printf("%s", TokenTypeStr[token.type]);
        if (token.value && cstr_eq(token.value, "\n")) {
            printf(" `\\n`");
        } else if (token.value) {
            printf(" `%s`", token.value);
        }
        if (token.line != 0) {
            printf("@ %d:%d", token.line, token.col);
        }
        if (i == marker) printf("\t\t<--- HERE");
        printf("\n");
    }
}

Token *lex_lines(const char **lines, int num_lines, const char *indent) {
    _glo_indents[0] = 0;
    _glo_num_indents = 1;
    bool suppressNL = false;
    int indent_length = 0;
    while (indent[indent_length]) indent_length++;

    for (int line_no = 0; line_no < num_lines; line_no++) {
        const char *line = lines[line_no];
        int i = 0;
        while (line[i] == ' ' || line[i] == '\n') i++;  /// @todo check this
        const char *lex_line = &line[i];
        int trim_len = 0;
        while (lex_line[trim_len]) trim_len++;

        int indentation = i / indent_length;

        lex_dedent(indentation, line_no);
        lex_indent(indentation, line_no);

        if (trim_len == 0) {
            lex_empty_line(line_no);
            continue;
        }

        int pos = 0;
        while (pos < trim_len) {
            int col = pos + 1 + i;
            char c = lex_line[pos];
            char d = lex_line[pos + 1];

            if (c == '/' && pos + 1 < trim_len && lex_line[pos + 1] == '/') {
                lex_comment(lex_line, &pos, trim_len, line_no + 1, col + 1);
                break;  // the rest of the line is a comment, so break
            } else if ((c == '#') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_') {
                lex_word(lex_line, &pos, trim_len, line_no + 1, col + 1, &suppressNL);
            } else if (c >= '0' && c <= '9') {
                lex_number(lex_line, &pos, trim_len, line_no + 1, col + 1);
            } else if (c == '{' || c == '}' || c == '(' || c == ')' || c == '[' || c == ']') {
                lex_bracket(lex_line, &pos, line_no + 1, col + 1);
            } else if (c == '"' || c == '\'') {
                lex_string(lex_line, &pos, trim_len, line_no + 1, col + 1);
            } else if ((c == ':' && d == '=') || c == '=' || c == '<' || c == '-') {
                lex_assign(lex_line, &pos, trim_len, line_no + 1, col + 1, &suppressNL);
            } else if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
                       c == '<' || c == '>' || c == '!' || c == '#' || c == '=') {
                lex_op(lex_line, &pos, trim_len, line_no + 1, col + 1);
            } else if (c == '\n' || c == ',' || c == ':' || c == ';' || c == '.') {
                lex_separator(lex_line, &pos, trim_len, line_no + 1, col + 1, &suppressNL);
            } else {
                pos++;
            }
        }

        if (!suppressNL) {
            lex_newline(line_no);
        }

        suppressNL = false;
    }
    lex_remaining_dedent(num_lines);
    return _glo_tokens;
}

void lex_comment(const char *lex_line, int *pos, int trim_len, int line_no, int col) {
    int start = *pos + 2;  // Skip the initial '//'
    *pos += 2;

    // Capture everything until the end of the line
    while (*pos < trim_len && lex_line[*pos] != '\n') {
        (*pos)++;
    }

    (*pos)++;  // remove the newline

    if (!LEX_CAPTURE_COMMENTS) return;

    /// @note remembered doc-comments have this style
    /// @todo impl something in the parser for this.. eg., add `@help` to scope
    ///         and make @help(scope) return all comments?
    // if(pos[0] == '/') pos++;
    // if(!(pos[0] == '@' || (pos[0] == ' ' && pos[1] == '@'))) return;
    // Token *token = &_glo_tokens[watchpoint(_glo_num_tokens++)];
    // token->type = TT_COMMENT;
    // token->value = cstr_ndup(lex_line + start, *pos - start);
    // token->line = line_no;
    // token->col = col;
}

void lex_dedent(int indentation, int line_no) {
    while (indentation < _glo_indents[_glo_num_indents - 1]) {
        Token *token = &_glo_tokens[_glo_num_tokens++];
        token->type = TT_DEDENT;
        token->line = line_no;
        token->col = 1;
        _glo_num_indents--;
    }
}

void lex_indent(int indentation, int line_no) {
    if (indentation > _glo_indents[_glo_num_indents - 1]) {
        Token *token = &_glo_tokens[_glo_num_tokens++];
        token->type = TT_INDENT;
        token->line = line_no;
        token->col = 1;
        _glo_indents[_glo_num_indents++] = indentation;
    }
}

void lex_empty_line(int line_no) {
    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_END;
    token->value = cstr_dup("\n");
    token->line = line_no;
    token->col = 1;
}

void lex_word(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *supressNL) {
    int start = (*pos)++;
    while (*pos < trim_len &&
           ((lex_line[*pos] >= 'a' && lex_line[*pos] <= 'z') ||
            (lex_line[*pos] >= 'A' && lex_line[*pos] <= 'Z') ||
            (lex_line[*pos] >= '0' && lex_line[*pos] <= '9') ||
            lex_line[*pos] == '_')) {
        (*pos)++;
    }

    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->line = line_no;
    token->col = col;
    token->value = cstr_ndup(lex_line + start, *pos - start);

    if (token->value[0] == '#') {
        token->type = TT_TAG;
    } else if (token->value[0] >= 'A' && token->value[0] <= 'Z' && token->value[0] != '_') {
        token->type = TT_TYPE;
    } else if (lex_is_keyword(token->value)) {
        token->type = TT_KEYWORD;
    } else {
        token->type = TT_IDENTIFIER;
    }
}

void lex_number(const char *lex_line, int *pos, int trim_len, int line_no, int col) {
    int start = (*pos)++;
    while (*pos < trim_len && lex_line[*pos] >= '0' && lex_line[*pos] <= '9') {
        (*pos)++;
    }
    if (*pos < trim_len && lex_line[*pos] == '.') {
        (*pos)++;
        while (*pos < trim_len && lex_line[*pos] >= '0' && lex_line[*pos] <= '9') {
            (*pos)++;
        }
    }
    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_NUMBER;
    token->value = cstr_ndup(lex_line + start, *pos - start);
    token->line = line_no;
    token->col = col;
}

void lex_bracket(const char *lex_line, int *pos, int line_no, int col) {
    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_BRACKET;
    token->value = cstr_ndup(lex_line + *pos, 1);
    token->line = line_no;
    token->col = col;
    (*pos)++;
}

void lex_string(const char *lex_line, int *pos, int trim_len, int line_no, int col) {
    char quote_char = lex_line[*pos];
    int start = ++(*pos);
    while (*pos < trim_len && lex_line[*pos] != quote_char) {
        (*pos)++;
    }

    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_STRING;
    token->value = cstr_ndup(lex_line + start, *pos - start);
    token->line = line_no;
    token->col = col;
    if (*pos < trim_len) {
        (*pos)++;
    }
}

void lex_assign(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *suppressNL) {
    int start = *pos;
    char c = lex_line[*pos];
    if ((c == ':' && *pos + 1 < trim_len && lex_line[*pos + 1] == '=') ||
        (c == '<' && *pos + 1 < trim_len && lex_line[*pos + 1] == '-') ||
        (c == '-' && *pos + 1 < trim_len && lex_line[*pos + 1] == '>')) {
        *pos += 2;
    } else {
        (*pos)++;
    }

    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_ASSIGN;
    token->value = cstr_ndup(lex_line + start, *pos - start);
    token->line = line_no;
    token->col = col;

    if (cstr_eq(token->value, ":=")) {
        *suppressNL = true;
    }
}

void lex_op(const char *lex_line, int *pos, int trim_len, int line_no, int col) {
    int start = (*pos)++;
    while (*pos < trim_len &&
           (lex_line[*pos] == '+' || lex_line[*pos] == '-' ||
            lex_line[*pos] == '*' || lex_line[*pos] == '/' ||
            lex_line[*pos] == '%' || lex_line[*pos] == '<' ||
            lex_line[*pos] == '>' || lex_line[*pos] == '!' ||
            lex_line[*pos] == '#' || lex_line[*pos] == '=')) {
        (*pos)++;
    }

    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_OP;
    token->value = cstr_ndup(lex_line + start, *pos - start);
    token->line = line_no;
    token->col = col;
}

void lex_separator(const char *lex_line, int *pos, int trim_len, int line_no, int col, bool *suppressNL) {
    int start = *pos;
    char c = lex_line[*pos];
    if (c == '.' && *pos + 2 < trim_len &&
        lex_line[*pos + 1] == '.' && lex_line[*pos + 2] == '.') {
        *pos += 3;
    } else {
        (*pos)++;
    }

    char *separator = cstr_ndup(lex_line + start, *pos - start);

    Token *token = &_glo_tokens[_glo_num_tokens++];
    if (separator[0] == '\n') {
        *suppressNL = false;
    }

    if (separator[0] == ';') {
        token->type = TT_DISCARD;
    } else if (cstr_eq(separator, "...")) {
        token->type = TT_IGNORE;
    } else if (separator[0] == '.') {
        token->type = TT_DEREF;
    } else if (separator[0] == ':') {
        token->type = TT_ANNOTATION;
    } else {
        token->type = TT_SEP;
    }

    token->value = separator;
    token->line = line_no;
    token->col = col;
}

void lex_newline(int line_no) {
    Token *token = &_glo_tokens[_glo_num_tokens++];
    token->type = TT_END;
    token->value = cstr_dup("\n");
    token->line = line_no;
    token->col = 1;
}

void lex_remaining_dedent(int num_lines) {
    while (_glo_num_indents > 1) {
        Token *token = &_glo_tokens[_glo_num_tokens++];
        token->type = TT_DEDENT;
        token->line = num_lines;
        token->col = 1;
        _glo_num_indents--;
    }
}

bool lex_is_keyword(const char *s) {
    for (int i = 0; i < _glo_num_keywords; i++) {
        if (cstr_eq(s, _glo_keywords[i])) {
            return true;
        }
    }
    return false;
}

#pragma endregion

#pragma region ParserApiImpl

// Helper function to print indentation
void print_indent(int depth) {
    for (int i = 0; i < depth; i++) {
        putchar(' ');
    }
}

// Forward declaration
void parser_print_ast_node(AstNode *ast, int depth);

void parser_print_ast(AstNode *ast) {
    if (!ast) {
        printf("AST is NULL\n");
        return;
    }
    parser_print_ast_node(ast, 0);
}

void parser_print_ast_node(AstNode *ast, int depth) {
    if (!ast) {
        print_indent(depth);
        printf("(null)\n");
        return;
    }

    print_indent(depth);
    printf("%s (Line %d:%d)", AstTypeStr[ast->type], ast->line, ast->col);

    depth++;
    switch (ast->type) {
        case AST_ID:
            print_indent(depth);
            printf("ID: %s\n", ast->id.name);
            break;
        case AST_NUM:
            print_indent(depth);
            printf("Number: %f\n", ast->num.value);
            break;
        case AST_STR:
            print_indent(depth);
            printf("String: %s\n", ast->str.value);
            break;
        case AST_TAG:
            print_indent(depth);
            printf("Tag: %s\n", ast->tag.name);
            break;
        case AST_BOP:
            print_indent(depth);
            printf("Binary Operation: %s\n", ast->bop.op);
            parser_print_ast_node(ast->bop.left, depth);
            parser_print_ast_node(ast->bop.right, depth);
            break;
        case AST_UOP:
            print_indent(depth);
            printf("Unary Operation: %s\n", ast->uop.op);
            parser_print_ast_node(ast->uop.operand, depth);
            break;
        case AST_CALL:
            print_indent(depth);
            printf("Function Call: %s\n", ast->call.callee->id.name);  // Assuming callee is an identifier
            parser_print_ast_node(ast->call.callee, depth + 1);
            for (int i = 0; i < ast->call.num_args; i++) {
                parser_print_ast_node(ast->call.args[i], depth + 1);
            }
            break;

        case AST_METHOD_CALL:
            print_indent(depth);
            printf("Method Call: %s\n", ast->method_call.method);
            parser_print_ast_node(ast->method_call.target, depth);
            for (int i = 0; i < ast->method_call.num_args; i++) {
                parser_print_ast_node(ast->method_call.args[i], depth);
            }
            break;
        case AST_MEMBER_ACCESS:
            print_indent(depth);
            printf("Member Access: %s\n", ast->member_access.property);
            parser_print_ast_node(ast->member_access.target, depth);
            break;
        case AST_BLOCK:
            print_indent(depth);
            printf("Block:\n");
            for (int i = 0; i < ast->block.num_statements; i++) {
                parser_print_ast_node(ast->block.statements[i], depth);
            }
            break;
        case AST_FN:
        case AST_FN_ANON:
            print_indent(depth);
            printf("Function %s:\n", (ast->type == AST_FN ? "Definition" : "Anonymous"));
            for (int i = 0; ast->fn.params && i < ast->fn.num_params; i++) {
                print_indent(depth + 1);
                printf("Param: %s\n", ast->fn.params[i].param_name);
            }
            parser_print_ast_node(ast->fn.body, depth + 1);
            break;
        case AST_STRUCT:
            print_indent(depth);
            printf("Struct: %s\n", ast->struct_def.name);
            for (int i = 0; i < ast->struct_def.num_fields; i++) {
                print_indent(depth + 1);
                printf("Field: %s (Type: %s, Static: %d)\n",
                       ast->struct_def.fields[i].name,
                       ast->struct_def.fields[i].type,
                       ast->struct_def.fields[i].is_static);
            }
            break;
        case AST_STRUCT_INIT:
            print_indent(depth);
            printf("Struct Initialization: %s\n", ast->struct_init.struct_name);
            for (int i = 0; i < ast->struct_init.num_fields; i++) {
                print_indent(depth + 1);
                printf("Field: %s\n", ast->struct_init.fields[i].name);
                parser_print_ast_node(ast->struct_init.fields[i].value, depth + 1);
            }
            break;
        case AST_TRAIT:
            print_indent(depth);
            printf("Trait: %s\n", ast->trait.name);
            for (int i = 0; i < ast->trait.num_methods; i++) {
                print_indent(depth + 1);
                printf("Method: %s\n", ast->trait.methods[i].name);
                parser_print_ast_node(ast->trait.methods[i].body, depth + 1);
            }
            break;
        case AST_LET:
            print_indent(depth);
            printf("Let Statement:\n");
            for (int i = 0; i < ast->let_stmt.num_bindings; i++) {
                print_indent(depth + 1);
                printf("Binding: %s\n", ast->let_stmt.bindings[i].identifier);
                parser_print_ast_node(ast->let_stmt.bindings[i].expression, depth + 1);
            }
            parser_print_ast_node(ast->let_stmt.body, depth);
            break;
        case AST_VEC:
            print_indent(depth);
            printf("Vector (Elements: %d):\n", ast->vec.num_elements);
            for (int i = 0; ast->vec.elements && i < ast->vec.num_elements; i++) {
                print_indent(depth + 1);
                printf("Element %d:", i + 1);
                parser_print_ast_node(ast->vec.elements[i], depth + 2);
            }
            break;
        case AST_MATCH:
            print_indent(depth);
            printf("Match Statement (Variable: %s):\n", ast->match.variable);
            for (int i = 0; i < ast->match.num_cases; i++) {
                print_indent(depth + 1);
                printf("Case (Kind: %s):\n", ast->match.cases[i].kind);
                parser_print_ast_node(ast->match.cases[i].test, depth + 2);
                parser_print_ast_node(ast->match.cases[i].value, depth + 2);
            }
            break;
        case AST_YIELD:
            print_indent(depth);
            printf("Yield:\n");
            parser_print_ast_node(ast->yield_stmt.value, depth);
            break;
        case AST_IGNORE:
            print_indent(depth);
            printf("Ignore Statement\n");
            break;
        case AST_EXP:
            print_indent(depth);
            printf("Expression:\n");
            parser_print_ast_node(ast->exp_stmt.expression, depth);
            break;
        case AST_MOD:
            print_indent(depth);
            printf("Module: %s\n", ast->mod_stmt.name);
            break;
        case AST_CONST:
            print_indent(depth);
            printf("Constant: %s\n", ast->const_stmt.name);
            parser_print_ast_node(ast->const_stmt.value, depth);
            break;
        case AST_MUTATION:
            print_indent(depth);
            printf("Mutation:\n");
            parser_print_ast_node(ast->mutation.target, depth);
            parser_print_ast_node(ast->mutation.value, depth);
            break;
        case AST_USE:
            print_indent(depth);
            printf("Use Statement:\n");
            for (int i = 0; i < ast->use_stmt.num_module_paths; i++) {
                print_indent(depth + 1);
                printf("Module Path: %s\n", ast->use_stmt.module_path[i]);
            }
            if (ast->use_stmt.alias) {
                print_indent(depth + 1);
                printf("Alias: %s\n", ast->use_stmt.alias);
            }
            break;

        case AST_FN_MACRO:
            // todo
            break;

        case AST_RETURN:

            /// @todo  :
            break;
        default:
            print_indent(depth);
            printf("Unhandled node type: %s\n", AstTypeStr[ast->type]);
    }
}

Parser *parser_ast_from_source(Parser *parser, const char **lines, int num_lines, const char *indent) {
    parser->tokens = lex_lines(lines, num_lines, indent);

    parser->num_tokens = _glo_num_tokens;
    parser->index = 0;
    parser->lines = lines;
    parser->num_lines = num_lines;
    parser->depth = 0;

    ASSERT(parser->tokens != NULL, "Tokens is NULL");
    ASSERT(parser->num_tokens > 0, "No tokens counted");
    ASSERT(parser->lines != NULL, "Lines is NULL");
    ASSERT(parser->lines[0] != NULL, "First line is NULL");
    ASSERT(parser->num_lines > 0, "No lines of code.");

    return parse(parser);
}

void parser_error(Parser *parser, Token *token, const char *message) {
    LOG(LOG_LEVEL_ERROR, "Parser Current Token: %d / %d", parser->index, parser->num_tokens);

    ASSERT(parser->lines != NULL, "Parser has lost its lines of code");
    ASSERT(parser->index >= 0, "Parser has gone below its lines of code");
    ASSERT(parser->index <= parser->num_tokens, "Parser has gone above its number of tokens");
    ASSERT(token->line != 0, "Token has no line associated with it");
    ASSERT(token->col != 0, "Token has no column associated with it");

    int line = token->line;
    int col = token->col;
    const char *line_content = parser->lines[token->line - 1];  /// @todo Token line numbers are off

    if (line >= 0 && col >= 0) {
        printf("\n Parse Error at line %d, col %d: %s\n", line, col, message);
        if (line_content) {
            printf("   Line: %s\n", line_content);
            printf("   Col : %*s^\n", col, "");
        }
    } else {
        printf("Parse Error: %s\n", message);
    }

    /// @todo  : guard this on a debugging level
    printf("   Parser Index: %d\n\n", parser->index);
    parser_tracer_print_stack();
    if (parser->index > 10) {
        lex_print_tokens(parser->tokens, parser->index - 10, parser->index + 10, parser->index);
    } else {
        lex_print_tokens(parser->tokens, 0, 10, parser->index);
    }
    abort();
}

char *token_info(Token *token) {
    const char *value = token->value;
    if (cstr_eq(token->value, "\n")) {
        value = "\\n";
    }

    char *info = cstr_fmt("%d: %s @ %d:%d", token->type, value, token->line + 1, token->col);
    if (!info) return NULL;
    return info;

    // char *info = (char *)dou_new(256);
    // int len = sprintf(info, "%d: %s @ %d:%d", token->type, value, token->line + 1, token->col);
    // info[len] = '\0';
}

#define peek() ((parser->index < parser->num_tokens) ? &parser->tokens[parser->index] : NULL)
#define peek_ahead() (&parser->tokens[parser->index + 1])

#define consume(value, type, message) parser_consume(parser, value, type, message)
#define consume_type(type, message) parser_consume(parser, NULL, type, message)
#define consume_any() parser_consume(parser, NULL, 0, NULL)
#define consume_newlines() parser_consume_newlines(parser)  /// @todo: should the lexer just collapse?

Token *parser_peek(Parser *parser, int offset) {
    if (parser->index + offset < parser->num_tokens) {
        return &parser->tokens[parser->index + offset];
    }
    return NULL;
}

int maybe(Parser *parser, const char *value, TokenType type) {
    Token *token = peek();
    if (token && (!type || token->type == type) && (!value || cstr_eq(token->value, value))) {
        parser->index++;
        return 1;
    }
    return 0;
}

Token *parser_consume(Parser *parser, const char *value, TokenType type, const char *message) {
    Token *token = peek();
    if (token && (!type || token->type == type) && (!value || cstr_eq(token->value, value))) {
        parser->index++;
        return token;
    }

    parser_error(parser, token, message ? message : error_withloc("Syntax error"));
    return NULL;
}

Token *parser_consume_newlines(Parser *parser) {
    Token *token;
    while ((token = peek()) && cstr_eq(token->value, "\n")) {
        parser->index++;
    }
    return NULL;
}

Parser *parse(Parser *parser) {
    /// @todo parser_node_new() and MIN_NODES vs. max
    parser->ast = (AstNode *)dou_new(sizeof(AstNode));
    parser->ast->type = AST_BLOCK;
    parser->ast->block.statements = (AstNode **)dou_new(PARSER_MAX_NODES * sizeof(AstNode *));
    parser->ast->block.num_statements = 0;

    while (parser->index < parser->num_tokens) {
        parser_trace("parse");
        parser->depth = 0;
        AstNode *statement = parse_statement(parser);
        if (statement) {
            parser->ast->block.statements[parser->ast->block.num_statements++] = statement;
        }
        parser_clear_trace();
    }

    return parser;
}

AstNode *parse_statement(Parser *parser) {
    Token *token = peek();
    parser_trace(token ? TokenTypeStr[token->type] : "(null)");

    if (!token) return NULL;
    if (token->value && cstr_eq(token->value, "\n")) {
        // consume("\n", 0, NULL);
        consume_newlines();
        return NULL;
    } else if (token->type == TT_KEYWORD) {
        return parser_keyword_part(parser, false);
    } else if (token->type == TT_INDENT) {
        return parser_block(parser);
    } else if (token->value) {
        /// @todo: move out to a parser_expression_stmt
        AstNode *expression = parser_expression(parser, 0);
        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_EXP;
        node->exp_stmt.expression = expression;
        consume_type(TT_END, error_withloc("Statements should end with a newline"));
        return node;
    }

    parser_error(parser, token, error_withloc("Failed to parse statement"));
    return NULL;
}

AstNode *parser_expression(Parser *parser, int precedence) {
    if (parser->depth++ > 512) {
        parser_error(parser, peek(), error_withloc("High levels of parser recursion"));
    }

    Token *token = peek();
    parser_trace(TokenTypeStr[token->type]);

    if (token->type == TT_END) return NULL;

    AstNode *left = parser_expression_part(parser);

    while (true) {
        Token *token = peek();
        if (!token) break;
        if (cstr_eq(token->value, "\n") || cstr_eq(token->value, ",")) break;
        if (token->type == TT_BRACKET || token->type == TT_ASSIGN) break;

        int new_prec = parser_get_precedence(token);
        if (new_prec <= precedence) break;

        /// @todo  : is there some need for bounds checking here?

        consume_any();
        AstNode *right = parser_expression(parser, new_prec);

        if (right) {
            AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
            node->type = AST_BOP;
            node->bop.op = token->value;
            node->bop.left = left;
            node->bop.right = right;
            left = node;
        }
    }

    return left;
}

AstNode *parser_expression_part(Parser *parser) {
    Token *token = peek();

    parser_trace(token->value);

    switch (token->type) {
        case TT_KEYWORD:
            return parser_keyword_part(parser, true);
        case TT_TAG:
        case TT_NUMBER:
        case TT_STRING:
            return parser_literal(parser);
        case TT_DEREF:
            return parser_dereference(parser);
        case TT_IDENTIFIER:
            return parser_identifier(parser);
        case TT_TYPE:
            return parser_struct_literal(parser);
        case TT_DISCARD:
            return NULL;
        case TT_BRACKET:
            return parser_bracketed(parser);
        case TT_OP:
            return parser_unary(parser);
        default:
            parser_error(parser, token, error_withloc("Unknown token type or not allowed in an expression"));
            return NULL;
    }
}
AstNode *parser_keyword_part(Parser *parser, bool in_expr) {
    Token *token = peek();
    parser_trace(TokenTypeStr[token->type]);

    if (!token || token->type != TT_KEYWORD) {
        parser_error(parser, token, error_withloc("Expected a keyword"));
        return NULL;
    }

    if (cstr_eq(token->value, "use")) {
        if (in_expr) parser_error(parser, token, error_withloc("Not allowed in an expression"));
        return parser_use(parser);
    } else if (cstr_eq(token->value, "trait")) {
        if (in_expr) parser_error(parser, token, error_withloc("Not allowed in an expression"));
        return parser_trait(parser);
    } else if (cstr_eq(token->value, "struct")) {
        if (in_expr) parser_error(parser, token, error_withloc("Not allowed in an expression"));
        return parser_struct(parser);
    } else if (cstr_eq(token->value, "const")) {
        if (in_expr) parser_error(parser, token, error_withloc("Not allowed in an expression"));
        return parser_const(parser);
    } else if (cstr_eq(token->value, "fn")) {
        return parser_function(parser, false);
    } else if (cstr_eq(token->value, "for")) {
        return parser_for(parser);
    } else if (cstr_eq(token->value, "if")) {
        return parser_if(parser);
    } else if (cstr_eq(token->value, "let")) {
        return parser_let(parser);
    } else if (cstr_eq(token->value, "mut") || cstr_eq(token->value, "mut*")) {
        return parser_mutation(parser);
    } else if (cstr_eq(token->value, "match")) {
        return parser_match(parser);
    } else if (cstr_eq(token->value, "yield")) {
        return parser_yield(parser);
    } else if (cstr_eq(token->value, "return")) {
        return parser_return(parser);
    } else if (cstr_eq(token->value, "loop")) {
        return parser_loop(parser);
    } else if (cstr_eq(token->value, "mod")) {
        return parser_mutation(parser);
    } else if (cstr_eq(token->value, "macro")) {
        return parser_macro(parser);
    } else {
        parser_error(parser, token, error_withloc("Unknown keyword"));
        return NULL;
    }
}

AstNode *parser_identifier(Parser *parser) {
    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_ID;

    Token *token = consume_type(TT_IDENTIFIER, NULL);
    node->id.name = token->value;
    node->id.type = NULL;

    parser_trace(token->value);

    if (peek() && peek()->value[0] == '(') {
        node = parser_consumed_call(parser, node, 256);
    } else if (peek() && peek()->type == TT_ANNOTATION) {
        node->id.type = cstr_maybe_dup(consume_type(TT_TYPE, NULL)->value);
    }

    return node;
}

int parser_get_precedence(Token *token) {
    parser_trace(token->value);

    if (!token) return 0;
    if (cstr_eq(token->value, "="))
        return 1;
    else if (cstr_eq(token->value, "||"))
        return 2;
    else if (cstr_eq(token->value, "&&"))
        return 3;
    else if (cstr_eq(token->value, "==") || cstr_eq(token->value, "!="))
        return 4;
    else if (cstr_eq(token->value, "<") || cstr_eq(token->value, ">") ||
             cstr_eq(token->value, "<=") || cstr_eq(token->value, ">="))
        return 5;
    else if (cstr_eq(token->value, "+") || cstr_eq(token->value, "-"))
        return 6;
    else if (cstr_eq(token->value, "*") || cstr_eq(token->value, "/") || cstr_eq(token->value, "%"))
        return 7;
    else if (cstr_eq(token->value, "as"))
        return 8;
    else if (cstr_eq(token->value, "#") || cstr_eq(token->value, ";"))
        return 9;
    else
        return 99;
}

AstNode *parser_macro(Parser *parser) {
    consume("macro", TT_KEYWORD, "Expected 'macro' keyword");
    Token *name_token = consume_type(TT_IDENTIFIER, "Expected macro name");
    parser_trace(name_token->value);

    // Implement parsing of macro parameters and body
    // ...

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_FN_MACRO;
    // Assign macro details to node
    // ...

    return node;
}

AstNode *parser_loop(Parser *parser) {
    Token *token = peek();

    parser_trace(token->value);

    consume("loop", TT_KEYWORD, error_withloc("Expected `loop` keyword"));

    token = peek();
    if (token && cstr_eq(token->value, "fn")) {
        return parser_function(parser, true);
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_LOOP;
    node->loop.condition = NULL;

    if (token && cstr_eq(token->value, "if")) {
        consume("if", TT_KEYWORD, NULL);
        AstNode *condition = parser_expression(parser, 0);
        consume_newlines();
        AstNode *body = parser_block(parser);
        node->loop.condition = condition;
        node->loop.body = body;
        node->loop.yields = false;
        return node;
    } else {
        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        // node->type = AST_FOR;
        node->loop.yields = true;  // todo: think about this -- maybe `loop` kw should never yield
                                   //  so repeat this logic with a for loop
        consume_type(TT_END, error_withloc("The `loop` keyword should be on its own line"));
        consume_newlines();
        parser_bindings_loop(parser, node);

        if (peek() && cstr_eq(peek()->value, "match")) {
            consume("match", TT_KEYWORD, error_withloc("The `loop`, expected an `match` block"));
            consume_type(TT_END, error_withloc("The `loop`, the `match` keyword should be on its own line"));
        } else {
            consume("in", TT_KEYWORD, error_withloc("The `loop`, expected an 'in' block"));
            consume_type(TT_END, error_withloc("The `loop`, the 'in' keyword should be on its own line"));
        }

        node->loop.body = parser_block(parser);
        return node;
    }
}

AstNode *parser_for(Parser *parser) {
    parser_trace("parser");

    /// @todo  : this is incorrect, in needs to hand yield etc.

    consume("for", TT_KEYWORD, NULL);
    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_FOR;

    int num_bindings = 0;
    parser_bindings_loop(parser, node);

    AstNode *body = NULL;
    Token *token = peek();
    if (token && token->type == TT_INDENT) {
        body = parser_block(parser);
    } else {
        body = parser_expression(parser, 0);
    }

    node->loop.num_bindings = num_bindings;
    node->loop.body = body;

    return node;
}

AstNode *parser_if(Parser *parser) {
    consume("if", TT_KEYWORD, NULL);
    Token *token = peek();

    parser_trace(token->value);

    AstNode *condition = parser_expression(parser, 0);

    AstNode *body = NULL;
    if (token && token->type == TT_INDENT) {
        body = parser_block(parser);
    } else {
        body = parser_expression(parser, 0);
    }

    AstNode *else_body = NULL;
    token = peek();
    if (token && token->type == TT_KEYWORD && cstr_eq(token->value, "else")) {
        consume("else", TT_KEYWORD, NULL);
        token = peek();
        if (token && token->type == TT_INDENT) {
            else_body = parser_block(parser);
        } else {
            else_body = parser_expression(parser, 0);
        }
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_IF;
    node->if_stmt.condition = condition;
    node->if_stmt.body = body;
    node->if_stmt.else_body = else_body;
    return node;
}

void parser_bindings_loop(Parser *parser, AstNode *node) {
    Token *token = peek();
    parser_trace(token->value);

    unsigned int capacity = PARSER_MIN_NODE_CAPACITY;
    node->loop.bindings = (typeof(node->loop.bindings))dou_new(capacity * sizeof(typeof(node->loop.bindings)));
    node->loop.num_bindings = 0;

    int is_bracket = token->value && cstr_eq(token->value, "(");
    if (is_bracket) {
        consume("(", TT_BRACKET, NULL);
    }

    int is_indent = token && token->type == TT_INDENT;
    if (is_indent) {
        consume_type(TT_INDENT, NULL);
    }

    TokenType closing = is_bracket ? TT_BRACKET : TT_DEDENT;

    while (parser->index < parser->num_tokens) {
        /// @todo  : refactor so all bindings go through one system
        //....  may require an AST binding node? & single emit fn

        token = peek();
        if (!token || (is_bracket && cstr_eq(token->value, ")")) || (is_indent && token->type == TT_DEDENT)) {
            break;
        }

        Token *id_token = consume_type(TT_IDENTIFIER, error_withloc("Expected identifier in loop binding"));
        const char *identifier = id_token->value;

        Token *op_token = consume_type(TT_ASSIGN, error_withloc("Expected assignment op in loop binding"));
        const char *assign_op = op_token->value;

        AstNode *value = parser_expression(parser, 0);

        if (is_indent) {
            consume_type(TT_END, error_withloc("Bindings should end one a newline"));
        } else if (is_bracket && node->loop.num_bindings >= 1) {
            consume(",", TT_SEP, error_withloc("for-yield bindings are comma-separated"));
        }

        node->loop.bindings[node->loop.num_bindings].identifier = identifier;
        node->loop.bindings[node->loop.num_bindings].assign_op = assign_op;
        node->loop.bindings[node->loop.num_bindings].value = value;
        node->loop.bindings[node->loop.num_bindings].is_stream = cstr_eq(assign_op, "<-");
        node->loop.num_bindings++;
    }

    if (is_bracket) {
        consume(")", TT_BRACKET, error_withloc("for-yield bindings should close with a parenthesis"));
    }

    if (is_indent) {
        consume_type(TT_DEDENT, error_withloc("loop bindings should close with a dedent"));
    }
}

AstNode *parser_block(Parser *parser) {
    Token *token = peek();
    parser_trace(token->value ? token->value : TokenTypeStr[token->type]);

    if (!token || token->type != TT_INDENT) {
        parser_error(parser, token, error_withloc("Expected INDENT to start a block"));
        return NULL;
    }
    consume_type(TT_INDENT, error_withloc("Blocks must be indented"));
    // consume_newlines(); // @todo: ensure lexer removes CLEARs if there's a INDENT

    unsigned int capacity = PARSER_MIN_NODE_CAPACITY;
    AstNode **statements = (AstNode **)dou_new(capacity * sizeof(AstNode *));
    int num_statements = 0;

    while (parser->index < parser->num_tokens - 2) {
        consume_newlines();

        token = peek();
        if (!token || token->type == TT_DEDENT) {
            break;
        }

        parser_trace(token->value);
        AstNode *stmt = parse_statement(parser);
        if (stmt) {
            statements[num_statements++] = stmt;
        }
    }

    consume_type(TT_DEDENT, error_withloc("Blocks should finish with a dedent"));
    // consume_type(TT_END, error_withloc("Blocks should finish on a newline"));
    consume_newlines();

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_BLOCK;
    node->block.statements = statements;
    node->block.num_statements = num_statements;
    return node;
}

AstNode *parser_bracketed(Parser *parser) {
    Token *token = peek();
    parser_trace(token->value);

    if (!token || token->type != TT_BRACKET) {
        parser_error(parser, token, error_withloc("Expected bracket"));
        return NULL;
    }

    if (cstr_eq(token->value, "(")) {
        consume("(", TT_BRACKET, NULL);
        AstNode *expr = parser_expression(parser, 0);
        consume(")", TT_BRACKET, NULL);
        return expr;
    } else if (cstr_eq(token->value, "[")) {
        return parser_vec(parser);
    } else if (cstr_eq(token->value, "{")) {
        return parser_dict(parser);
    } else {
        parser_error(parser, token, error_withloc("Unknown bracket type"));
        return NULL;
    }
}

AstNode *parser_dereference(Parser *parser) {
    consume(".", TT_DEREF, NULL);

    AstNode *target = parser_expression(parser, 0);
    Token *token = consume_type(TT_IDENTIFIER, NULL);
    parser_trace(token->value);

    const char *property = token->value;

    token = peek();
    if (token && cstr_eq(token->value, "(")) {
        AstNode *method_call = parser_consumed_call(parser, NULL, 256);

        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_METHOD_CALL;
        node->method_call.target = target;
        node->method_call.method = property;
        node->method_call.args = method_call->call.args;
        node->method_call.num_args = method_call->call.num_args;
        return node;
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_MEMBER_ACCESS;
    node->member_access.target = target;
    node->member_access.property = property;
    return node;
}

AstNode *parser_unary(Parser *parser) {
    Token *token = consume_type(TT_OP, NULL);
    parser_trace(token->value);

    const char *op = token->value;

    if (cstr_eq(op, "#") || cstr_eq(op, "!")) {
        int precedence = parser_get_precedence(token);
        AstNode *operand = parser_expression(parser, precedence);

        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_UOP;
        node->uop.op = op;
        node->uop.operand = operand;
        return node;
    }

    parser_error(parser, token, error_withloc("Unknown unary op"));
    return NULL;
}

AstNode *parser_literal(Parser *parser) {
    Token *token = peek();
    parser_trace(token->value);

    if (!token) {
        parser_error(parser, token, error_withloc("Expected literal"));
        return NULL;
    }

    if (token->type == TT_TAG) {
        token = consume_type(TT_TAG, NULL);

        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_TAG;
        node->tag.name = token->value;

        return node;
    } else if (token->type == TT_NUMBER) {
        token = consume_type(TT_NUMBER, NULL);

        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_NUM;
        node->num.value = atof(token->value);
        return node;
    } else if (token->type == TT_STRING) {
        token = consume_type(TT_STRING, NULL);

        AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
        node->type = AST_STR;
        node->str.value = token->value;
        return node;
    } else {
        parser_error(parser, token, error_withloc("No such literal"));
        return NULL;
    }
}

AstNode *parser_consumed_call(Parser *parser, AstNode *calleeNode, int max_args) {
    const char *callee = calleeNode->id.name;  /// @todo is this right in the case of methods?

    parser_trace(callee);

    consume("(", TT_BRACKET, NULL);

    AstNode **args = (AstNode **)dou_new(max_args * sizeof(AstNode *));
    int num_args = 0;

    while (parser->index < parser->num_tokens && max_args--) {
        Token *token = peek();
        if (token && cstr_eq(token->value, ")")) {
            break;
        }

        AstNode *arg = parser_expression(parser, 0);
        args[num_args++] = arg;

        token = peek();
        if (token && cstr_eq(token->value, ",")) {
            consume(",", TT_SEP, NULL);
        }
    }

    consume(")", TT_BRACKET, NULL);

    AstNode *result = (AstNode *)dou_new(sizeof(AstNode));
    result->type = AST_CALL;
    result->call.callee = calleeNode;
    result->call.args = args;
    result->call.num_args = num_args;

    Token *token = peek();
    while (token && cstr_eq(token->value, ".")) {
        consume(".", TT_DEREF, NULL);

        token = consume_type(TT_IDENTIFIER, NULL);
        const char *method = token->value;

        token = peek();
        if (token && cstr_eq(token->value, "(")) {
            AstNode *method_call = parser_consumed_call(parser, NULL, max_args);

            AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
            node->type = AST_METHOD_CALL;
            node->method_call.target = result;
            node->method_call.method = method;
            node->method_call.args = method_call->call.args;
            node->method_call.num_args = method_call->call.num_args;
            result = node;
        } else {
            AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
            node->type = AST_MEMBER_ACCESS;
            node->member_access.target = result;
            node->member_access.property = method;
            result = node;
        }
        token = peek();
    }

    return result;
}
AstNode *parser_struct_literal(Parser *parser) {
    Token *token = consume_type(TT_TYPE, NULL);
    parser_trace(token->value);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->struct_init.struct_name = token->value;
    node->type = AST_STRUCT_INIT;

    consume("{", TT_BRACKET, NULL);

    int struct_field_capacity = PARSER_MIN_NODE_CAPACITY;
    node->struct_init.fields = (typeof(node->struct_init.fields))
        dou_new(struct_field_capacity * sizeof(typeof(node->struct_init.fields)));

    node->struct_init.num_fields = 0;

    /// @todo presumably we can just precompute the needed capacity
    /// ... by counting fields until we hit the end of the definition?
    /// ... to do this would require unwinding the parser back to the starting index

    while (parser->index < parser->num_tokens) {
        token = peek();
        if (token && cstr_eq(token->value, "}")) {
            break;
        }
        consume(".", TT_DEREF, error_withloc("Fields should start with a . symbol"));
        token = consume_type(TT_IDENTIFIER, NULL);
        const char *name = token->value;

        consume("=", TT_ASSIGN, NULL);
        AstNode *value = parser_expression(parser, 0);

        node->struct_init.fields[node->struct_init.num_fields].name = name;
        node->struct_init.fields[node->struct_init.num_fields].value = value;
        node->struct_init.num_fields++;

        token = peek();
        if (token && (cstr_eq(token->value, ",") || cstr_eq(token->value, "\n"))) {
            consume_any();
        }
    }

    consume("}", TT_BRACKET, NULL);

    return node;
}

AstNode *parser_match(Parser *parser) {
    consume("match", TT_KEYWORD, NULL);

    Token *token = consume_type(TT_IDENTIFIER, NULL);
    const char *variable = token->value;

    parser_trace(token->value);
    consume_newlines();
    consume_type(TT_INDENT, error_withloc("The `match`, blocks should be indented"));

    int match_case_capacity = PARSER_MIN_NODE_CAPACITY;
    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->match.cases = (typeof(node->match.cases))dou_new(match_case_capacity * sizeof(node->match.cases[0]));
    int num_cases = 0;

    while (parser->index < parser->num_tokens) {
        token = peek();
        if (token && token->type == TT_DEDENT) {
            break;
        }

        token = consume_type(TT_KEYWORD, NULL);
        const char *kind = token->value;

        if (cstr_eq(kind, "if")) {
            AstNode *test = parser_expression(parser, 0);
            consume("->", TT_ASSIGN, error_withloc("The `match` if cases should use the -> op."));
            AstNode *value = parser_expression(parser, 0);
            node->match.cases[num_cases].test = test;
            node->match.cases[num_cases].value = value;
            node->match.cases[num_cases].kind = "if";
            node->match.num_cases++;
            consume_type(TT_END, error_withloc("Match cases should end on a newline"));
            consume_newlines();
        } else if (cstr_eq(kind, "else")) {
            consume("->", TT_ASSIGN, error_withloc("The `match`, else should use the -> op"));
            AstNode *value = parser_expression(parser, 0);

            node->match.cases[num_cases].test = NULL;
            node->match.cases[num_cases].value = value;
            node->match.cases[num_cases].kind = "else";
            node->match.num_cases++;

            consume_type(TT_END, error_withloc("Match cases should end on a newline"));
            consume_newlines();
        } else {
            parser_error(parser, token, error_withloc("Expected 'if' or 'else' in `match` statement"));
            return NULL;
        }
    }

    consume_type(TT_DEDENT, NULL);

    node->type = AST_MATCH;
    node->match.variable = variable;
    node->match.num_cases = num_cases;

    return node;
}

AstNode *parser_vec(Parser *parser) {
    consume("[", TT_BRACKET, NULL);

    int node_capacity = PARSER_MIN_NODE_CAPACITY;
    AstNode **elements = (AstNode **)dou_new(node_capacity * sizeof(AstNode *));
    int num_elements = 0;

    while (parser->index < parser->num_tokens) {
        Token *token = peek();
        parser_trace(token->value);  /// @todo  : should each step be traced?

        if (token && cstr_eq(token->value, "]")) {
            break;
        }

        AstNode *element = parser_expression(parser, 0);
        elements[num_elements++] = element;

        token = peek();
        if (token && cstr_eq(token->value, ",")) {
            consume(",", TT_SEP, NULL);
        }
    }

    if (cstr_eq(peek()->value, ",")) consume(",", TT_SEP, NULL);

    consume("]", TT_BRACKET, NULL);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_VEC;
    node->vec.elements = elements;
    node->vec.num_elements = num_elements;
    return node;
}

AstNode *parser_dict(Parser *parser) {
    consume("{", TT_BRACKET, NULL);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_DICT;

    int dov_dict_capacity = PARSER_MIN_NODE_CAPACITY;
    node->dict.properties = (typeof(node->dict.properties))dou_new(dov_dict_capacity * sizeof(node->dict.properties[0]));
    node->dict.num_properties = 0;

    while (parser->index < parser->num_tokens) {
        Token *token = peek();
        parser_trace(token->value);

        if (token && cstr_eq(token->value, "}")) {
            break;
        }

        token = consume_type(TT_IDENTIFIER, NULL);
        const char *key = token->value;

        consume(":", TT_ANNOTATION, error_withloc("Colon expected"));

        AstNode *value = parser_expression(parser, 0);

        node->dict.properties[node->dict.num_properties].key = key;
        node->dict.properties[node->dict.num_properties].value = value;
        node->dict.num_properties++;

        token = peek();
        if (token && cstr_eq(token->value, ",")) {
            consume(",", TT_SEP, NULL);
        }
    }

    consume("}", TT_BRACKET, NULL);

    return node;
}

AstNode *parser_return(Parser *parser) {
    parser_trace("parser");

    consume("return", TT_KEYWORD, NULL);

    Token *token = peek();
    AstNode *value = NULL;
    if (token && !cstr_eq(token->value, "\n") && !cstr_eq(token->value, ";")) {
        value = parser_expression(parser, 0);
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_RETURN;
    node->return_stmt.value = value;
    return node;
}

AstNode *parser_yield(Parser *parser) {
    consume("yield", TT_KEYWORD, NULL);

    Token *token = peek();
    parser_trace(token->value);

    AstNode *value = NULL;
    if (token && !cstr_eq(token->value, "\n")) {
        value = parser_expression(parser, 0);
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_YIELD;
    node->return_stmt.value = value;
    return node;
}

AstNode *parser_ignore(Parser *parser) {
    parser_trace("parser");

    consume("...", TT_IGNORE, NULL);
    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_IGNORE;
    return node;
}

AstNode *parser_trait(Parser *parser) {
    consume("trait", TT_KEYWORD, NULL);
    Token *name = consume_type(TT_IDENTIFIER, error_withloc("Expected trait name"));
    parser_trace(name->value);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_TRAIT;
    node->trait.name = name->value;

    consume("{", TT_BRACKET, error_withloc("Expected '{' for trait body"));

    return node;
}
AstNode *parser_struct(Parser *parser) {
    consume("struct", TT_KEYWORD, NULL);
    Token *name_token = consume_type(TT_TYPE, error_withloc("Expected type (types must have a first capital letter)"));
    const char *name = cstr_maybe_dup(name_token->value);

    parser_trace(name_token->value);

    // consume("\n", TT_END, NULL);
    consume_newlines();
    consume(NULL, TT_INDENT, error_withloc("Expected INDENT after struct declaration"));

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_STRUCT;
    node->struct_def.name = name;

    int struct_def_fields_capacity = PARSER_MIN_NODE_CAPACITY;
    node->struct_def.fields = (typeof(node->struct_def.fields))
        dou_new(struct_def_fields_capacity * sizeof(node->struct_def.fields[0]));

    node->struct_def.num_fields = 0;

    while (peek() && peek()->type != TT_DEDENT) {
        int is_static = 0;
        if (cstr_eq(peek()->value, "static")) {
            consume("static", TT_KEYWORD, NULL);
            is_static = 1;
        }

        consume_newlines();
        consume(".", TT_DEREF, error_withloc("Expected '.' before field name"));
        Token *field_token = consume_type(TT_IDENTIFIER, error_withloc("Expected field name"));
        const char *field_name = cstr_maybe_dup(field_token->value);

        const char *type = NULL;
        if (peek()->type == TT_ANNOTATION) {
            consume(":", TT_ANNOTATION, "Expected a type annotation");
            type = cstr_maybe_dup(consume_type(TT_TYPE, "Expected a type after a type annotation `:`")->value);
        }

        node->struct_def.fields[node->struct_def.num_fields].name = field_name;
        node->struct_def.fields[node->struct_def.num_fields].is_static = is_static;
        node->struct_def.fields[node->struct_def.num_fields].type = type;
        node->struct_def.num_fields++;

        // consume("\n", TT_END, NULL);
        consume_newlines();
    }

    consume(NULL, TT_DEDENT, error_withloc("Expected DEDENT after struct fields"));

    return node;
}

AstNode *parser_use(Parser *parser) {
    parser_trace("NOT YET IMPLEMENTED");
    LOG(LOG_LEVEL_INFO, "NOT YET IMPLEMENTED (%s)", "Parser for modules");

    consume("use", TT_KEYWORD, NULL);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_USE;

    //     Token *alias = consume_type(TT_IDENTIFIER, error_withloc("Expected alias name"));
    //     node->use_stmt.alias = alias->value;
    // }
    return node;
}
AstNode *parser_let(Parser *parser) {
    consume("let", TT_KEYWORD, NULL);
    consume_type(TT_END, error_withloc("The 'let' keyword should be on its own line"));
    consume_newlines();
    consume_type(TT_INDENT, error_withloc("Expected INDENT after 'let'"));

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_LET;

    int let_binding_capacity = PARSER_MIN_NODE_CAPACITY;
    node->let_stmt.bindings = (typeof(node->let_stmt.bindings))dou_new(let_binding_capacity * sizeof(node->let_stmt.bindings[0]));
    node->let_stmt.num_bindings = 0;

    while (peek() && peek()->type != TT_DEDENT) {
        Token *name_token = consume_type(TT_IDENTIFIER, error_withloc("Expected binding name"));
        parser_trace(name_token->value);

        node->let_stmt.bindings[node->let_stmt.num_bindings].identifier = cstr_maybe_dup(name_token->value);

        Token *assign_op_token = consume_type(TT_ASSIGN, error_withloc("Expected assignment op"));

        AstNode *expression;
        if (cstr_eq(assign_op_token->value, ":=")) {
            node->let_stmt.bindings[node->let_stmt.num_bindings].expression = parser_block(parser);
        } else {
            node->let_stmt.bindings[node->let_stmt.num_bindings].expression = parser_expression(parser, 0);
        }

        node->let_stmt.num_bindings++;

        // consume("\n", TT_END, NULL);
        consume_newlines();
    }

    consume_type(TT_DEDENT, error_withloc("Expected DEDENT after 'let' bindings"));
    consume("in", TT_KEYWORD, error_withloc("Expected 'in' after 'let' bindings"));
    consume_type(TT_END, error_withloc("The 'in' keyword should be on its own line"));
    consume_newlines();

    node->let_stmt.body = parser_block(parser);

    return node;
}

AstNode *parser_const(Parser *parser) {
    consume("const", TT_KEYWORD, NULL);

    if (cstr_eq(peek()->value, "fn")) {
        parser_trace("fn");
        /// @todo  : implement something specific for const fn
        return parser_function(parser, false);
    }

    Token *name = consume_type(TT_IDENTIFIER, error_withloc("Expected constant name"));
    parser_trace(name->value);

    Token *assign_op = consume_type(TT_ASSIGN, error_withloc("Expected `=` or `:=` for constant assignment"));
    if (assign_op->value[0] == ':') {
        consume_type(TT_INDENT, "Expected an indent after `:=`");
        consume_type(TT_END, "Expected a newline with an indent");
    }
    AstNode *value = parser_expression(parser, 0);
    if (assign_op->value[0] == ':') {
        consume_type(TT_END, "Expected a newline with a dedent");
        consume_type(TT_DEDENT, "Expected an indent after `:=`");
    }

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_CONST;
    node->const_stmt.name = name->value;
    node->const_stmt.value = value;
    return node;
}

AstNode *parser_keyword(Parser *parser, const char *keyword, AstType type) {
    parser_trace(keyword);

    consume(keyword, TT_KEYWORD, NULL);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = type;
    return node;
}
AstNode *parser_function(Parser *parser, bool is_generator) {
    Token *next = peek_ahead();
    if (next && cstr_eq(next->value, "(")) {
        parser_trace("anon fn");
        return parser_anonymous_function(parser);
    }

    consume("fn", TT_KEYWORD, NULL);

    /// @todo  : maybe factor out repeat new
    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_FN;
    node->fn.name = consume_type(TT_IDENTIFIER, "Expected function name")->value;
    node->fn.params = NULL;
    node->fn.num_params = 0;
    node->fn.is_generator = is_generator;

    parser_trace(node->fn.name);
    consume("(", TT_BRACKET, error_withloc("Expected '(' for function parameters"));

    int param_capacity = PARSER_MIN_NODE_CAPACITY;
    node->fn.params = (typeof(node->fn.params))dou_new(param_capacity * sizeof(*node->fn.params));
    /// @todo  : just count and then know!

    while (peek() && !cstr_eq(peek()->value, ")")) {
        Token *param_token = consume_type(TT_IDENTIFIER, error_withloc("Expected parameter name"));
        parser_trace(param_token->value);

        const char *param_name = param_token->value;

        node->fn.params[node->fn.num_params].param_name = cstr_maybe_dup(param_name);
        node->fn.params[node->fn.num_params].tags = NULL;
        node->fn.params[node->fn.num_params].num_tags = 0;

        node->fn.num_params++;

        if (peek() && cstr_eq(peek()->value, ",")) {
            consume(",", TT_SEP, NULL);
        }
    }

    consume(")", TT_BRACKET, error_withloc("Expected ')' after function parameters"));

    Token *assign_token = consume_type(TT_ASSIGN, error_withloc("Expected '=' or ':=' for function body"));

    node->fn.body = cstr_eq(assign_token->value, ":=")
                        ? parser_block(parser)
                        : parser_expression(parser, false);

    return node;
}

AstNode *parser_anonymous_function(Parser *parser) {
    parser_trace("parser");

    consume("fn", TT_KEYWORD, NULL);
    consume("(", TT_BRACKET, error_withloc("Expected '(' for anonymous function parameters"));

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_FN_ANON;

    int num_params = 0;
    int param_capacity = PARSER_MIN_NODE_CAPACITY;
    node->fn_anon.params = (typeof(node->fn_anon.params))dou_new(param_capacity * sizeof(*node->fn_anon.params));

    while (peek() && !cstr_eq(peek()->value, ")")) {
        if (num_params >= param_capacity) {
            /// @todo does it make sense to realloc at all?
            node->fn_anon.params = (typeof(node->fn_anon.params))dou_realloc(node->fn_anon.params,
                                                                             param_capacity * sizeof(*node->fn_anon.params),
                                                                             param_capacity * 2);

            param_capacity *= 2;
        }

        Token *param_token = consume_type(TT_IDENTIFIER, error_withloc("Expected parameter name"));
        const char *param_name = param_token->value;
        parser_trace(param_name);
        node->fn_anon.params[num_params].param_name = cstr_maybe_dup(param_name);
        node->fn_anon.params[num_params].tags = NULL;
        node->fn_anon.params[num_params].num_tags = 0;

        //     int dov_tag_capacity = PARSER_MIN_NODE_CAPACITY;
        //     int num_tags = 0;
        //     node->fn_anon.params[num_params].tags = (char **)dou_new(dov_tag_capacity * sizeof(char *));
        //     node->fn_anon.params[num_params].num_tags = num_tags;
        // }

        num_params++;

        if (peek() && cstr_eq(peek()->value, ",")) {
            consume(",", TT_SEP, NULL);
        }
    }

    consume(")", TT_BRACKET, error_withloc("Expected ')' after anonymous function parameters"));

    consume("->", TT_ASSIGN, error_withloc("Expected '->' for anonymous function body"));

    AstNode *body = parser_expression(parser, 0);
    node->fn.body = body;
    node->fn.is_generator = false;

    return node;
}

AstNode *parser_mutation(Parser *parser) {
    Token *token = consume_type(TT_KEYWORD, NULL);
    bool is_broadcast = false;

    parser_trace(token->value);

    if (cstr_eq(token->value, "mut*")) {
        is_broadcast = true;
    } else if (!cstr_eq(token->value, "mut")) {
        parser_error(parser, token, error_withloc("Expected 'mut' or 'mut*'"));
    }

    AstNode *target = parser_expression(parser, 0);

    consume("=", TT_ASSIGN, error_withloc("Expected '=' after 'mut' target"));

    AstNode *value = parser_expression(parser, 0);

    AstNode *node = (AstNode *)dou_new(sizeof(AstNode));
    node->type = AST_MUTATION;
    node->mutation.target = target;
    node->mutation.value = value;
    node->mutation.is_broadcast = is_broadcast;

    return node;
}

#pragma endregion




#pragma region InterpreterDoHeader

typedef struct Box {
    uint64_t payload : 61;  // Payload holds the pointer (eg., Do *)
    uint8_t type : 3;       // Type field (3 bits allow up to 8 types)
} Box;


typedef enum DType {
    // Unboxed types
    DTYPE_NULL = 0,
    DTYPE_INT = 1,
    DTYPE_UINT = 2,
    DTYPE_BOOL = 3,
    DTYPE_DOUBLE = 4,
    DTYPE_TAG = 5,
    DTYPE_OBJECT = 6,
    DTYPE_UNDEF = 7,

    // Boxed types
    DTYPE_STRING = 8,
    DTYPE_DICT,
    DTYPE_SCOPE,
    DTYPE_VEC,
    DTYPE_SET,
    DTYPE_STRUCT,
    DTYPE_STRUCT_INSTANCE,
    DTYPE_FN,
    DTYPE_NATIVE_FN,
    DTYPE_GENERATOR,
    DTYPE_NUM_TYPES // Ensure this reflects the total number of types
} DType;

const char *DTypeStr[DTYPE_NUM_TYPES] = {
    [DTYPE_NULL] = "null",
    [DTYPE_INT] = "int",
    [DTYPE_UINT] = "uint",
    [DTYPE_BOOL] = "bool",
    [DTYPE_DOUBLE] = "double",
    [DTYPE_TAG] = "tag",
    [DTYPE_OBJECT] = "object",
    [DTYPE_UNDEF] = "undefined",
    [DTYPE_STRING] = "string",
    [DTYPE_DICT] = "dict",
    [DTYPE_SCOPE] = "scope",
    [DTYPE_VEC] = "vec",
    [DTYPE_SET] = "set",
    [DTYPE_STRUCT] = "struct",
    [DTYPE_STRUCT_INSTANCE] = "struct_instance",
    [DTYPE_FN] = "fn",
    [DTYPE_NATIVE_FN] = "native_fn",
    [DTYPE_GENERATOR] = "generator"
};

// typedef struct Iterable {
//     unsigned int length;
//     void *payload;
// } Iterable;



typedef struct String {
    unsigned int length;
    const char *cstr;
    unsigned int capacity;
    void *allocator;
} String;

typedef struct Tag {
    int length;
    char value[128];
} Tag;

typedef struct KeyValue {
    Box key;
    Box value;
} KeyValue;

typedef struct DictEntry {
    KeyValue kv;
    struct DictEntry *next;
} DictEntry;

typedef struct Dict {
    unsigned int size;
    DictEntry **buckets;
    unsigned int capacity;
    void *allocator;
} Dict;

typedef struct Vec {
    unsigned int size;
    Box *items;
    void *allocator;
    unsigned int capacity;
} Vec;

typedef struct Set {
    unsigned int size;
    Box *items;
    unsigned int capacity;
    void *allocator;
} Set;

typedef struct Struct {
    void *allocator;
    char *name;
    struct StructFieldDef {
        char *name;
        int is_static;
        char *type;
    } *fields;
    int num_fields;
} Struct;  

typedef struct Obj {
    void *allocator;
    Struct *struct_def;
    Box fields;
} Obj;  

typedef struct Scope {
    void *allocator;
    Box parent_scope;
    Box local_scope_dict;
} Scope;

typedef struct Fn {
    void *allocator;
    AstNode *ast;
    const char *name;
    Box closure_scope;
    Box (*fn_ptr)(Box *args, int num_args);
} Fn;

typedef struct Gn {
    void *allocator;
    unsigned int state;
    Fn *generator_fn;
    Box local_scope; 
    int num_locals;
} Gn;


#define box_is_type(ptr, t) (t == DTYPE_OBJECT ?  (box_as_do(ptr)->type == t) : ((ptr).type == (t)))
#define box_is_notnull(ptr) ((ptr).type != DTYPE_NULL)
#define box_is_null(ptr) ((ptr).type == DTYPE_NULL)

#define box_payload(ptr) ((ptr).payload)
#define box_literal(type_pattern, value) ((Box){.type = (type_pattern), .payload = (uint64_t)(value)})

#define box_from_null() box_literal(DTYPE_NULL, 0)
#define box_from_int(value) box_literal(DTYPE_INT, (value))
#define box_from_bool(value) box_literal(DTYPE_BOOL, (value))
#define box_from_tag(value) box_literal(DTYPE_TAG, (value))
#define box_from_do(ptr_to_do) box_literal(DTYPE_OBJECT, (ptr_to_do))

#define box_as_int(ptr) ((int64_t)box_payload(ptr))
#define box_as_double(ptr) (*(double *)box_payload(ptr))
#define box_as_do(ptr) (*(Do *)box_payload(ptr))
#define box_as_bool(ptr) ((bool)box_payload(ptr))
#define box_as_tag(ptr) ((int64_t)(box_payload(ptr)))

#define box_print_literal(ptr) \
    printf("box(Type: %d, Payload: %lu)\n", (ptr).type, box_payload(ptr))

const char *box_type_to_cstr(Box box);




Box _glo_box_null = box_from_null();
Box _glo_box_true = box_from_bool(true);
Box _glo_box_false = box_from_bool(false);
Box _glo_box_undef = box_literal(DTYPE_UNDEF, 1);


typedef struct Do {
    DType type;
    bool is_managed: 1;

    union {
        String string;
        Dict dict;
        Scope scope;
        Struct structure;
        Obj instance;
        Vec vec;
        Set set;
        Fn fn;
        Gn gen;
    } as;
} Do;

typedef struct DictIterator {
    DictEntry *current;
    int bucket_index;
} DictIterator;

typedef struct VecIterator {
    int current_index;
    int size;
    Vec *vec;
} VecIterator;

typedef struct SetIterator {
    int current_index;
    int size;
    Set *set;
} SetIterator;

SetIterator box_set_iterator(Box set);
bool box_set_iterator_next(SetIterator *iter, Box *value);


VecIterator box_vec_iterator(Box vec);
bool box_vec_iterator_next(VecIterator *iter, Box *value);


DictIterator box_dict_iterator(Box dict);
bool box_dict_iterator_next(DictIterator *iter, Box *key, Box *value);


// look at _try fns
// #define box_force_double(box) (box_is_type(box, DTYPE_DOUBLE) ? box_as_double(box) : (double)box_as_int(box))

#define boxed(value) (_glo_box_value)


const char *box_to_cstr(Box res, char *buf, size_t buf_size);
bool box_dict_to_cstr(Box dict, char *buf, size_t buf_size, size_t *offset);
bool box_scope_to_cstr(Box dict, char *buf, size_t buf_size, size_t *offset);
bool box_vec_to_cstr(Box vec, char *buf, size_t buf_size, size_t *offset);
bool box_set_to_cstr(Box set, char *buf, size_t buf_size, size_t *offset);
bool box_struct_to_cstr(Box *struct_def, char *buf, size_t buf_size, size_t *offset);
bool box_object_to_cstr(Box object, char *buf, size_t buf_size, size_t *offset);
bool box_fn_to_cstr(Box fn, char *buf, size_t buf_size, size_t *offset);
bool box_gn_to_cstr(Box gen, char *buf, size_t buf_size, size_t *offset);

#define cstr_buf_append(buf, size, offset, fmt, ...)                                         \
    if ((*offset) < (size)) {                                                                \
        int written = snprintf((buf) + (*offset), (size) - (*offset), (fmt), ##__VA_ARGS__); \
        if (written < 0 || written >= (size) - (*offset)) {                                  \
            return NULL;                                                                     \
        } else {                                                                             \
            (*offset) += written;                                                            \
        }                                                                                    \
    } else                                                                                   \
        return NULL



Box box_copy(Box original);
Box box_set_copy(Box original);
Box box_scope_copy(Box original); 
Box box_str_copy(Box original); 
Box box_vec_copy(Box original);
Box box_dict_copy(Box original);
Box box_set_copy(Box original);
Box box_gn_copy(Box original);
Box box_object_copy(Box original);
Box box_double_copy(Box original);

#define box_int_copy(i) i 
#define box_bool_copy(i) i 
#define box_tag_copy(i) i 


Box box_str_new(const char *str);
Box box_tag_new(const char *name);
Box box_dict_new(unsigned int capacity);
Box box_vec_new(unsigned int capacity);
Box box_double_new(double);
Box box_int_new(long);
Box box_bool_new(bool);
Box box_scope_new(Box parent_scope);
Box box_fn_new(const char *name, Fn fn);
Box box_gn_new(Gn gn);
Box box_native_fn_new(const char *name, Box (*fn_ptr)(Box *args, unsigned int num_args));
Box box_struct_new(Struct def);
Box box_object_new(Struct def, Box *fields, unsigned int num_fields);

void box_free(Box box);
void box_print(Box result);

unsigned int box_hash(Box key);
Box box_by_inference(Box a, Box b);
bool box_is_truthy(Box value);
bool box_is_eq(Box a, Box b);
int box_cmp(Box a, Box b);
int box_cmp_nat(Box a, Box b);
int box_quicksort_partition(Box *items, int left, int right, int (*compare)(Box, Box));
void box_quicksort(Box *items, int left, int right, int (*compare)(Box, Box));


typedef enum BoxError {
    BOX_SUCCESS = 0,
    BOX_ERROR_INVALID_TYPE,
    BOX_ERROR_OUT_OF_MEMORY,
} BoxError;

BoxError box_dict_set(Box dict, Box key, Box value);

Box box_dict_get(Box dict, Box key);
Box box_dict_get_default(Box dict, Box key, Box default_value);
Box box_dict_has_as_bool(Box dict, Box key);
void box_dict_extend(Box box_dict_left, Box box_dict_right);
Box box_dict_keys_as_vec(Box dict);
Box box_dict_as_box_vec_pairs(Box dict);

Box box_vec_get(Box vec, Box index);
Box box_vec_get_default(Box vec, Box index, Box default_value);
Box box_vec_has_as_bool(Box vec, Box value);
BoxError box_vec_set(Box vec, Box index, Box value);
void box_vec_append(Box vec, Box value);
void box_vec_prepend(Box vec, Box value);
void box_vec_insert_at(Box vec, Box value, unsigned int index);
void box_vec_sort_quicksort(Box vec);
void box_vec_sort_naturalsort(Box vec);
void box_vec_reverse(Box vec);
void box_vec_extend(Box box_vec_left, Box box_vec_right);

Box box_vec_as_set(Box vec);
Box box_vec_values_as_vec(Box vec);
Box box_vec_local_scope_dict(Box vec);

Box box_scope_keys(Box scope);
Box box_scope_has_as_bool(Box scope, Box identifier);

unsigned int box_hash_int(Box box);
unsigned int box_hash_double(Box box);
unsigned int box_hash_tag(Box box);
unsigned int box_hash_string(Box box);

bool box_is_less_than(Box a, Box b);
bool box_is_greater_than(Box a, Box b);


bool box_try_cast_to_int(Box box, int64_t *out);
bool box_try_cast_to_double(Box box, double *out);
bool box_try_cast_to_string(Box box, String **out);

#pragma endregion


#pragma region InterpreterApiHeader
// Error handling
void interp_error(const char *message, const char *more_info);

// Running code
Box interp_run_from_source(const char *source, const char *indent);
Box interp_run(AstNode *ast, Box globals);

// Evaluation functions
Box interp_eval_fn(Box fn_obj, int num_args, Box *args);
Box interp_eval_uop(const char *op, Box operand);
Box interp_eval_bop(const char *op, Box left, Box right);
void interp_eval_def(const char *name, Box value, Box scope);
bool interp_eval_lookup(const char *name, Box scope, Box *out_value);
Box interp_eval_ast(AstNode *node, Box scope);

// AST node evaluation functions
Box interp_box_if(AstNode *node, Box scope);
Box interp_box_dict(AstNode *node, Box scope);
Box interp_box_vec(AstNode *node, Box scope);
Box interp_box_method_call(AstNode *node, Box scope);
Box interp_box_member_access(AstNode *node, Box scope);
Box interp_box_literal(AstNode *node, Box scope);
Box interp_box_identifier(AstNode *node, Box scope);
Box interp_box_module(AstNode *node, Box scope);
Box interp_box_call_fn(AstNode *node, Box scope);
Box interp_box_block(AstNode *node, Box scope);
Box interp_box_fn_def(AstNode *node, Box scope);
Box interp_box_loop(AstNode *node, Box scope);
Box interp_box_for(AstNode *node, Box scope);
Box interp_box_expression(AstNode *node, Box scope);
Box interp_box_fn_anon(AstNode *node, Box scope);
Box interp_box_const_def(AstNode *node, Box scope);
Box interp_box_let_def(AstNode *node, Box scope);
Box interp_box_binary_op(AstNode *node, Box scope);
Box interp_box_unary_op(AstNode *node, Box scope);
Box interp_box_use(AstNode *node, Box scope);
Box interp_box_struct(AstNode *node, Box scope);
Box interp_box_object_literal(AstNode *node, Box scope);
Box interp_box_match(AstNode *node, Box scope);
Box interp_box_return(AstNode *node, Box scope);
Box interp_box_mutation(AstNode *node, Box scope);
#pragma endregion


#pragma region InterpreterDoImpl
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Assume the new header code is already included here.

// Helper function to allocate memory and handle errors.
void *interp_new(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Helper function to free allocated memory.
void interp_free(void *ptr) {
    free(ptr);
}

// Helper function to copy strings safely.
char *cstr_ndup(const char *str, size_t n) {
    char *copy = (char*)malloc(n + 1);
    if (!copy) {
        fprintf(stderr, "String duplication failed\n");
        exit(EXIT_FAILURE);
    }
    strncpy(copy, str, n);
    copy[n] = '\0';
    return copy;
}

// Implementations of the new prototypes:

// Function to convert Box type to string.
const char *box_type_to_cstr(Box ptr) {
    if (ptr.type < DTYPE_NUM_TYPES) {
        return DTypeStr[ptr.type];
    }
    return "<unknown type>";
}

// Implementations of iterator functions.
SetIterator box_set_iterator(Box set) {
    SetIterator iter = {0, 0, NULL};
    if (!box_is_type(set, DTYPE_SET)) {
        fprintf(stderr, "box_set_iterator: Expected DTYPE_SET\n");
        return iter;
    }
    Do *do_ptr = box_as_do(set);
    iter.current_index = 0;
    iter.size = do_ptr->as.set.size;
    iter.set = &do_ptr->as.set;
    return iter;
}

bool box_set_iterator_next(SetIterator *iter, Box *value) {
    if (iter->current_index >= iter->size) {
        return false;
    }
    *value = iter->set->items[iter->current_index++];
    return true;
}

VecIterator box_vec_iterator(Box vec) {
    VecIterator iter = {0, 0, NULL};
    if (!box_is_type(vec, DTYPE_VEC)) {
        fprintf(stderr, "box_vec_iterator: Expected DTYPE_VEC\n");
        return iter;
    }
    Do *do_ptr = box_as_do(vec);
    iter.current_index = 0;
    iter.size = do_ptr->as.vec.size;
    iter.vec = &do_ptr->as.vec;
    return iter;
}

bool box_vec_iterator_next(VecIterator *iter, Box *value) {
    if (iter->current_index >= iter->size) {
        return false;
    }
    *value = iter->vec->items[iter->current_index++];
    return true;
}

DictIterator box_dict_iterator(Box dict) {
    DictIterator iter = {0, -1};
    if (!box_is_type(dict, DTYPE_DICT)) {
        fprintf(stderr, "box_dict_iterator: Expected DTYPE_DICT\n");
        return iter;
    }
    iter.current = NULL;
    iter.bucket_index = -1;
    return iter;
}

bool box_dict_iterator_next(DictIterator *iter, Box *key, Box *value) {
    Dict *dict = NULL;
    if (!iter->current) {
        // First call to next, initialize
        dict = &box_as_do(iter->current->kv.key)->as.dict;
        iter->bucket_index = 0;
        iter->current = NULL;
    }
    while (iter->bucket_index < dict->capacity) {
        if (!iter->current) {
            iter->current = dict->buckets[iter->bucket_index++];
        } else {
            iter->current = iter->current->next;
        }
        if (iter->current) {
            *key = iter->current->kv.key;
            *value = iter->current->kv.value;
            return true;
        }
    }
    return false;
}

// Function to copy a Box value.
Box box_copy(Box original) {
    if (box_is_null(original)) {
        return _glo_box_undef;
    }

    if (box_is_type(original, DTYPE_INT) || box_is_type(original, DTYPE_UINT) ||
        box_is_type(original, DTYPE_BOOL) || box_is_type(original, DTYPE_TAG) ||
        box_is_type(original, DTYPE_DOUBLE)) {
        // For immediate types, copy directly
        return original;
    }

    switch (original.type) {
        case DTYPE_STRING:
            return box_str_copy(original);
        case DTYPE_DICT:
            return box_dict_copy(original);
        case DTYPE_SCOPE:
            return box_scope_copy(original);
        case DTYPE_VEC:
            return box_vec_copy(original);
        case DTYPE_SET:
            return box_set_copy(original);
        case DTYPE_STRUCT_INSTANCE:
            return box_object_copy(original);
        case DTYPE_FN:
        case DTYPE_NATIVE_FN:
            // For functions, shallow copy
            return original;
        case DTYPE_GENERATOR:
            return box_gn_copy(original);
        default:
            fprintf(stderr, "box_copy: Unknown Box type encountered during copy\n");
            return _glo_box_undef;
    }
}

// Function to copy a string Box.
Box box_str_copy(Box original) {
    if (!box_is_type(original, DTYPE_STRING)) {
        fprintf(stderr, "box_str_copy: Expected DTYPE_STRING\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    String *original_str = &original_do->as.string;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_STRING;
    copy_do->is_managed = false;

    String *copy_str = &copy_do->as.string;
    copy_str->allocator = original_str->allocator;
    copy_str->capacity = original_str->capacity;
    copy_str->length = original_str->length;
    copy_str->cstr = interp_new(copy_str->capacity);
    memcpy(copy_str->cstr, original_str->cstr, copy_str->length + 1);

    return box_from_do(copy_do);
}

// Function to copy a vector Box.
Box box_vec_copy(Box original) {
    if (!box_is_type(original, DTYPE_VEC)) {
        fprintf(stderr, "box_vec_copy: Expected DTYPE_VEC\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Vec *original_vec = &original_do->as.vec;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_VEC;
    copy_do->is_managed = false;

    Vec *copy_vec = &copy_do->as.vec;
    copy_vec->allocator = original_vec->allocator;
    copy_vec->capacity = original_vec->capacity;
    copy_vec->size = original_vec->size;
    copy_vec->items = interp_new(sizeof(Box) * copy_vec->capacity);

    for (unsigned int i = 0; i < copy_vec->size; ++i) {
        copy_vec->items[i] = box_copy(original_vec->items[i]);
    }
    return box_from_do(copy_do);
}

// Function to copy a dictionary Box.
Box box_dict_copy(Box original) {
    if (!box_is_type(original, DTYPE_DICT)) {
        fprintf(stderr, "box_dict_copy: Expected DTYPE_DICT\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Dict *original_dict = &original_do->as.dict;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_DICT;
    copy_do->is_managed = false;

    Dict *copy_dict = &copy_do->as.dict;
    copy_dict->allocator = original_dict->allocator;
    copy_dict->capacity = original_dict->capacity;
    copy_dict->size = 0;
    copy_dict->buckets = interp_new(sizeof(DictEntry*) * copy_dict->capacity);
    memset(copy_dict->buckets, 0, sizeof(DictEntry*) * copy_dict->capacity);

    for (unsigned int i = 0; i < original_dict->capacity; ++i) {
        DictEntry *entry = original_dict->buckets[i];
        while (entry) {
            DictEntry *new_entry = interp_new(sizeof(DictEntry));
            new_entry->kv.key = box_copy(entry->kv.key);
            new_entry->kv.value = box_copy(entry->kv.value);
            unsigned int hash = box_hash(new_entry->kv.key);
            unsigned int index = hash % copy_dict->capacity;
            new_entry->next = copy_dict->buckets[index];
            copy_dict->buckets[index] = new_entry;
            copy_dict->size += 1;
            entry = entry->next;
        }
    }
    return box_from_do(copy_do);
}

// Function to copy a scope Box.
Box box_scope_copy(Box original) {
    if (!box_is_type(original, DTYPE_SCOPE)) {
        fprintf(stderr, "box_scope_copy: Expected DTYPE_SCOPE\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Scope *original_scope = &original_do->as.scope;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_SCOPE;
    copy_do->is_managed = false;

    Scope *copy_scope = &copy_do->as.scope;
    copy_scope->allocator = original_scope->allocator;
    copy_scope->local_scope_dict = box_dict_copy(original_scope->local_scope_dict);
    copy_scope->parent_scope = original_scope->parent_scope;

    return box_from_do(copy_do);
}

// Function to copy a set Box.
Box box_set_copy(Box original) {
    if (!box_is_type(original, DTYPE_SET)) {
        fprintf(stderr, "box_set_copy: Expected DTYPE_SET\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Set *original_set = &original_do->as.set;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_SET;
    copy_do->is_managed = false;

    Set *copy_set = &copy_do->as.set;
    copy_set->allocator = original_set->allocator;
    copy_set->capacity = original_set->capacity;
    copy_set->size = original_set->size;
    copy_set->items = interp_new(sizeof(Box) * copy_set->capacity);

    for (unsigned int i = 0; i < copy_set->size; ++i) {
        copy_set->items[i] = box_copy(original_set->items[i]);
    }
    return box_from_do(copy_do);
}

// Function to copy a generator Box.
Box box_gn_copy(Box original) {
    if (!box_is_type(original, DTYPE_GENERATOR)) {
        fprintf(stderr, "box_gn_copy: Expected DTYPE_GENERATOR\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Gn *original_gn = &original_do->as.gen;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_GENERATOR;
    copy_do->is_managed = false;

    Gn *copy_gn = &copy_do->as.gen;
    copy_gn->allocator = original_gn->allocator;
    copy_gn->state = original_gn->state;
    copy_gn->generator_fn = original_gn->generator_fn; // Shallow copy
    copy_gn->local_scope = original_gn->local_scope;             // Shallow copy
    copy_gn->num_locals = original_gn->num_locals;

    return box_from_do(copy_do);
}

// Function to copy an object Box.
Box box_object_copy(Box original) {
    if (!box_is_type(original, DTYPE_STRUCT_INSTANCE)) {
        fprintf(stderr, "box_object_copy: Expected DTYPE_STRUCT_INSTANCE\n");
        return _glo_box_undef;
    }

    Do *original_do = box_as_do(original);
    Obj *original_obj = &original_do->as.instance;

    Do *copy_do = interp_new(sizeof(Do));
    copy_do->type = DTYPE_STRUCT_INSTANCE;
    copy_do->is_managed = false;

    Obj *copy_obj = &copy_do->as.instance;
    copy_obj->allocator = original_obj->allocator;
    copy_obj->struct_def = original_obj->struct_def; // Assuming structs are immutable
    copy_obj->fields = box_copy(original_obj->fields);

    return box_from_do(copy_do);
}

// Function to copy a double Box.
Box box_double_copy(Box original) {
    if (!box_is_type(original, DTYPE_DOUBLE)) {
        fprintf(stderr, "box_double_copy: Expected DTYPE_DOUBLE\n");
        return _glo_box_undef;
    }
    return original; // Doubles are immediate types in this implementation
}

// Function to create a new string Box.
Box box_str_new(const char *str) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_STRING;
    do_ptr->is_managed = false;

    String *string = &do_ptr->as.string;
    string->length = strlen(str);
    string->capacity = string->length + 1;
    string->cstr = cstr_ndup(str, string->length);
    string->allocator = NULL; // Set appropriate allocator

    return box_from_do(do_ptr);
}

// Function to create a new tag Box.
Box box_tag_new(const char *name) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_TAG;
    do_ptr->is_managed = false;

    Tag *tag = &do_ptr->as.tag;
    tag->length = strlen(name);
    strncpy(tag->value, name, sizeof(tag->value));
    tag->value[sizeof(tag->value) - 1] = '\0';

    return box_from_do(do_ptr);
}

// Helper function to create a new Dict.
Dict *box_dict_create(unsigned int capacity) {
    Dict *dict = interp_new(sizeof(Dict));
    dict->allocator = NULL; // Set appropriate allocator
    dict->capacity = capacity > 0 ? capacity : 8;
    dict->size = 0;
    dict->buckets = interp_new(sizeof(DictEntry*) * dict->capacity);
    memset(dict->buckets, 0, sizeof(DictEntry*) * dict->capacity);
    return dict;
}

// Function to create a new dictionary Box.
Box box_dict_new(unsigned int capacity) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_DICT;
    do_ptr->is_managed = false;

    Dict *dict = &do_ptr->as.dict;
    *dict = *box_dict_create(capacity);

    return box_from_do(do_ptr);
}


Box box_gn_new(Gn gn) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_GENERATOR;
    do_ptr->is_managed = false;

    do_ptr->as.gen = gn;

    return box_from_do(do_ptr);
}


// Function to create a new vector Box.
Box box_vec_new(unsigned int capacity) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_VEC;
    do_ptr->is_managed = false;

    Vec *vec = &do_ptr->as.vec;
    vec->allocator = NULL; // Set appropriate allocator
    vec->capacity = capacity > 0 ? capacity : 4;
    vec->size = 0;
    vec->items = interp_new(sizeof(Box) * vec->capacity);

    return box_from_do(do_ptr);
}

// Function to create a new double Box.
Box box_double_new(double value) {
    return box_literal(DTYPE_DOUBLE, *(uint64_t*)&value);
}

// Function to create a new integer Box.
Box box_int_new(long value) {
    return box_from_int(value);
}

// Function to create a new boolean Box.
Box box_bool_new(bool value) {
    return box_from_bool(value);
}

// Function to create a new scope Box.
Box box_scope_new(Box parent_scope) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_SCOPE;
    do_ptr->is_managed = false;

    Scope *scope = &do_ptr->as.scope;
    scope->allocator = NULL; // Set appropriate allocator
    scope->parent_scope = parent_scope;
    scope->local_scope_dict = box_dict_new(16);

    return box_from_do(do_ptr);
}

// Function to create a new function Box.
Box box_fn_new(const char *name, Fn fn) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_FN;
    do_ptr->is_managed = false;

    Fn *fn_ptr = &do_ptr->as.fn;
    *fn_ptr = fn;
    fn_ptr->name = name;

    return box_from_do(do_ptr);
}

// Function to create a new native function Box.
Box box_native_fn_new(const char *name, Box (*fn_ptr)(Box *args, unsigned int num_args)) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_NATIVE_FN;
    do_ptr->is_managed = false;

    Fn *fn = &do_ptr->as.fn;
    fn->allocator = NULL; // Set appropriate allocator
    fn->name = name;
    fn->fn_ptr = fn_ptr;

    return box_from_do(do_ptr);
}

// Function to create a new struct Box.
Box box_struct_new(Struct def) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_STRUCT;
    do_ptr->is_managed = false;

    Struct *struct_def = &do_ptr->as.structure;
    *struct_def = def;

    return box_from_do(do_ptr);
}

// Function to create a new object Box.
Box box_object_new(Struct def, Box *fields, unsigned int num_fields) {
    Do *do_ptr = interp_new(sizeof(Do));
    do_ptr->type = DTYPE_STRUCT_INSTANCE;
    do_ptr->is_managed = false;

    Obj *obj = &do_ptr->as.instance;
    obj->allocator = NULL; // Set appropriate allocator
    obj->struct_def = &def;

    // Create a fields dictionary
    obj->fields = box_dict_new(num_fields * 2);
    for (unsigned int i = 0; i < num_fields; ++i) {
        box_dict_set(obj->fields, box_from_int(i), fields[i]);
    }

    return box_from_do(do_ptr);
}

// Function to free a Box.
void box_free(Box box) {
    if (box_is_null(box) || box_is_type(box, DTYPE_INT) || box_is_type(box, DTYPE_UINT) ||
        box_is_type(box, DTYPE_BOOL) || box_is_type(box, DTYPE_TAG) ||
        box_is_type(box, DTYPE_DOUBLE)) {
        return;
    }

    Do *do_ptr = box_as_do(box);
    switch (do_ptr->type) {
        case DTYPE_STRING:
            interp_free(do_ptr->as.string.cstr);
            break;
        case DTYPE_VEC:
            for (unsigned int i = 0; i < do_ptr->as.vec.size; ++i) {
                box_free(do_ptr->as.vec.items[i]);
            }
            interp_free(do_ptr->as.vec.items);
            break;
        case DTYPE_DICT:
            // Free dictionary entries
            for (unsigned int i = 0; i < do_ptr->as.dict.capacity; ++i) {
                DictEntry *entry = do_ptr->as.dict.buckets[i];
                while (entry) {
                    box_free(entry->kv.key);
                    box_free(entry->kv.value);
                    DictEntry *next = entry->next;
                    interp_free(entry);
                    entry = next;
                }
            }
            interp_free(do_ptr->as.dict.buckets);
            break;
        // Add cases for other types as needed
        default:
            break;
    }
    interp_free(do_ptr);
}


// Function to get a value from a dictionary Box.
Box box_dict_get(Box dict, Box key) {
    if (!box_is_type(dict, DTYPE_DICT)) {
        fprintf(stderr, "box_dict_get: Expected DTYPE_DICT\n");
        return _glo_box_undef;
    }

    Do *dict_do = box_as_do(dict);
    Dict *dict_struct = &dict_do->as.dict;

    unsigned int hash = box_hash(key);
    unsigned int index = hash % dict_struct->capacity;
    DictEntry *entry = dict_struct->buckets[index];
    while (entry) {
        if (box_is_eq(entry->kv.key, key)) {
            return entry->kv.value;
        }
        entry = entry->next;
    }
    return _glo_box_undef;
}

// Function to set a value in a dictionary Box.
BoxError box_dict_set(Box dict, Box key, Box value) {
    if (!box_is_type(dict, DTYPE_DICT)) {
        fprintf(stderr, "box_dict_set: Expected DTYPE_DICT\n");
        return BOX_ERROR_INVALID_TYPE;
    }

    Do *dict_do = box_as_do(dict);
    Dict *dict_struct = &dict_do->as.dict;

    unsigned int hash = box_hash(key);
    unsigned int index = hash % dict_struct->capacity;

    DictEntry *entry = dict_struct->buckets[index];
    while (entry) {
        if (box_is_eq(entry->kv.key, key)) {
            entry->kv.value = value;
            return BOX_SUCCESS;
        }
        entry = entry->next;
    }

    DictEntry *new_entry = interp_new(sizeof(DictEntry));
    new_entry->kv.key = key;
    new_entry->kv.value = value;
    new_entry->next = dict_struct->buckets[index];
    dict_struct->buckets[index] = new_entry;
    dict_struct->size += 1;

    if ((float)dict_struct->size / dict_struct->capacity > 0.75f) {
        // Implement box_dict_resize to expand capacity
        // box_dict_resize(dict_struct, dict_struct->capacity * 2);
    }
    return BOX_SUCCESS;
}


Box box_by_inference(Box a, Box b) {
    if (box_is_type(a, DTYPE_DOUBLE) || box_is_type(b, DTYPE_DOUBLE)) {
        return box_from_double(0.0); // Return type with DTYPE_DOUBLE
    } else if (box_is_type(a, DTYPE_INT) && box_is_type(b, DTYPE_INT)) {
        return box_from_int(0); // Return type with DTYPE_INT
    } else {
        // Default to double for mixed types
        return box_from_double(0.0);
    }
}

bool box_is_truthy(Box value) {
    switch (value.type) {
        case DTYPE_NULL:
        case DTYPE_UNDEF:
            return false;
        case DTYPE_BOOL:
            return box_as_bool(value);
        case DTYPE_INT:
            return box_as_int(value) != 0;
        case DTYPE_DOUBLE:
            return box_as_double(value) != 0.0;
        case DTYPE_STRING: {
            Do *do_ptr = box_as_do(value);
            return do_ptr->as.string.length > 0;
        }
        case DTYPE_VEC:
        case DTYPE_DICT:
        case DTYPE_SET:
            // Assuming non-empty collections are truthy
            return true;
        default:
            return true;
    }
}


int box_cmp(Box a, Box b) {
    if (a.type != b.type) {
        return a.type - b.type;
    }

    switch (a.type) {
        case DTYPE_INT:
            return (box_as_int(a) > box_as_int(b)) - (box_as_int(a) < box_as_int(b));
        case DTYPE_DOUBLE:
            return (box_as_double(a) > box_as_double(b)) - (box_as_double(a) < box_as_double(b));
        case DTYPE_STRING: {
            Do *a_do = box_as_do(a);
            Do *b_do = box_as_do(b);
            return strcmp(a_do->as.string.cstr, b_do->as.string.cstr);
        }
        default:
            return 0;
    }
}

// Placeholder for natural sort comparison
int box_cmp_nat(Box a, Box b) {
    // Implement natural sort comparison if needed
    return box_cmp(a, b);
}


int box_quicksort_partition(Box *items, int left, int right, int (*compare)(Box, Box)) {
    Box pivot = items[right];
    int i = left - 1;
    for (int j = left; j < right; j++) {
        if (compare(items[j], pivot) <= 0) {
            i++;
            Box temp = items[i];
            items[i] = items[j];
            items[j] = temp;
        }
    }
    Box temp = items[i + 1];
    items[i + 1] = items[right];
    items[right] = temp;
    return i + 1;
}

void box_quicksort(Box *items, int left, int right, int (*compare)(Box, Box)) {
    if (left < right) {
        int pi = box_quicksort_partition(items, left, right, compare);
        box_quicksort(items, left, pi - 1, compare);
        box_quicksort(items, pi + 1, right, compare);
    }
}



// Implement the hash function for Box.
unsigned int box_hash(Box key) {
    unsigned int hash = 0;
    switch (key.type) {
        case DTYPE_INT:
            hash = (unsigned int)box_as_int(key);
            break;
        case DTYPE_DOUBLE: {
            double d = box_as_double(key);
            unsigned long long bits;
            memcpy(&bits, &d, sizeof(double));
            hash = (unsigned int)(bits ^ (bits >> 32));
            break;
        }
        case DTYPE_STRING: {
            Do *do_ptr = box_as_do(key);
            char *str = do_ptr->as.string.cstr;
            while (*str) {
                hash = (hash * 31) + *str++;
            }
            break;
        }
        case DTYPE_BOOL:
            hash = box_as_bool(key) ? 1 : 0;
            break;
        default:
            hash = 0;
            break;
    }
    return hash;
}

unsigned int box_hash_int(Box box) {
    return (unsigned int)box_as_int(box);
}

unsigned int box_hash_double(Box box) {
    double d = box_as_double(box);
    unsigned long long bits;
    memcpy(&bits, &d, sizeof(double));
    return (unsigned int)(bits ^ (bits >> 32));
}

unsigned int box_hash_tag(Box box) {
    return (unsigned int)box_as_tag(box);
}

unsigned int box_hash_string(Box box) {
    Do *do_ptr = box_as_do(box);
    char *str = do_ptr->as.string.cstr;
    unsigned int hash = 0;
    while (*str) {
        hash = (hash * 31) + *str++;
    }
    return hash;
}


// Implement comparison function for Box.
bool box_is_eq(Box a, Box b) {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case DTYPE_NULL:
        case DTYPE_UNDEF:
            return true;
        case DTYPE_INT:
            return box_as_int(a) == box_as_int(b);
        case DTYPE_DOUBLE:
            return box_as_double(a) == box_as_double(b);
        case DTYPE_STRING: {
            Do *a_do = box_as_do(a);
            Do *b_do = box_as_do(b);
            return strcmp(a_do->as.string.cstr, b_do->as.string.cstr) == 0;
        }
        case DTYPE_BOOL:
            return box_as_bool(a) == box_as_bool(b);
        default:
            return false;
    }
}

bool box_is_less_than(Box a, Box b) {
    return box_cmp(a, b) < 0;
}

bool box_is_greater_than(Box a, Box b) {
    return box_cmp(a, b) > 0;
}

bool box_try_cast_to_int(Box box, int64_t *out) {
    if (box_is_type(box, DTYPE_INT)) {
        *out = box_as_int(box);
        return true;
    } else if (box_is_type(box, DTYPE_DOUBLE)) {
        *out = (int64_t)box_as_double(box);
        return true;
    }
    return false;
}

bool box_try_cast_to_double(Box box, double *out) {
    if (box_is_type(box, DTYPE_DOUBLE)) {
        *out = box_as_double(box);
        return true;
    } else if (box_is_type(box, DTYPE_INT)) {
        *out = (double)box_as_int(box);
        return true;
    }
    return false;
}

bool box_try_cast_to_string(Box box, String** out) {
    if (box_is_type(box, DTYPE_STRING)) {
        Do *do_ptr = box_as_do(box);
        *out = &do_ptr->as.string;
        return true;
    }
    return false;
}



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Helper macro for appending to a buffer safely
#define cstr_buf_append(buf, size, offset, fmt, ...)                             \
    do {                                                                         \
        if (*(offset) < (size)) {                                                \
            int written = snprintf((buf) + *(offset), (size) - *(offset),        \
                                   (fmt), ##__VA_ARGS__);                        \
            if (written < 0 || (size_t)written >= (size) - *(offset)) {          \
                return false;                                                    \
            } else {                                                             \
                *(offset) += (size_t)written;                                    \
            }                                                                    \
        } else {                                                                 \
            return false;                                                        \
        }                                                                        \
    } while (0)


void box_print(Box result) {
    char buf[1024];
    box_to_cstr(result, buf, sizeof(buf));
    puts(buf);
}

// Function to convert a Box type to its string representation
const char *box_type_to_cstr(Box ptr) {
    if (ptr.type < DTYPE_NUM_TYPES) {
        return DTypeStr[ptr.type];
    }
    return "<unknown type>";
}

// Function to convert a Box to its string representation
const char *box_to_cstr(Box res, char *buf, size_t buf_size) {
    size_t offset = 0;  // Current position in the buffer

    if (box_is_null(res)) {
        cstr_buf_append(buf, buf_size, &offset, "null");
        return buf;
    }

    switch (res.type) {
        case DTYPE_INT:
            cstr_buf_append(buf, buf_size, &offset, "%ld", box_as_int(res));
            break;
        case DTYPE_UINT:
            cstr_buf_append(buf, buf_size, &offset, "%lu", (unsigned long)box_as_int(res));
            break;
        case DTYPE_BOOL:
            cstr_buf_append(buf, buf_size, &offset, "%s", box_as_bool(res) ? "true" : "false");
            break;
        case DTYPE_DOUBLE:
            cstr_buf_append(buf, buf_size, &offset, "%f", box_as_double(res));
            break;
        case DTYPE_TAG:
            cstr_buf_append(buf, buf_size, &offset, "<tag %ld>", box_as_tag(res));
            break;
        case DTYPE_STRING: {
            Do *do_ptr = box_as_do(res);
            cstr_buf_append(buf, buf_size, &offset, "\"%s\"", do_ptr->as.string.cstr);
            break;
        }
        case DTYPE_DICT:
            box_dict_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_SCOPE:
            box_scope_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_VEC:
            box_vec_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_SET:
            box_set_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_STRUCT:
            box_struct_to_cstr(&res, buf, buf_size, &offset);
            break;
        case DTYPE_STRUCT_INSTANCE:
            box_object_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_FN:
        case DTYPE_NATIVE_FN:
            box_fn_to_cstr(res, buf, buf_size, &offset);
            break;
        case DTYPE_GENERATOR:
            box_gn_to_cstr(res, buf, buf_size, &offset);
            break;
        default:
            cstr_buf_append(buf, buf_size, &offset, "<unknown type %d>", res.type);
            break;
    }

    // Ensure null-termination
    if (offset < buf_size) {
        buf[offset] = '\0';
    } else if (buf_size > 0) {
        buf[buf_size - 1] = '\0';
    }

    return buf;
}

// Function to convert a dictionary Box to its string representation
bool box_dict_to_cstr(Box dict_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(dict_box, DTYPE_DICT)) {
        cstr_buf_append(buf, buf_size, offset, "<not a dict>");
        return true;
    }

    cstr_buf_append(buf, buf_size, offset, "{");
    Do *do_ptr = box_as_do(dict_box);
    Dict *dict = &do_ptr->as.dict;
    bool first = true;
    for (unsigned int i = 0; i < dict->capacity; ++i) {
        DictEntry *entry = dict->buckets[i];
        while (entry) {
            if (!first) {
                cstr_buf_append(buf, buf_size, offset, ", ");
            }
            char key_buf[128];
            box_to_cstr(entry->kv.key, key_buf, sizeof(key_buf));
            char value_buf[128];
            box_to_cstr(entry->kv.value, value_buf, sizeof(value_buf));
            cstr_buf_append(buf, buf_size, offset, "%s: %s", key_buf, value_buf);
            first = false;
            entry = entry->next;
        }
    }
    cstr_buf_append(buf, buf_size, offset, "}");
    return true;
}

// Function to convert a vector Box to its string representation
bool box_vec_to_cstr(Box vec_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        cstr_buf_append(buf, buf_size, offset, "<not a vector>");
        return true;
    }

    cstr_buf_append(buf, buf_size, offset, "[");
    Do *do_ptr = box_as_do(vec_box);
    Vec *vec = &do_ptr->as.vec;
    for (unsigned int i = 0; i < vec->size; ++i) {
        if (i > 0) {
            cstr_buf_append(buf, buf_size, offset, ", ");
        }
        char item_buf[128];
        box_to_cstr(vec->items[i], item_buf, sizeof(item_buf));
        cstr_buf_append(buf, buf_size, offset, "%s", item_buf);
    }
    cstr_buf_append(buf, buf_size, offset, "]");
    return true;
}

// Function to convert a set Box to its string representation
bool box_set_to_cstr(Box set_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(set_box, DTYPE_SET)) {
        cstr_buf_append(buf, buf_size, offset, "<not a set>");
        return true;
    }

    cstr_buf_append(buf, buf_size, offset, "set{");
    Do *do_ptr = box_as_do(set_box);
    Set *set = &do_ptr->as.set;
    for (unsigned int i = 0; i < set->size; ++i) {
        if (i > 0) {
            cstr_buf_append(buf, buf_size, offset, ", ");
        }
        char item_buf[128];
        box_to_cstr(set->items[i], item_buf, sizeof(item_buf));
        cstr_buf_append(buf, buf_size, offset, "%s", item_buf);
    }
    cstr_buf_append(buf, buf_size, offset, "}");
    return true;
}

// Function to convert a scope Box to its string representation
bool box_scope_to_cstr(Box scope_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(scope_box, DTYPE_SCOPE)) {
        cstr_buf_append(buf, buf_size, offset, "<not a scope>");
        return true;
    }

    Do *do_ptr = box_as_do(scope_box);
    Scope *scope = &do_ptr->as.scope;
    cstr_buf_append(buf, buf_size, offset, "Scope(");
    box_dict_to_cstr(scope->local_scope_dict, buf, buf_size, offset);
    cstr_buf_append(buf, buf_size, offset, ")");
    return true;
}

// Function to convert a struct definition Box to its string representation
bool box_struct_to_cstr(Box *struct_def_box, char *buf, size_t buf_size, size_t *offset) {
    if (!struct_def_box || !box_is_type(*struct_def_box, DTYPE_STRUCT)) {
        cstr_buf_append(buf, buf_size, offset, "<not a struct>");
        return true;
    }

    Do *do_ptr = box_as_do(*struct_def_box);
    Struct *struct_def = &do_ptr->as.structure;
    cstr_buf_append(buf, buf_size, offset, "struct %s", struct_def->name ? struct_def->name : "<anonymous>");
    return true;
}

// Function to convert a struct instance Box (object) to its string representation
bool box_object_to_cstr(Box object_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(object_box, DTYPE_STRUCT_INSTANCE)) {
        cstr_buf_append(buf, buf_size, offset, "<not an object>");
        return true;
    }

    Do *do_ptr = box_as_do(object_box);
    Obj *obj = &do_ptr->as.instance;
    cstr_buf_append(buf, buf_size, offset, "Object of %s with fields ", obj->struct_def->name ? obj->struct_def->name : "<anonymous>");
    box_to_cstr(obj->fields, buf, buf_size, offset);
    return true;
}

// Function to convert a function Box to its string representation
bool box_fn_to_cstr(Box fn_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(fn_box, DTYPE_FN) && !box_is_type(fn_box, DTYPE_NATIVE_FN)) {
        cstr_buf_append(buf, buf_size, offset, "<not a function>");
        return true;
    }

    Do *do_ptr = box_as_do(fn_box);
    Fn *fn = &do_ptr->as.fn;
    cstr_buf_append(buf, buf_size, offset, "<function %s>", fn->name ? fn->name : "<anonymous>");
    return true;
}

// Function to convert a generator Box to its string representation
bool box_gn_to_cstr(Box gen_box, char *buf, size_t buf_size, size_t *offset) {
    if (!box_is_type(gen_box, DTYPE_GENERATOR)) {
        cstr_buf_append(buf, buf_size, offset, "<not a generator>");
        return true;
    }

    cstr_buf_append(buf, buf_size, offset, "<generator>");
    return true;
}


Box box_dict_get_default(Box dict, Box key, Box default_value) {
    Box value = box_dict_get(dict, key);
    if (box_is_null(value)) {
        return default_value;
    }
    return value;
}

Box box_dict_has_as_bool(Box dict, Box key) {
    Box value = box_dict_get(dict, key);
    return box_from_bool(!box_is_null(value));
}

void box_dict_extend(Box box_dict_left, Box box_dict_right) {
    if (!box_is_type(box_dict_left, DTYPE_DICT) || !box_is_type(box_dict_right, DTYPE_DICT)) {
        return;
    }

    Do *left_do = box_as_do(box_dict_left);
    Do *right_do = box_as_do(box_dict_right);

    Dict *left_dict = &left_do->as.dict;
    Dict *right_dict = &right_do->as.dict;

    for (unsigned int i = 0; i < right_dict->capacity; ++i) {
        DictEntry *entry = right_dict->buckets[i];
        while (entry) {
            box_dict_set(box_dict_left, entry->kv.key, entry->kv.value);
            entry = entry->next;
        }
    }
}

Box box_dict_keys_as_vec(Box dict_box) {
    if (!box_is_type(dict_box, DTYPE_DICT)) {
        return box_from_null();
    }

    Do *dict_do = box_as_do(dict_box);
    Dict *dict = &dict_do->as.dict;

    Box vec_box = box_vec_new(dict->size);
    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    for (unsigned int i = 0; i < dict->capacity; ++i) {
        DictEntry *entry = dict->buckets[i];
        while (entry) {
            box_vec_append(vec_box, entry->kv.key);
            entry = entry->next;
        }
    }
    return vec_box;
}

Box box_dict_as_box_vec_pairs(Box dict_box) {
    if (!box_is_type(dict_box, DTYPE_DICT)) {
        return box_from_null();
    }

    Do *dict_do = box_as_do(dict_box);
    Dict *dict = &dict_do->as.dict;

    Box vec_box = box_vec_new(dict->size);
    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    for (unsigned int i = 0; i < dict->capacity; ++i) {
        DictEntry *entry = dict->buckets[i];
        while (entry) {
            // Create a pair vector [key, value]
            Box pair_box = box_vec_new(2);
            box_vec_append(pair_box, entry->kv.key);
            box_vec_append(pair_box, entry->kv.value);
            box_vec_append(vec_box, pair_box);
            entry = entry->next;
        }
    }
    return vec_box;
}


Box box_vec_get(Box vec_box, Box index_box) {
    if (!box_is_type(vec_box, DTYPE_VEC) || !box_is_type(index_box, DTYPE_INT)) {
        return box_from_null();
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;
    int64_t index = box_as_int(index_box);

    if (index < 0 || index >= (int64_t)vec->size) {
        return box_from_null();
    }

    return vec->items[index];
}

Box box_vec_get_default(Box vec_box, Box index_box, Box default_value) {
    Box value = box_vec_get(vec_box, index_box);
    if (box_is_null(value)) {
        return default_value;
    }
    return value;
}

Box box_vec_has_as_bool(Box vec_box, Box value) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return box_from_bool(false);
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    for (unsigned int i = 0; i < vec->size; ++i) {
        if (box_is_eq(vec->items[i], value)) {
            return box_from_bool(true);
        }
    }
    return box_from_bool(false);
}

BoxError box_vec_set(Box vec_box, Box index_box, Box value) {
    if (!box_is_type(vec_box, DTYPE_VEC) || !box_is_type(index_box, DTYPE_INT)) {
        return BOX_ERROR_INVALID_TYPE;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;
    int64_t index = box_as_int(index_box);

    if (index < 0 || index >= (int64_t)vec->size) {
        return BOX_ERROR_INVALID_TYPE;
    }

    vec->items[index] = value;
    return BOX_SUCCESS;
}

void box_vec_append(Box vec_box, Box value) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    if (vec->size == vec->capacity) {
        vec->capacity = vec->capacity ? vec->capacity * 2 : 4;
        vec->items = realloc(vec->items, sizeof(Box) * vec->capacity);
    }

    vec->items[vec->size++] = value;
}

void box_vec_prepend(Box vec_box, Box value) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    if (vec->size == vec->capacity) {
        vec->capacity = vec->capacity ? vec->capacity * 2 : 4;
        vec->items = realloc(vec->items, sizeof(Box) * vec->capacity);
    }

    memmove(&vec->items[1], &vec->items[0], sizeof(Box) * vec->size);
    vec->items[0] = value;
    vec->size++;
}

void box_vec_insert_at(Box vec_box, Box value, unsigned int index) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    if (index > vec->size) {
        return;
    }

    if (vec->size == vec->capacity) {
        vec->capacity = vec->capacity ? vec->capacity * 2 : 4;
        vec->items = realloc(vec->items, sizeof(Box) * vec->capacity);
    }

    memmove(&vec->items[index + 1], &vec->items[index], sizeof(Box) * (vec->size - index));
    vec->items[index] = value;
    vec->size++;
}

void box_vec_sort_quicksort(Box vec_box) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    box_quicksort(vec->items, 0, vec->size - 1, box_cmp);
}

void box_vec_sort_naturalsort(Box vec_box) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    box_quicksort(vec->items, 0, vec->size - 1, box_cmp_nat);
}

void box_vec_reverse(Box vec_box) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return;
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    for (unsigned int i = 0; i < vec->size / 2; ++i) {
        Box temp = vec->items[i];
        vec->items[i] = vec->items[vec->size - 1 - i];
        vec->items[vec->size - 1 - i] = temp;
    }
}

void box_vec_extend(Box box_vec_left, Box box_vec_right) {
    if (!box_is_type(box_vec_left, DTYPE_VEC) || !box_is_type(box_vec_right, DTYPE_VEC)) {
        return;
    }

    Do *left_do = box_as_do(box_vec_left);
    Do *right_do = box_as_do(box_vec_right);

    Vec *left_vec = &left_do->as.vec;
    Vec *right_vec = &right_do->as.vec;

    for (unsigned int i = 0; i < right_vec->size; ++i) {
        box_vec_append(box_vec_left, right_vec->items[i]);
    }
}

Box box_vec_as_set(Box vec_box) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return box_from_null();
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    Box set_box = box_set_new(vec->size);
    Do *set_do = box_as_do(set_box);
    Set *set = &set_do->as.set;

    for (unsigned int i = 0; i < vec->size; ++i) {
        if (box_set_has_as_bool(set_box, vec->items[i]).payload == 0) {
            box_set_add(set_box, vec->items[i]);
        }
    }
    return set_box;
}

// Implement box_set_new, box_set_has_as_bool, and box_set_add as needed

Box box_vec_values_as_vec(Box vec_box) {
    // Assuming this function returns a copy of the vector
    return box_vec_copy(vec_box);
}

Box box_vec_local_scope_dict(Box vec_box) {
    if (!box_is_type(vec_box, DTYPE_VEC)) {
        return box_from_null();
    }

    Do *vec_do = box_as_do(vec_box);
    Vec *vec = &vec_do->as.vec;

    Box dict_box = box_dict_new(vec->size * 2);
    for (unsigned int i = 0; i < vec->size; ++i) {
        box_dict_set(dict_box, box_from_int(i), vec->items[i]);
    }
    return dict_box;
}


Box box_scope_keys(Box scope_box) {
    if (!box_is_type(scope_box, DTYPE_SCOPE)) {
        return box_from_null();
    }

    Do *scope_do = box_as_do(scope_box);
    Scope *scope = &scope_do->as.scope;

    return box_dict_keys_as_vec(scope->local_scope_dict);
}

Box box_scope_has_as_bool(Box scope_box, Box identifier) {
    if (!box_is_type(scope_box, DTYPE_SCOPE)) {
        return box_from_bool(false);
    }

    Do *scope_do = box_as_do(scope_box);
    Scope *scope = &scope_do->as.scope;

    return box_dict_has_as_bool(scope->local_scope_dict, identifier);
}



#pragma endregion


#pragma region InterpreterApiImpl

void interp_error(const char *message, const char *more_info) {
    // Modify interp_error to include line and col from AstNode
    fprintf(stderr, "%s (%s)\n", message, more_info);
    /// @todo: show some of the parser/lexer?
    /// @todo: include line numbers on AstNode & line in output
    /// @todo: make this variadic
    interp_tracer_print_stack();
    abort();
}

Box interp_run_from_source(const char *source, const char *indent) {
    // Adjust allocator setup as needed
    // dou_setup_allocators();

    ASSERT(&_glo_arena_internal != NULL, "Compiler arena out of memory");
    ASSERT(&_glo_arena_global != NULL, "Interpreter heap arena out of memory");
    ASSERT(&_glo_arena_local != NULL, "Interpreter temp arena out of memory");

    int out_num_lines = 0;
    const char **lines = (const char **)cstr_lines(source, &out_num_lines);

    Parser parser = {0};
    parser_ast_from_source(&parser, lines, out_num_lines, indent);

    return interp_run(parser.ast, interp_new_env());
}

// Execute the AST
Box interp_run(AstNode *ast, Box globals) {
    interp_trace("ast");

    for (int i = 0; i < ast->block.num_statements; i++) {
        Box result = interp_eval_ast(ast->block.statements[i], globals);

        if (!box_is_undef(result)) {
            LOG(LOG_LEVEL_ERROR, "Global statements should all be definitions (Statement ID: %d)", i);
        }
    }

    interp_native_minimal_api(globals);
    box_scope_print(globals);

    Box main_func;
    if (interp_eval_lookup("main", globals, &main_func)) {
        if (box_is_type(main_func, DTYPE_FN)) {
            Box result = interp_eval_fn(main_func, 0, NULL);

        } else {
            interp_error("The term `main` should refer to a function.", box_type_to_cstr(main_func));
        }
    } else {
        interp_error("The `main` function is not defined.", "<undefined>");
    }

    return globals;
}

Box interp_eval_fn(Box fn_obj, int num_args, Box *args) {
    if (box_is_type(fn_obj, DTYPE_NATIVE_FN)) {
        return box_as_fn(fn_obj)->fn_ptr(args, num_args);
    } else if (box_is_type(fn_obj, DTYPE_FN)) {
        Fn *function = &box_as_do(fn_obj)->as.fn;
        Box fn_scope = box_scope_new(function->closure_scope);

        AstNode *ast = function->ast;

        // Bind arguments
        for (int i = 0; i < ast->fn.num_params; i++) {
            const char *param_name = ast->fn.params[i].param_name;
            if (i < num_args && !box_is_undef(args[i])) {
                interp_eval_def(param_name, args[i], fn_scope);
            } else {
                interp_error("Runtime error: argument missing", param_name);
                interp_eval_def(param_name, __glo_box_undef, fn_scope);
            }
        }

        return interp_eval_ast(ast->fn.body, fn_scope);
    } else {
        interp_error("Runtime error: not a function", box_type_to_cstr(fn_obj));
        return __glo_box_undef;
    }
}

Box interp_eval_uop(const char *op, Box operand) {
    interp_trace(op);

    if (strcmp(op, "!") == 0) {
        return box_bool_new(!box_is_truthy(operand));
    } else if (strcmp(op, "-") == 0) {
        if (box_is_type(operand, DTYPE_INT)) {
            return box_int_new(-box_as_int(operand));
        } else if (box_is_type(operand, DTYPE_DOUBLE)) {
            return box_double_new(-box_as_double(operand));
        } else {
            interp_error("Runtime error: invalid operand for unary '-' op", box_type_to_cstr(operand));
            return __glo_box_undef;
        }
    } else {
        interp_error("Runtime error: unknown unary operator", op);
        return __glo_box_undef;
    }
}

Box interp_eval_bop(const char *op, Box left, Box right) {
    if (strcmp(op, "+") == 0) {
        if (box_is_type(left, DTYPE_INT) && box_is_type(right, DTYPE_INT)) {
            return box_int_new(box_as_int(left) + box_as_int(right));
        } else if (box_is_type(left, DTYPE_DOUBLE) || box_is_type(right, DTYPE_DOUBLE)) {
            double lval = box_force_double(left);
            double rval = box_force_double(right);
            return box_double_new(lval + rval);
        } else if (box_is_type(left, DTYPE_STRING) && box_is_type(right, DTYPE_STRING)) {
            Do *left_do = box_as_do(left);
            Do *right_do = box_as_do(right);
            size_t new_length = left_do->as.string.length + right_do->as.string.length;
            char *new_str = (char *)interp_new(new_length + 1);
            strcpy(new_str, left_do->as.string.cstr);
            strcat(new_str, right_do->as.string.cstr);
            Box result = box_str_new(new_str);
            interp_free(new_str);
            return result;
        } else {
            interp_error("Runtime error: invalid operands for '+'", "");
            return __glo_box_undef;
        }
    } else if (strcmp(op, "-") == 0) {
        if (box_is_type(left, DTYPE_INT) && box_is_type(right, DTYPE_INT)) {
            return box_int_new(box_as_int(left) - box_as_int(right));
        } else if (box_is_type(left, DTYPE_DOUBLE) || box_is_type(right, DTYPE_DOUBLE)) {
            double lval = box_force_double(left);
            double rval = box_force_double(right);
            return box_double_new(lval - rval);
        } else {
            interp_error("Runtime error: invalid operands for '-'", "");
            return __glo_box_undef;
        }
    } else if (strcmp(op, "*") == 0) {
        if (box_is_type(left, DTYPE_INT) && box_is_type(right, DTYPE_INT)) {
            return box_int_new(box_as_int(left) * box_as_int(right));
        } else if (box_is_type(left, DTYPE_DOUBLE) || box_is_type(right, DTYPE_DOUBLE)) {
            double lval = box_force_double(left);
            double rval = box_force_double(right);
            return box_double_new(lval * rval);
        } else {
            interp_error("Runtime error: invalid operands for '*'", "");
            return __glo_box_undef;
        }
    } else if (strcmp(op, "/") == 0) {
        double lval = box_force_double(left);
        double rval = box_force_double(right);
        if (rval != 0.0) {
            return box_double_new(lval / rval);
        } else {
            interp_error("Runtime error: division by zero", "");
            return __glo_box_undef;
        }
    } else if (strcmp(op, "==") == 0) {
        return box_bool_new(box_is_eq(left, right));
    } else if (strcmp(op, "!=") == 0) {
        return box_bool_new(!box_is_eq(left, right));
    } else if (strcmp(op, "<") == 0) {
        return box_bool_new(box_is_less_than(left, right));
    } else if (strcmp(op, ">") == 0) {
        return box_bool_new(box_is_greater_than(left, right));
    } else if (strcmp(op, "<=") == 0) {
        return box_bool_new(!box_is_greater_than(left, right));
    } else if (strcmp(op, ">=") == 0) {
        return box_bool_new(!box_is_less_than(left, right));
    } else if (strcmp(op, "&&") == 0) {
        return box_bool_new(box_is_truthy(left) && box_is_truthy(right));
    } else if (strcmp(op, "||") == 0) {
        return box_bool_new(box_is_truthy(left) || box_is_truthy(right));
    } else {
        interp_error("Runtime error: unknown operator", op);
        return __glo_box_undef;
    }
}

void interp_eval_def(const char *name, Box value, Box scope) {
    Box key = box_str_new(name);
    if (!box_dict_has_as_bool(box_get_scope_dict(scope), key).payload) {
        box_dict_set(box_get_scope_dict(scope), key, value);
    } else {
        interp_error("Variable already declared in this scope", name);
    }
}

bool interp_eval_lookup(const char *name, Box scope, Box *out_value) {
    interp_trace(name);

    Box key = box_str_new(name);
    while (!box_is_null(scope)) {
        Box dict = box_get_scope_dict(scope);
        Box value = box_dict_get(dict, key);
        if (!box_is_null(value)) {
            *out_value = value;
            return true;
        }
        scope = box_get_scope_parent(scope);
    }
    return false;
}

Box interp_eval_ast(AstNode *node, Box scope) {
    if (!node) return __glo_box_undef;

    switch (node->type) {
        case AST_ZERO:
            ASSERT(false, "AST Malformed (ZERO)");
            return __glo_box_undef;
        case AST_IF:
            return interp_box_if(node, scope);
        case AST_DICT:
            return interp_box_dict(node, scope);
        case AST_VEC:
            return interp_box_vec(node, scope);
        case AST_METHOD_CALL:
            return interp_box_method_call(node, scope);
        case AST_MEMBER_ACCESS:
            return interp_box_member_access(node, scope);
        case AST_TAG:
        case AST_NUM:
        case AST_STR:
            return interp_box_literal(node, scope);
        case AST_ID:
            return interp_box_identifier(node, scope);
        case AST_MOD:
            return interp_box_module(node, scope);
        case AST_CALL:
            return interp_box_call_fn(node, scope);
        case AST_BLOCK:
            return interp_box_block(node, scope);
        case AST_FN:
            return interp_box_fn_def(node, scope);
        case AST_LOOP:
            return interp_box_loop(node, scope);
        case AST_FOR:
            return interp_box_for(node, scope);
        case AST_EXP:
            return interp_box_expression(node, scope);
        case AST_FN_ANON:
            return interp_box_fn_anon(node, scope);
        case AST_CONST:
            return interp_box_const_def(node, scope);
        case AST_LET:
            return interp_box_let_def(node, scope);
        case AST_BOP:
            return interp_box_binary_op(node, scope);
        case AST_UOP:
            return interp_box_unary_op(node, scope);
        case AST_USE:
            return interp_box_use(node, scope);
        case AST_STRUCT:
            return interp_box_struct(node, scope);
        case AST_STRUCT_INIT:
            return interp_box_object_literal(node, scope);
        case AST_MATCH:
            return interp_box_match(node, scope);
        case AST_RETURN:
            return interp_box_return(node, scope);
        case AST_MUTATION:
            return interp_box_mutation(node, scope);
        default:
            LOG(LOG_LEVEL_ASSERT, "Unknown AST Node type (%d, %s)", node->type,
                node->type < AST_NUM_TYPES ? AstTypeStr[node->type] : "??");
            return __glo_box_undef;
    }
}

Box interp_box_if(AstNode *node, Box scope) {
    interp_trace("if");

    Box condition = interp_eval_ast(node->if_stmt.condition, scope);
    if (box_is_truthy(condition)) {
        return interp_eval_ast(node->if_stmt.body, scope);
    } else if (node->if_stmt.else_body) {
        return interp_eval_ast(node->if_stmt.else_body, scope);
    }
    return __glo_box_undef;
}

Box interp_box_literal(AstNode *node, Box scope) {
    switch (node->type) {
        case AST_NUM:
            return box_double_new(node->num.value);
        case AST_STR:
            return box_str_new(node->str.value);
        case AST_TAG:
            return box_tag_new(node->tag.name);
        default:
            interp_error("Unknown literal type", "");
            return __glo_box_undef;
    }
}

Box interp_box_identifier(AstNode *node, Box scope) {
    Box value;
    if (interp_eval_lookup(node->id.name, scope, &value)) {
        if (!box_is_null(value)) {
            return value;
        } else {
            interp_error("Lookup returned undefined for identifier", node->id.name);
            return __glo_box_undef;
        }
    } else {
        interp_error("Undefined identifier", node->id.name);
        return __glo_box_undef;
    }
}

Box interp_box_call_fn(AstNode *node, Box scope) {
    Box fn_obj = interp_eval_ast(node->call.callee, scope);
    if (!(box_is_type(fn_obj, DTYPE_FN) || box_is_type(fn_obj, DTYPE_NATIVE_FN))) {
        interp_error("Runtime error: not a function", box_type_to_cstr(fn_obj));
        return __glo_box_undef;
    }

    int num_args = node->call.num_args;
    Box *args = (Box *)interp_new(sizeof(Box) * num_args);
    for (int i = 0; i < num_args; i++) {
        args[i] = interp_eval_ast(node->call.args[i], scope);
    }

    Box result = interp_eval_fn(fn_obj, num_args, args);
    interp_free(args);
    return result;
}


Box interp_box_dict(AstNode *node, Box scope) {
    interp_trace("dict");

    Box dict_box = box_dict_new(16);
    for (int i = 0; i < node->dict.num_properties; i++) {
        const char *key_str = node->dict.properties[i].key;
        AstNode *value_node = node->dict.properties[i].value;

        Box key = box_str_new(key_str);
        Box value = interp_eval_ast(value_node, scope);

        box_dict_set(dict_box, key, value);
    }

    return dict_box;
}



Box interp_box_vec(AstNode *node, Box scope) {
    interp_trace("vector");

    int num_elements = node->vec.num_elements;
    Box vec_box = box_vec_new(num_elements);

    for (int i = 0; i < num_elements; i++) {
        AstNode *element_node = node->vec.elements[i];
        Box element = interp_eval_ast(element_node, scope);
        box_vec_append(vec_box, element);
    }

    return vec_box;
}


Box interp_box_method_call(AstNode *node, Box scope) {
    interp_trace("method_call");

    Box obj = interp_eval_ast(node->method_call.target, scope);
    const char *method_name = node->method_call.method;

    // Get the method
    Box method = box_dict_get(obj, box_str_new(method_name));
    if (!box_is_type(method, DTYPE_FN) && !box_is_type(method, DTYPE_NATIVE_FN)) {
        interp_error("Method not found or not a function", method_name);
        return __glo_box_undef;
    }

    // Evaluate arguments
    int num_args = node->method_call.num_args;
    Box *args = (Box *)interp_new(sizeof(Box) * (num_args + 1));
    args[0] = obj; // Pass the object as 'self' or 'this'
    for (int i = 0; i < num_args; i++) {
        args[i + 1] = interp_eval_ast(node->method_call.args[i], scope);
    }

    // Call the method
    Box result = interp_eval_fn(method, num_args + 1, args);
    interp_free(args);
    return result;
}


Box interp_box_member_access(AstNode *node, Box scope) {
    interp_trace("member_access");

    Box obj = interp_eval_ast(node->member_access.target, scope);
    const char *property = node->member_access.property;

    Box value = box_dict_get(obj, box_str_new(property));
    if (box_is_null(value)) {
        interp_error("Property not found", property);
        return __glo_box_undef;
    }

    return value;
}

Box interp_box_module(AstNode *node, Box scope) {
    interp_trace("module");

    // Implementation depends on how modules are handled
    // For simplicity, assume modules are similar to scopes

    Box module_scope = box_scope_new(scope);
    // Load module content into module_scope

    return module_scope;
}

Box interp_box_loop(AstNode *node, Box scope) {
    interp_trace("loop");

    Box result = __glo_box_undef;
    while (true) {
        Box condition = interp_eval_ast(node->loop.condition, scope);
        if (!box_is_truthy(condition)) {
            break;
        }
        result = interp_eval_ast(node->loop.body, scope);
    }

    return result;
}

Box interp_box_for(AstNode *node, Box scope) {
    interp_trace("for");

    // Evaluate the iterable
    Box iterable = interp_eval_ast(node->for_stmt.iterable, scope);

    // Get iterator
    VecIterator iter = box_vec_iterator(iterable);
    Box item;
    Box result = __glo_box_undef;

    while (box_vec_iterator_next(&iter, &item)) {
        // Create a new scope for each iteration
        Box iter_scope = box_scope_new(scope);
        interp_eval_def(node->for_stmt.iterator, item, iter_scope);

        result = interp_eval_ast(node->for_stmt.body, iter_scope);
    }

    return result;
}

Box interp_box_expression(AstNode *node, Box scope) {
    interp_trace("expression");

    return interp_eval_ast(node->exp_stmt.expression, scope);
}

Box interp_box_fn_anon(AstNode *node, Box scope) {
    interp_trace("anonymous_function");

    Do *do_ptr = (Do *)interp_new(sizeof(Do));
    do_ptr->type = DTYPE_FN;
    do_ptr->is_managed = false;

    Fn *function = &do_ptr->as.fn;
    function->allocator = NULL; // Set appropriate allocator
    function->ast = node;
    function->closure_scope = scope;
    function->name = NULL; // Anonymous function

    return box_from_do(do_ptr);
}

Box interp_box_const_def(AstNode *node, Box scope) {
    interp_trace("const_definition");

    Box value = interp_eval_ast(node->const_stmt.value, scope);
    interp_eval_def(node->const_stmt.name, value, scope);

    return __glo_box_undef;
}

Box interp_box_let_def(AstNode *node, Box scope) {
    interp_trace("let_definition");

    Box let_scope = box_scope_new(scope);
    for (int i = 0; i < node->let_stmt.num_bindings; i++) {
        const char *identifier = node->let_stmt.bindings[i].identifier;
        Box value = interp_eval_ast(node->let_stmt.bindings[i].expression, scope);
        interp_eval_def(identifier, value, let_scope);
    }

    return interp_eval_ast(node->let_stmt.body, let_scope);
}


Box interp_box_binary_op(AstNode *node, Box scope) {
    interp_trace("binary_operation");

    Box left = interp_eval_ast(node->bop.left, scope);
    Box right = interp_eval_ast(node->bop.right, scope);

    return interp_eval_bop(node->bop.op, left, right);
}

Box interp_box_unary_op(AstNode *node, Box scope) {
    interp_trace("unary_operation");

    Box operand = interp_eval_ast(node->uop.operand, scope);
    return interp_eval_uop(node->uop.op, operand);
}



Box interp_box_use(AstNode *node, Box scope) {
    interp_trace("use");

    // // Implementation depends on module loading system
    // // For example:
    // const char *module_name = node->use_stmt.module_path[node->use_stmt.num_module_paths - 1];
    // Box module = load_module(module_name); // Define load_module appropriately

    // if (box_is_null(module)) {
    //     interp_error("Failed to load module", module_name);
    //     return __glo_box_undef;
    // }

    // const char *alias = node->use_stmt.alias ? node->use_stmt.alias : module_name;
    // interp_eval_def(alias, module, scope);

    return __glo_box_undef;
}
Box interp_box_struct(AstNode *node, Box scope) {
    interp_trace("struct_definition");

    Do *do_ptr = (Do *)interp_new(sizeof(Do));
    do_ptr->type = DTYPE_STRUCT;
    do_ptr->is_managed = false;

    Struct *struct_def = &do_ptr->as.structure;
    struct_def->name = strdup(node->struct_def.name);
    struct_def->num_fields = node->struct_def.num_fields;

    struct_def->fields = (StructField *)interp_new(sizeof(StructField) * struct_def->num_fields);
    for (int i = 0; i < struct_def->num_fields; i++) {
        struct_def->fields[i].name = strdup(node->struct_def.fields[i].name);
        struct_def->fields[i].is_static = node->struct_def.fields[i].is_static;
        struct_def->fields[i].type = node->struct_def.fields[i].type ? strdup(node->struct_def.fields[i].type) : NULL;
    }

    Box struct_obj = box_from_do(do_ptr);
    interp_eval_def(struct_def->name, struct_obj, scope);

    return __glo_box_undef;
}


Box interp_box_object_literal(AstNode *node, Box scope) {
    interp_trace("object_literal");

    Box struct_def_obj;
    if (!interp_eval_lookup(node->struct_init.struct_name, scope, &struct_def_obj)) {
        interp_error("Undefined struct", node->struct_init.struct_name);
        return __glo_box_undef;
    }
    if (!box_is_type(struct_def_obj, DTYPE_STRUCT)) {
        interp_error("Identifier is not a struct", node->struct_init.struct_name);
        return __glo_box_undef;
    }

    Do *instance_do = (Do *)interp_new(sizeof(Do));
    instance_do->type = DTYPE_STRUCT_INSTANCE;
    instance_do->is_managed = false;

    Obj *instance = &instance_do->as.instance;
    instance->allocator = NULL; // Set appropriate allocator
    instance->struct_def = &box_as_do(struct_def_obj)->as.structure;
    instance->fields = box_dict_new(instance->struct_def->num_fields);

    for (int i = 0; i < node->struct_init.num_fields; i++) {
        const char *field_name = node->struct_init.fields[i].name;
        Box value = interp_eval_ast(node->struct_init.fields[i].value, scope);
        box_dict_set(instance->fields, box_str_new(field_name), value);
    }

    return box_from_do(instance_do);
}


Box interp_box_match(AstNode *node, Box scope) {
    interp_trace("match");

    Box variable_value = interp_eval_ast(node->match.variable, scope);

    for (int i = 0; i < node->match.num_cases; i++) {
        AstNode *test_node = node->match.cases[i].test;
        AstNode *value_node = node->match.cases[i].value;

        if (test_node == NULL) {
            // 'else' case
            return interp_eval_ast(value_node, scope);
        } else {
            Box test_value = interp_eval_ast(test_node, scope);
            if (box_is_eq(variable_value, test_value)) {
                return interp_eval_ast(value_node, scope);
            }
        }
    }

    return __glo_box_undef;
}

Box interp_box_mutation(AstNode *node, Box scope) {
    interp_trace("mutation");

    Box value = interp_eval_ast(node->mutation.value, scope);

    if (node->mutation.target->type == AST_ID) {
        const char *name = node->mutation.target->id.name;
        // Update the variable in the scope chain
        Box target_scope = scope;
        while (!box_is_null(target_scope)) {
            if (box_dict_has_as_bool(box_get_scope_dict(target_scope), box_str_new(name)).payload) {
                box_dict_set(box_get_scope_dict(target_scope), box_str_new(name), value);
                return value;
            }
            target_scope = box_get_scope_parent(target_scope);
        }
        interp_error("Variable not found for assignment", name);
    } else if (node->mutation.target->type == AST_MEMBER_ACCESS) {
        Box obj = interp_eval_ast(node->mutation.target->member_access.target, scope);
        const char *property = node->mutation.target->member_access.property;
        box_dict_set(obj, box_str_new(property), value);
        return value;
    } else {
        interp_error("Invalid assignment target", "");
    }

    return __glo_box_undef;
}



Box interp_box_fn_def(AstNode *node, Box scope) {
    Do *do_ptr = (Do *)interp_new(sizeof(Do));
    do_ptr->type = DTYPE_FN;
    do_ptr->is_managed = false;

    Fn *function = &do_ptr->as.fn;
    function->allocator = NULL; // Set appropriate allocator
    function->ast = node;
    function->closure_scope = scope;
    function->name = strdup(node->fn.name);

    Box fn_obj = box_from_do(do_ptr);
    interp_eval_def(function->name, fn_obj, scope);

    return __glo_box_undef;
}

Box interp_box_block(AstNode *node, Box scope) {
    interp_trace("block");

    if (node->block.num_statements == 0) {
        LOG(LOG_LEVEL_ERROR, "This block has no statements");
        return __glo_box_undef;
    }

    Box block_scope = box_scope_new(scope);
    Box result = __glo_box_undef;

    for (int i = 0; i < node->block.num_statements; i++) {
        result = interp_eval_ast(node->block.statements[i], block_scope);
    }
    return result;
}

Box interp_box_return(AstNode *node, Box scope) {
    interp_trace("return");

    Box result = __glo_box_undef;

    if (node->return_stmt.value) {
        result = interp_eval_ast(node->return_stmt.value, scope);
    }

    // Handle return value appropriately in your interpreter
    return result;
}


void box_scope_print(Box scope) {
    // Implement a function to print the contents of a scope
    Box keys = box_scope_keys(scope);
    printf("Scope:\n");
    for (unsigned int i = 0; i < box_as_do(keys)->as.vec.size; ++i) {
        Box key = box_as_do(keys)->as.vec.items[i];
        char key_buf[128];
        box_to_cstr(key, key_buf, sizeof(key_buf));
        printf("  %s\n", key_buf);
    }
}
#pragma endregion


#pragma region Main
/// @author Michael Burgess <doj@michaelburgess.co.uk>
/// @remark compile: 
///     clang -std=c23 -o doj -g doum.c 
/// @remark compile & debug:
///      clang -std=c23 -o test_doubt -g doum.c && lldb -o 'r' -o 'bt' -- ./test_doubt 

const char _glo_eg1[] = {
    #embed "eg.rs" prefix(0xEF, 0xBB, 0xBF, ) suffix(,) 
    0
};
 
int main(int argc, char **argv) {
    enum {CLI_HELP, CLI_INDENT};
    CliOption options[] = {
        [CLI_HELP]   = cli_opt_flag("help"),
        [CLI_INDENT] = cli_opt_default("indent", "    "),
    };

    interp_run_from_source(_glo_eg1, cli_opt_get(options, CLI_INDENT));
    return 0;
}
#pragma endregion
