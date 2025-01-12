/// @brief Doubt: A Programming Language
/// @author Michael Burgess <doubt@michaelburgess.co.uk>
/// @note This is a work-in-progress, and not yet ready for use.

///// ---------- ---------- ---------- PREAMBLE ---------- ---------- ---------- /////

//#region Preamble_DoubtHeader
#pragma region DoubtPreambleHeader

/// This file is here to preview some initial work developing 
/// a second-order probabilistic programming language.  
/// This is not the development version (which is local, and not released)
///... but a quick snapshot of some progress to provide some insight into the code. 


/// Compile with: 
/// clang -std=c23 -DTEST -DEXAMPLES -o doubt_test doubt.c
/// This will run the example code in `__glo_example1` below.

// Compile (full): 
// clang \
//   -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-extra-semi\
//   -fno-omit-frame-pointer -fsanitize=undefined,address \
//   -fcolor-diagnostics\
//   -g -O0 \
//   -std=c23 \
//   -DTEST \
//   -DEXAMPLES \
//   -o doubt_test doubt.c"

/// Debug with:
/// lldb -o 'r' -o 'bt' -- ./doubt_test


const char __glo_example1[] =
"const xs = [0, 1, 2, 3]\n"
"const ys = [0, 1, 4, 6]\n"
"struct Person :=\n"
"    /// @brief This is struct definition\n"
"    /// @note This will be remembered on the scope\n"
"    name : String \n"
"fn main() :=\n"
"    demo()\n"
"    log(1, 2, xs, ys)\n"
"    log({city: \"London\"})\n"
"    log(Person {name = \"Michael\"})\n"
"    log(for( x <- range(1, 3) ) x)\n"
"    log(infer(model(), #MCMC).take(3))\n"
"fn demo() :=\n"
"    loop \n"
"        i <- range(3)\n"
"    in \n"
"        match i \n"
"            if 1 -> log(\"Yay\")\n"
"            else -> log(\"Nay\")\n"
"loop fn model() :=\n"
"    let \n"
"        f = fn(x, a, b) -> a * x + b \n"
"        m = sample(normal(2, f(1, 2, 3)))\n"
"        c = sample(normal(1, 2))\n"
"        sigma = sample(gamma(1, 1))\n"
"    in\n"
"        log(\"x =\", f(10, m, c))\n"
"        observe(normal(f(10, m, c), sigma));\n"
"        return [m, c, sigma]\n";


#pragma endregion
//#endregion Preamble_DoubtHeader

//#region Preamble_LicenseHeader
#pragma region LicenseHeader

#define DOUBT_VERSION "Version: v0.1"
#define DOUBT_AUTHOR "Author: Michael J. Burgess (doubt@michaelburgess.co.uk)"
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

#pragma endregion
//#endregion Preamble_LicenseHeader


///// ---------- ---------- ---------- HEADERS ---------- ---------- ---------- /////

/// @headerfile internal.h

//#region Internal_ConfiguringDefines 
#pragma region ConfiguringDefines 

// #ifdef TEST
    #define DEBUG_LOGGING_ON
    #define DEBUG_ABORT_ON_ASSERT
    #define DEBUG_ABORT_ON_ERROR
    #define DEBUG_REQUIRE_ASSERTS
    #define TRACEBACK_ABORT_ON_OVERFLOW
    // #define DEBUG_MEMORY
// #endif

#pragma endregion
//#endregion Internal_ConfiguringDefines

//#region Internal_cIncludes 
#pragma region cIncludes 

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>

#pragma endregion
//#endregion Internal_cIncludes

//#region Internal_MacroHelpers
#pragma region MacroHelpers

#define _STR_HELPER(x) #x
#define _STR(x) _STR_HELPER(x)

// #define _NUM_ARGS(...) ( sizeof( (void *[]){__VA_ARGS__} ) / sizeof(void *) )

#pragma endregion
//#endregion Internal_MacroHelpers

//#region Internal_TimerHeader 
#pragma region TimerHeader 

struct LastTime {
    struct timespec start;
    struct timespec end;
} __glo_lAst_timer;

void timer_start() {
    clock_gettime(CLOCK_MONOTONIC, &__glo_lAst_timer.start);
}

void timer_end() {
    clock_gettime(CLOCK_MONOTONIC, &__glo_lAst_timer.start);
}

double timer_elapsed_ms() {
    return 1000 * (
        (double) (__glo_lAst_timer.end.tv_sec - __glo_lAst_timer.start.tv_sec) + 
        (double) (__glo_lAst_timer.end.tv_nsec - __glo_lAst_timer.start.tv_nsec) / 1e9
    );
}


#pragma endregion
//#endregion Internal_TimerHeader

//#region Internal_ArenaHeader
#pragma region ArenaHeader

typedef struct Arena {
    void **blocks;
    size_t *block_sizes;  // Array to track sizes of blocks
    size_t block_size;
    size_t used;
    size_t num_blocks;
} Arena;


unsigned char *Arena_save_cnew(Arena *a, size_t *out_size);
bool Arena_load_cnew(Arena *a, unsigned char *data, size_t size);

bool Arena_init(Arena *a, size_t block_size);
void Arena_adel(void *ptr); //a no-op
void Arena_adel_underlying(Arena *a);
void *Arena_anew(Arena *a, size_t size);
void *Arena_cextend(Arena *a, void *mem, size_t old_size, size_t new_size);
void Arena_reset(Arena *a);
void Arena_rewind(Arena *a, size_t pAst_size);
void Arena_print_stats(Arena *a);
void Arena_test_main();


#define ARENA_1MB (1024 * 1024)

#pragma endregion
//#endregion Internal_ArenaHeader

//#region Internal_StringHeader
#pragma region StringHeader


#define ANSI_COL_RESET       "\x1b[0m"
#define ANSI_COL_CYAN       "\x1b[36m" // Cyan
#define ANSI_COL_GREEN        "\x1b[32m" // Green
#define ANSI_COL_YELLOW     "\x1b[33m" // Yellow
#define ANSI_COL_RED       "\x1b[31m" // Red
#define ANSI_COL_MAGENTA "\x1b[35m" // Magenta
#define ANSI_COL_BLUE        "\x1b[34m" // Blue
#define ANSI_COL_WHITE     "\x1b[37m" // White

// Bright Colors
#define ANSI_COL_BRIGHT_BLACK "\x1b[90m"
#define ANSI_COL_BRIGHT_RED   "\x1b[91m"
#define ANSI_COL_BRIGHT_GREEN "\x1b[92m"
#define ANSI_COL_BRIGHT_YELLOW "\x1b[93m"
#define ANSI_COL_BRIGHT_BLUE  "\x1b[94m"
#define ANSI_COL_BRIGHT_MAGENTA "\x1b[95m"
#define ANSI_COL_BRIGHT_CYAN  "\x1b[96m"
#define ANSI_COL_BRIGHT_WHITE "\x1b[97m"

// Background Colors (Optional)
#define ANSI_COL_BG_RED       "\x1b[41m"
#define ANSI_COL_BG_GREEN     "\x1b[42m"
#define ANSI_COL_BG_YELLOW    "\x1b[43m"
#define ANSI_COL_BG_BLUE      "\x1b[44m"
#define ANSI_COL_BG_MAGENTA   "\x1b[45m"
#define ANSI_COL_BG_CYAN      "\x1b[46m"
#define ANSI_COL_BG_WHITE     "\x1b[47m"

typedef struct Str {
    const char *cstr;
    size_t size; 
} Str;

#define Str_len(str) (str).size

char *cstr_repeat_anew(const char *cstr, size_t count);
bool write_bytes_to_file(const char *filename, const char *data, size_t size);
unsigned char *read_bytes_from_file_cnew(const char *filename,  size_t *out_size);

int char_to_base37(char c);
char base37_to_char(uint8_t value);

// Function declarations
void Str_puts(Str str);
void Str_print_variadic(size_t count, ...);
void Str_printf(const char *format, ...);
Str Str_read_file_anew(const char *filename);
Str Str_trim_delim(Str str);
Str Str_anew(const char *cstr, size_t len);
Str Str_join_anew(Str one, ...);
Str Str_col_anew(const char *color_code, Str str);
Str Str_chr(char chr);
Str Str_glue_anew(Str left, Str right);
Str Str_glue_sep_anew(Str left, Str sep, Str right);
Str Str_firstN(Str str, size_t n);
bool Str_eq(Str left, Str right);
bool Str_eq_cstr(Str left, const char *right);
bool Str_eq_chr(Str left, char right);
Str Str_copy(Str str);
bool Str_contains(Str str, Str part);
bool Str_index_of(Str str, Str part, size_t *out_index);
bool Str_lAst_index(Str str, Str part, size_t *out_index);
bool Str_contains_chr(Str str, char chr);
int Str_index_of_chr(Str str, char chr);                            
int Str_lAst_index_of_chr(Str str, char chr);                       
Str Str_ncopy(Str str, size_t start, size_t end);                          
Str Str_sub(Str str, size_t start, size_t size);  
Str Str_replace(Str str, char target, char replacement);        
Str Str_trim(Str str);                                          
Str Str_upper(Str str);                                         
Str Str_lower(Str str);                                         
int Str_cmp(Str left, Str right);
int Str_cmp_natural(Str left, Str right);
Str Str_join_chr_anew(Str str, char chr);                             
Str Str_join_cstr_anew(Str str, const char *cstr);                    
Str *Str_split_chr(Str str, char delimiter, size_t *out_count);  
Str *Str_lines(Str str, size_t *out_count);
Str Str_repeat_anew(Str str, size_t count);
Str Str_fmt_anew(Str format, ...);
Str Str_fmt_va_anew(Str format, va_list args);
void Str_adel(Str str); 

#define Str_empty() ((Str) {0})

bool Str_is_empty(Str str);
Str Str_part(Str line, size_t start, size_t end);

#define Str_from_cstr(txt) ((Str){ .cstr = txt, .size = strlen(txt)})
#define Str_chr_at(line, at) ((line).cstr[at])
#define Str_chr_eq(line, at, chr) ((line).cstr[at] == chr)
#define s(txt) ((Str){ .cstr = (txt), .size = sizeof(txt) - 1 })
#define ch(chr) ((Str){ .cstr = &(chr), .size = 1 })
#define fmt(str) (int) str.size, str.cstr


#pragma endregion
//#endregion Internal_StringHeader

//#region Internal_LoggingHeader
#pragma region LoggingHeader

typedef enum LogLevel {
    LL_DEFAULT,
    LL_DEBUG,
    LL_INFO,
    LL_WARNING,
    LL_ERROR,
    LL_RECOVERABLE,
    LL_ASSERT,
    LL_TEST, 
    LL_ENUM_MAX,
} LogLevel;


Str ll_nameof(LogLevel ll) {
    static char *names[] = {
        [LL_DEFAULT] = "DEFAULT",
        [LL_DEBUG] = "DEBUG",
        [LL_INFO] = "INFO",
        [LL_WARNING] = "WARNING",
        [LL_ERROR] = "ERROR",
        [LL_RECOVERABLE] = "RECOVERABLE",
        [LL_ASSERT] = "ASSERT",
        [LL_TEST] = "TEST",
        [LL_ENUM_MAX] = "<LL_ERROR>"
    };

    return Str_from_cstr(names[ll]);
}



// Updated log_message with colored output
void log_message(LogLevel level, Str format, ...) {
    if (level < 0 || level >= LL_ENUM_MAX) {
        level = LL_ERROR;
    }

    const char *level_cols[]= {
        [LL_DEBUG] = ANSI_COL_BLUE,
        [LL_INFO] = ANSI_COL_CYAN,
        [LL_WARNING] = ANSI_COL_MAGENTA,
        [LL_ERROR] = ANSI_COL_RED,
        [LL_RECOVERABLE] = ANSI_COL_CYAN,
        [LL_ASSERT] = ANSI_COL_RED,
        [LL_TEST] = ANSI_COL_GREEN,
    };

    Str level_str = Str_col_anew(level_cols[level], ll_nameof(level));
    if (Str_is_empty(level_str)) {
        fprintf(stderr, "[%.*s] ", fmt(ll_nameof(level)));
    } else {
        fprintf(stderr, "[%.*s] ", fmt(level_str));
    }

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format.cstr, args);
    va_end(args);
    fprintf(stderr, "\n");

    #ifdef DEBUG_ABORT_ON_ERROR
        if(level == LL_ERROR) { abort(); }
    #endif

    #ifdef DEBUG_ABORT_ON_ASSERT
        if(level == LL_ASSERT) { abort(); }
    #endif
}



#pragma endregion
//#endregion Internal_LoggingHeader

//#region Internal_DebugHeader
#pragma region DebugHeader
/// @brief Behaviour which will compile-away given non-debug state
#define NOOP ((void) 0)

#ifdef DEBUG_LOGGING_ON
    /// file location information is suppressed w/o debug
    #define sMSG(message) \
        s(__FILE__ ":" _STR(__LINE__) ": " message)

    /// @todo unused
    // static inline Str Str_debug_message(const char *message, const char *func, const char *file, int line) {
    //     static char buffer[256];
    //     snprintf(buffer, sizeof(buffer), "%s (%s) %s:%d", message, func, file, line); 
    //     return (Str){ .cstr = buffer, .size = strlen(buffer) };
    // }

    // #define sBUF(message) Str_debug_message(message, __func__, __FILE__, __LINE__)

    #define debugger\
        Str_println(sMSG("<- Debugger: HERE! ->\n\n")); \
        abort()

    #define watch(var, cond) ((cond) ? (abort(), var) : var)

    #define LOG(format, ...) \
        log_message(LL_DEBUG, sMSG(format), __VA_ARGS__)
        
    #define log_assert(condition, message)\
        if(!(condition)) {log_message(LL_ASSERT, s("%.*s FAIL: %s"), fmt(message), _STR(condition));}

#else
    #define LOG(level, format, ...) NOOP
    #define sMSG(message) s(message)
    #define watch(var, cond) var
    #define debugger NOOP
    #define log_assert(condition, ...) NOOP
#endif


#ifdef DEBUG_REQUIRE_ASSERTS
    #define require_not_null(expr) assert((expr) != NULL)
    #define require_positive(expr) assert((expr) > 0)
    #define require_null(expr) assert((expr) == NULL)
#else
    /// @todo swap out all checks
    #define require_not_null(expr) 
#endif



    

#pragma endregion
//#endregion Internal_DebugHeader

//#region Internal_ErrorHeader
#pragma region ErrorHeader

#define require_index_bounded(index, limit) \
if ((index) >= (limit)) log_message(LL_ERROR, sMSG("Index out of bounds: %zu >= %zu"), (size_t)(index), (size_t)(limit))

#define require_enum_bounded(enum_val, max_val) \
if ((enum_val) < 0 || (enum_val) >= (max_val)) log_message(LL_ERROR, sMSG("Enum out of range: %d"), (int)(enum_val)); \


#define error_oom() log_message(LL_ERROR, sMSG("Allocation failed!"))
#define error_never() log_message(LL_ERROR, sMSG("Unreachable!"))

#pragma endregion
//#endregion Internal_ErrorHeader

//#region Internal_ArrayHelperHeader 
#pragma region ArrayHelperHeader 
/// @todo: maybe, _SIZE_SMALL_BRIEF, and _LONGLIVE = 32, 16 
#define ARRAY_SIZE_SMALL 16
#define ARRAY_SIZE_MEDIUM 256
 

#define ARRAY_REQUIRE_CHECKS

#ifdef ARRAY_REQUIRE_CHECKS
    #define require_small_array(count)\
        log_assert(count <= ARRAY_SIZE_SMALL, sMSG("Small array beyond capacity."));

    #define require_medium_array(count)\
        log_assert(count <= ARRAY_SIZE_MEDIUM, sMSG("Medium array beyond capacity."));

    #define require_not_null_array(elements)\
        log_assert(elements != NULL, sMSG("Array is NULL."));

    #define require_size_bounded(size, index)\
        log_assert(index < size, sMSG("Index out of bounds."));

    #define require_capacity_bounded(capacity, index)\
        log_assert(index < capacity, sMSG("Index out of bounds."));
#else
    #define require_small_array(count) NOOP
    #define require_medium_array(count) NOOP
    #define require_not_null_array(arr) NOOP
    #define require_size_bounded(arr, index) NOOP
    #define require_capacity_bounded(arr, index) NOOP
#endif



#define array_init(allocator, arr, arr_cap) \
    (arr).elements = allocator(arr_cap * sizeof(typeof((arr).elements[0]))); \
    (arr).meta.capacity = arr_cap; \
    (arr).meta.size = 0;\
    if(!(arr).elements) { error_oom(); } \
    memset(arr.elements, 0, arr_cap * sizeof(typeof((arr).elements[0]))); 

#define array_init_ref(allocator, arr, arr_cap) \
    require_not_null_array(arr); \
    (arr)->elements = allocator(arr_cap * sizeof(typeof((arr)->elements[0]))); \
    (arr)->meta.capacity = arr_cap; \
    (arr)->meta.size = 0;\
    if(!(arr)->elements) { error_oom(); } \
    memset(arr->elements, 0, arr_cap * sizeof(typeof((arr)->elements[0]))); 


#define array_init_small(allocator, arr)\
    array_init(allocator, arr, ARRAY_SIZE_SMALL)

#define len(arr) (arr).meta.size
#define cap(arr) (arr).meta.capacity

#define push(arr, elem) \
    (arr).elements[(arr).meta.size++] = elem

#define pop(arr) \
    (arr).elements[--(arr).meta.size]

#define last(arr) \
    (arr).elements[(arr).meta.size - 1]

#define first(arr) \
    (arr).elements[0]

#define set_zero_size(arr) \
    (arr).meta.size = 0

#define is_empty(arr) \
    (arr).meta.size == 0

#define is_full(arr) \
    (arr).size == (arr).meta.capacity

#define at(arr, index) \
    (arr).elements[index]

#define set(arr, index, value) \
    (arr).elements[index] = value


#define len_ref(arr) \
    (arr)->meta.size

#define capacity_ref(arr) \
    (arr)->meta.capacity
    
#define incr_len_ref(arr) \
    (arr)->meta.size++

#define decr_len_ref(arr) \
    (arr)->meta.size--

#define push_ref(arr, elem) \
    (arr)->elements[(arr)->meta.size++] = elem

#define pop_ref(arr) \
    (arr)->elements[--(arr)->meta.size]

#define last_ref(arr) \
    (arr)->elements[(arr)->meta.size - 1]

#define first_ref(arr) \
    (arr)->elements[0]

#define set_zero_len_ref(arr) \
    (arr)->meta.size = 0

#define is_empty_ref(arr) \
    (arr)->meta.size == 0

#define is_empty_ref(arr) \
    (arr)->meta.size == (arr)->meta.capacity

#define at_ref(arr, index) \
    (arr)->elements[index]

#define set_ref(arr, index, value) \
    (arr)->elements[index] = value

// typedef struct IndexSlice {
//     size_t start;
//     size_t end;
// } IndexSlice;

// typedef struct GenericSlice {
//     void *start;
//     void *end;
// } GenericSlice;

// typedef struct Array {
//     void *data;
//     size_t size;
//     size_t capacity;
//     size_t elem_size;
// } GenericArray;

// typedef struct ArraySlice {
//     void *data;
//     size_t size;
//     size_t elem_size;
// } GenericArraySlice;

// typedef struct FatPtr {
//     void *data;
//     size_t size;
// } FatPtr;

#pragma endregion
//#endregion Internal_ArrayHelperHeader

//#region Internal_HeapHeader
#pragma region HeapHeader

#define HEAP_BASE_CAPACITY 8 * (1022 * 1024) // 8 MB
#define HEAP_GC_1MB (1024 * 1024) // 1MB
#define HEAP_GC_2GB (2 * 1024 * 1024 * 1024) // 2 Gb
#define HEAP_GC_PREALLOC_RATIO 1.25
#define HEAP_GC_DEFAULT_THRESHOLD 256 * HEAP_GC_1MB


static const size_t PRIMES[] = {
    13, 53, 97, 193, 389, 769,
    1543, 3079, 6151, 12289, 24593,
    49157, 98317, 196613, 393241, 786433,
    1572869, 3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189, 805306457,
    1610612741
};
static const size_t NUM_PRIMES = sizeof(PRIMES) / sizeof(PRIMES[0]);

static inline size_t HeapArray_next_capacity(size_t initial_capacity) {
    for (size_t i = 0; i < NUM_PRIMES; i++) {
        if (PRIMES[i] >= initial_capacity) {
            return PRIMES[i];
        }
    }

    error_oom();
    log_assert(false, sMSG("Failed to find next prime capacity."));
    return (size_t) (initial_capacity * 2);
}

// Error codes for GC operations
typedef enum GCError {
    GC_ZERO = 0,
    GC_SUCCESS = 1,
    GC_ERR_HEAP_FULL,
    GC_ERR_INVALID_POINTER,
    GC_ERR_INVALID_TYPE,
    GC_ERR_ENUM_MAX
} GCError;

// Types of Heap Objects
typedef enum HeapObjectType {
    HEAP_ZERO = 0,
    HEAP_ALIVE,     // Object is alive and accessible
    HEAP_DEAD,      // Object marked for deletion
    HEAP_PINNED,    // Object is pinned and cannot be moved
    HEAP_ENUM_MAX  
} HeapObjectType;

// Forward declaration
struct Heap;

// Structure representing an object in the heap

/// @brief metadata attributes
typedef struct HeapMetaGc {
    HeapObjectType state;
    struct HeapObject *next;  // For free list chaining
    bool marked;        // GC marking flag
} HeapMetaGc;

/// @note data/payload attributes
typedef struct MetaData {
    uint8_t type;           // Type of the data
    size_t size;            // Size of the data
    size_t capacity;        // Capacity of the data
} MetaData;

typedef struct HeapObject {
    HeapMetaGc gc;
    MetaData meta;
    void *data;             // Pointer to the actual data
} HeapObject;



// Structure representing the heap
typedef struct Heap {
    struct  {
        size_t capacity;
        size_t size;
    } meta;
    HeapObject *free_list;      // Linked list of free elements
    HeapObject **elements;       // Dynamic array of all allocated elements
    size_t gc_threshold;        // Threshold to trigger GC
    size_t generation_count;    // Track GC cycles
} Heap;


void Heap_elements_cnew(Heap *h, size_t initial_capacity, size_t gc_threshold) {
    h->free_list = NULL;
    h->gc_threshold = gc_threshold;
    h->generation_count = 0;
    array_init_ref(cnew, h, initial_capacity);
}

void *Heap_cnew(Heap *heap, size_t size, uint8_t type) {
    HeapObject *obj = NULL;

    // Reuse from free list if available
    if (heap->free_list) {
        obj = heap->free_list;
        heap->free_list = heap->free_list->gc.next;
    } else {
        // Expand the elements array if necessary
        if (is_empty_ref(heap)) {
            size_t new_capacity = HeapArray_next_capacity(capacity_ref(heap));
            HeapObject **new_elements = cextend(heap->elements, sizeof(HeapObject *) * new_capacity);
            if (!new_elements) {
                log_message(LL_ERROR, sMSG("Heap allocation failed: no space to expand elements array."));
                return NULL;
            }
            memset(new_elements + capacity_ref(heap), 0, sizeof(HeapObject *) * (new_capacity - capacity_ref(heap)));
            heap->elements = new_elements;
            heap->meta.capacity = new_capacity;
        }

        // Allocate new HeapObject
        obj = cnew(sizeof(HeapObject));
        if (!obj) {
            log_message(LL_ERROR, sMSG("Heap allocation failed: unable to allocate HeapObject."));
            return NULL;
        }
        obj->gc.marked = false;
        obj->gc.next = NULL;
        push_ref(heap, obj);
    }

    obj->meta.type = type;
    obj->meta.size = size;
    obj->meta.capacity = size;
    obj->data = cnew(size);
    if (!obj->data) {
        log_message(LL_ERROR, sMSG("Heap allocation failed: unable to allocate object data."));
        // Return obj to free list
        obj->meta.type = HEAP_DEAD;
        return NULL;
    }

    return obj->data;
}

void Heap_destroy(Heap *heap);

void Heap_gc_mark(Heap *heap, HeapObject *obj);
void Heap_gc_adel(HeapObject *obj);
void Heap_gc_sweep();


#pragma endregion
//#endregion Internal_HeapHeader

//#region Internal_TracebackHeader
#pragma region TracebackHeader

typedef struct TraceEntry {
    Str fn_name;
    Str argument;
    Str file;
    size_t line;
} TraceEntry;

typedef struct CallStack {
    TraceEntry elements[512];
    size_t top;
} CallStack;


void Tracer_pop(CallStack *traces);
void Tracer_print_stack(CallStack *traces, size_t depth);
void Tracer_push(CallStack *traces, Str fn_name, Str arg, Str file, size_t line);

#pragma endregion
//#endregion Internal_TracebackHeader

//#region Internal_PerformanceAndProfilingHeader
#pragma region PerformanceAndProfilingHeader

/// @note profiling prototypes 
void perf_timer_start();
void perf_timer_end();
double perf_timer_elapsed_ms();



#pragma endregion
//#endregion Internal_PerformanceAndProfilingHeader

//#region Internal_GlobalContext
#pragma region GlobalContext


/// @brief Global memory context
/// @todo revisit this
/// eg., do we need an internal context to point to this?
///     ... or can we just inline on the internal context?

struct Memory {
    Arena arena_static;
    Arena ar_interp_local;
    Arena ar_interp_global;
    Heap    Heap_interp_gc;
} __glo_memory = {0};


#define mem() &__glo_memory

struct InternalContext {
    Heap *heap;
    Arena *arena;

    /// @note Arena 
    void *(*anew)(size_t);
    void *(*adel)(void *); /// no-op

    /// @note Heap (GC)
    void *(*gcnew)(size_t, uint8_t);
    void (*collect)(void *);


    // void *(*cnew)(size_t, size_t);
    // void *(*cextend)(void*, size_t);
    // void (*free)(void *);
    CallStack *callstack;
} __glo_internal_context = {0};


#define DEBUG_MEMORY

#ifdef DEBUG_MEMORY
    struct OsMemoryAllocations {
        void *allocations[1024];
        size_t num_allocations;
        Str line;
        bool is_free;
    } __glo_os_memory = {0};


    //__glo_internal_context.adel(ptr)
    #define adel(ptr) NOOP
    #define anew(size) __glo_internal_context.anew(size)
    #define gcnew(size, type) __glo_internal_context.gcnew(size, type)

    #define cnew(size) \
        ((log_message(LL_DEBUG, sMSG("Allocating %zu bytes"), (size)), \
        __glo_os_memory.allocations[__glo_os_memory.num_allocations++] = malloc(size)))

    
    #define cextend(ptr, count_size) \
        (log_message(LL_DEBUG, sMSG("Reallocating %zu bytes"), (count_size)), \
        __glo_os_memory.allocations[__glo_os_memory.num_allocations++] = realloc(ptr, count_size))

    #define cfree(ptr) \
        (log_message(LL_DEBUG, sMSG("Freeing memory: %p"), ptr), \
        __glo_os_memory.is_free = true, \
        free(ptr))

    #define cfree_log_leaks() \
        for(size_t i = 0; i < __glo_os_memory.num_allocations; i++) { \
            if(__glo_os_memory.is_free) { \
                log_message(LL_DEBUG, sMSG("Freed memory: %p"), __glo_os_memory.allocations[i]); \
            } else { \
                log_message(LL_DEBUG, sMSG("Leaked memory: %p"), __glo_os_memory.allocations[i]); \
            } \
        }
        
#else
    #define cnew(count, size) calloc(count, size)
    #define cextend(ptr, count_size) realloc(ptr, count_size)
    #define cfree(ptr) free(ptr)
    #define cfree_log_leaks() NOOP
#endif




void ctx_setup(struct InternalContext *ctx, Heap *heap, Arena *arena) {
    require_not_null(ctx);
    require_not_null(heap);
    require_not_null(arena);

    ctx->arena = arena;
    ctx->heap = heap;
    ctx->anew = ctx_Arena_anew;
    ctx->adel = Arena_adel;

    ctx->gcnew = ctx_Heap_cnew;

    // ctx->cnew = cnew;
    // ctx->cextend = cextend;

    Heap_elements_cnew(ctx->heap, 
        HEAP_BASE_CAPACITY, 
        HEAP_BASE_CAPACITY / HEAP_GC_PREALLOC_RATIO
    );
    
    ctx->callstack = Arena_anew(ctx->arena, sizeof(CallStack));
    require_not_null(ctx->callstack);
    require_not_null(ctx->arena);
}



#define ctx() __glo_internal_context
#define ctx_change_arena(alloc_name) ctx().arena = alloc_name;

#define ctx_trace_me(moreinfo)\
    Tracer_push(ctx().callstack, s(__func__), moreinfo, s(__FILE__), __LINE__)

#define ctx_trace_print(depth) Tracer_print_stack(ctx().callstack, depth)



void *ctx_Arena_anew(size_t size) {
    return Arena_anew(ctx().arena, size);
}

// void *ctx_Arena_cextend(void *mem, size_t old_size, size_t new_size) {
//     return Arena_cextend(ctx().arena, mem, old_size, new_size);
// }

void *ctx_Heap_cnew(size_t size, uint8_t type) {
    return Heap_cnew(ctx().heap, size, type);
}


#pragma endregion
//#endregion Internal_GlobalContext

//#region Internal_CliHeader
#pragma region CliHeader

typedef struct CliOption {
    Str name;
    Str value;
    Str or_else;
    bool is_set;
} CliOption;

void cli_parse_args(int argc, char **argv, CliOption *out_opts, size_t num_opts);
#define cli_parse_opts(opts) (cli_parse_args(argc, argv, opts, sizeof(opts) / sizeof(CliOption)))
#define cli_opt_flag(o_name) (CliOption)\
    {.name = o_name, .value = Str_empty(), .or_else = Str_empty(), .is_set = false}

#define cli_opt_default(o_name, o_or_else) (CliOption)\
    {.name = o_name, .value = Str_empty(), .or_else = o_or_else, .is_set = false}

#define cli_opt_get(options, index)\
 (options[index].is_set ? options[index].value : options[index].or_else)


#pragma endregion
//#endregion Internal_CliHeader

//#region Internal_JsonHeader
#pragma region JsonHeader
#pragma endregion
//#endregion Internal_JsonHeader

//#region Internal_UnitTestHeader
#pragma region UnitTestHeader

typedef struct TestReport {
    Str messages[32];
    bool results[32];
    size_t num_results;
} TestReport;

typedef void (*UnitTestFnPtr)(TestReport *r);

typedef struct TestSuite {
    UnitTestFnPtr *tests;
    TestReport reports[32];
    size_t num_tests;
} TestSuite; 

// void TestSuite_anew(TestSuite *ts, UnitTestFnPtr *tests, size_t num_tests);
void TestSuite_anew(TestSuite *ts, UnitTestFnPtr *tests, void *_end);
void TestReport_push(TestReport *r, bool result, Str message);
void TestSuite_print(TestSuite *ts);
void TestSuite_run(TestSuite *ts);
void TestSuite_register(TestSuite *ts,  UnitTestFnPtr fnptr);

#define _NUM_TEST_ARGS(...) ( sizeof( (UnitTestFnPtr []){__VA_ARGS__} ) / sizeof(UnitTestFnPtr) )

#define test_suite(...)     {\
    .num_tests = _NUM_TEST_ARGS(__VA_ARGS__), \
    .tests = ((UnitTestFnPtr[]) { __VA_ARGS__ })\
}


#define TEST_STR(expr) "(" _STR(expr) ")"

#define test_assert(report, expr, message) \
    TestReport_push(report, (bool) (expr), s(message TEST_STR(expr))) 

#pragma endregion
//#endregion Internal_UnitTestHeader


/// @headerfile parsing.h

//#region Parsing_TokenHeader
#pragma region ParsingTokenHeader

typedef enum TokenType {
    TT_INDENT = 0,
    TT_DEDENT,
    TT_END,
    TT_TAG,
    TT_IDENTIFIER,
    TT_TYPE,
    TT_ANNOTATION,
    TT_KEYWORD,
    TT_FLOAT,
    TT_DOUBLE,
    TT_INT,
    TT_BRA_OPEN,
    TT_BRA_CLOSE,
    TT_STRING,
    TT_ASSIGN,
    TT_OP,
    TT_SEP,
    TT_DISCARD,
    TT_IGNORE,
    TT_DEREF,
    TT_DOC_COMMENT,
} TokenType;


Str tt_nameof(TokenType tt) {
    static const char *names[] = {
        [TT_INDENT] = "token(indent)",
        [TT_DEDENT] = "token(dedent)",
        [TT_END] = "token(end)",
        [TT_TAG] = "token(tag)",
        [TT_IDENTIFIER] = "token(identifier)",
        [TT_TYPE] = "token(type)",
        [TT_ANNOTATION] = "token(type_annotation)",
        [TT_KEYWORD] = "token(keyword)",
        [TT_DOUBLE] = "token(double)",
        [TT_INT] = "token(int)",
        [TT_BRA_OPEN] = "token(open_bracket)",
        [TT_BRA_CLOSE] = "token(close_bracket)",
        [TT_STRING] = "token(string)",
        [TT_ASSIGN] = "token(assign)",
        [TT_OP] = "token(op)",
        [TT_SEP] = "token(sep)",
        [TT_DISCARD] = "token(discard)",
        [TT_IGNORE] = "token(ignore)",
        [TT_DEREF] = "token(deref)",
        [TT_DOC_COMMENT] = "token(doc_comment)",
    };

    return Str_from_cstr(names[tt]);
}

typedef struct Token {
    TokenType type;
    Str value;
    size_t line;
    size_t col;
} Token;


#define lex_print_all(lexer)\
    lex_print_elements(lexer.elements, 0, len(lexer), len(lexer)+1)


// void lexer_elements_print(Token *elements, size_t size);
// Str lexer_elements_to_readable(Token *elements, size_t size);
// Str lexer_elements_to_json(Token *elements, size_t size);

#pragma endregion
//#endregion

//#region Parsing_AstHeader 
#pragma region ParsingAstHeader 

typedef enum AstType {
    AST_ZERO,
    AST_DISCARD,
    AST_BOX,
    AST_EXPRESSION,
    AST_BOP,
    AST_UOP,
    AST_ID,
    AST_INT,
    AST_FLOAT,
    AST_DOUBLE,
    AST_STR,
    AST_TAG,
    // AST_CALL, // @todo what's this for?
    AST_FN_DEF_CALL,
    AST_METHOD_CALL,
    AST_MEMBER_ACCESS,
    AST_BLOCK,
    AST_FN_DEF,
    AST_FN_DEF_MACRO,
    AST_FN_DEF_ANON,
    AST_FN_DEF_GEN,
    AST_STRUCT_DEF,
    AST_OBJECT_LITERAL,
    AST_TRAIT,
    AST_IF,
    AST_MOD,
    AST_FOR,
    AST_LOOP,
    AST_LEF_DEF,
    AST_BINDING,
    AST_RETURN,
    AST_MATCH,
    AST_DICT,
    AST_VEC,
    AST_USE,
    AST_MODULE,
    AST_MUTATION,
    AST_CONST_DEF,
    AST_YIELD,
    AST_IGNORE,
    AST_TYPE_ANNOTATION, 
    AST_DESTRUCTURE_ASSIGN,
    AST_GENERIC_TYPE,
    AST_ENUM_MAX,
} AstType;


Str Ast_nameof(AstType ast) {
    
    if(ast >= AST_ENUM_MAX || ast < 0) {
        return Str_from_cstr("ast(<ERROR>)");
    }

    static const char *names[] = {
        [AST_ZERO] = "ast(<ERROR(ZERO)>)",
        [AST_DISCARD] = "ast(discard)",
        [AST_BOX] = "ast(box<PARTIAL>)",
        [AST_EXPRESSION] = "ast(expression)",
        [AST_BOP] = "ast(bop)",
        [AST_UOP] = "ast(uop)",
        [AST_ID] = "ast(id)",
        [AST_INT] = "ast(int)",
        [AST_FLOAT] = "ast(float)",
        [AST_DOUBLE] = "ast(double)",
        [AST_STR] = "ast(str)",
        [AST_TAG] = "ast(tag)",
        // [AST_CALL] = "ast(call)",
        [AST_METHOD_CALL] = "ast(method_call)",
        [AST_MEMBER_ACCESS] = "ast(member_access)",
        [AST_BLOCK] = "ast(block)",
        [AST_FN_DEF] = "ast(fn)",
        [AST_FN_DEF_CALL] = "ast(call_fn)",
        [AST_FN_DEF_MACRO] = "ast(fn_macro)",
        [AST_FN_DEF_ANON] = "ast(fn_anon)",
        [AST_FN_DEF_GEN] = "ast(fn_gen)",
        [AST_STRUCT_DEF] = "ast(struct)",
        [AST_OBJECT_LITERAL] = "ast(object_literal)",
        [AST_TRAIT] = "ast(trait)",
        [AST_IF] = "ast(if)",
        [AST_FOR] = "ast(for)",
        [AST_LOOP] = "ast(loop)",
        [AST_LEF_DEF] = "ast(let)",
        [AST_BINDING] = "ast(binding)",
        [AST_RETURN] = "ast(return)",
        [AST_MATCH] = "ast(match)",
        [AST_DICT] = "ast(dict)",
        [AST_VEC] = "ast(vec)",
        [AST_USE] = "ast(use)",
        [AST_MODULE] = "ast(module)",
        [AST_MUTATION] = "ast(mutation)",
        [AST_CONST_DEF] = "ast(const)",
        [AST_YIELD] = "ast(yield)",
        [AST_IGNORE] = "ast(ignore)",
        [AST_TYPE_ANNOTATION] = "ast(type_annotation)",
        [AST_ENUM_MAX] = "ast(<ERROR(NUM_TYPES)>)"
    };

    return Str_from_cstr(names[ast]);
}

typedef struct Ast Ast;


#define Ast_require_type(node, ast, error_fn) \
    require_not_null(node); \
    require_positive(node->type); \
    error_fn

struct Ast {
    AstType type;
    size_t line;
    size_t col;
    union {
        // Box box;  /// @todo for compile-time partial eval of AST
        struct AstTypeAnnotation {
            Str qualifier;               // e.g., "ref", "mut", etc. (optional)
            Str base_type;               // Base type name, e.g., "Vec", "Dict"
            Ast **type_params;       // Array of type parameter AST nodes (optional)
            size_t num_type_params;    // Number of type parameters
            Str size_constraint;         // Size constraint, e.g., "32", "dyn", "?"
        } type_annotation;
        struct AstDestructureAssign {
            Ast **variables;
            size_t num_variables;
            Ast *expression;
        } destructure_assign;
        struct AstIdentifier {
            Str name;
        } id;
        struct AstDoubleValue {
            double value;
        } dble;
        struct AstIntValue {
            int value;
        } integer;
        struct AstStringValue {
            Str value;
        } str;
        struct {
            Str name;
        } tag;
        struct AstBOP {
            Str op;
            Ast *left;
            Ast *right;
        } bop;
        struct AstUOP {
            Str op;
            Ast *operand;
        } uop;
        struct AstFnCall {
            Ast *callee; 
            Ast **args;
            size_t num_args;
        } call;
        struct AstMethodCall {
            Ast *target;
            Str method;
            Ast **args;
            size_t num_args;
        } method_call;
        struct AstMemberAccess {
            Ast *target;
            Str property;
        } member_access;
        struct AstBlock {
            Ast **statements;
            size_t num_statements;
            bool transparent;
        } block;
        struct AstFnDef {
            Str name;
            struct FnParam {
                Str name;
                Ast *type;
                Ast *default_value;
                // Str *tags;
                // size_t num_tags;
            } *params;
            size_t num_params;
            Ast *body;
            bool is_macro;
            bool is_generator;  // Add this flag
        } fn;
        struct AstFnAnon {
            struct AnonFnParam {
                Str name;
                Str type;
                Ast *default_value;
                // Str *tags;  /// @todo is this const?
                size_t num_tags;
            } *params;
            size_t num_params;
            Ast *body;
        } fn_anon;
        struct AstStructDef {
            Str name;
            struct StructField {
                Str name;
                // bool is_static;
                Ast *type;
                Ast *default_value;
            } *fields;
            size_t num_fields;
        } struct_def;
        struct AstObjectLiteral {
            Str struct_name;
            struct AstObjectField {
                Str name;
                Ast *value;
            } *fields;
            size_t num_fields;
        } object_literal;
        struct AstTraitDef {
            Str name;
            struct TraitMethod {
                Str name;
                Str *params;  /// @todo is this const?
                size_t num_params;
                Ast *body;
            } *methods;
            size_t num_methods;
        } trait;
        struct AstIf {
            Ast *condition;
            Ast *body;
            Ast *else_body;
        } if_stmt;
        struct AstLoop {
            Ast **bindings;
            size_t num_bindings;
            Ast *body;
            Ast *condition;
            bool yields;
        } loop;
        struct AstLet {
            Ast **bindings;
            size_t num_bindings;
            Ast *body;  /// @todo rename this
        } let_stmt;
        struct AstBinding {
            Str identifier;
            Str assign_op;
            Ast *expression;
        } binding;  /// @todo  :  go over all the inline bindings and replace with this
        struct AstReturn {
            Ast *value;
        } return_stmt;
        struct AstMatch {
            Ast *expression;
            struct MatchCase {
                Ast *condition;
                Ast *expression;
                Str kind; /// @todo -- remove
            } *cases;
            size_t num_cases;
        } match;
        struct AstDict {
            struct AstDictEntry {
                Ast *key;
                Ast *value;
            } *elements;
            size_t size;
        } dict;
        struct AstVec {
            Ast **elements;
            size_t size;
        } vec;  /// @todo  : change to vec
        struct AstUse {
            Str module_path;
            Str alias;
            bool wildcard;
        } use_stmt;
        struct AstMutation {
            Ast *target;
            Str op;
            Ast *value;
            bool is_broadcast;
        } mutation;
        struct AstConst {
            Str name;
            Ast *value;
        } const_stmt;
        struct AstYield {
            Ast *value;
        } yield_stmt;
        struct AstIgnore {
            Ast *null_expression;
        } ignore_stmt;
        struct AstExpression {
            Ast *expression;
        } exp_stmt;
        struct AstModule {
            Str name;
        } mod_stmt;
    };
};


Str Ast_to_str(Ast *node);
void Ast_print(Ast *ast);
Str Ast_to_json(Ast *node, size_t indent) ;
Str Ast_node_to_json(Ast *node, size_t indent) ;
Str Ast_dict_to_json(Ast *node, size_t indent) ;
Str Ast_vec_to_json(Ast *node, size_t indent) ;
Str Ast_use_to_json(Ast *node, size_t indent) ;
Str Ast_ignore_to_json(Ast *node, size_t indent) ;
Str Ast_yield_to_json(Ast *node, size_t indent) ;
Str Ast_expression_to_json(Ast *node, size_t indent) ;
Str Ast_module_to_json(Ast *node, size_t indent) ;
Str Ast_struct_def_to_json(Ast *node, size_t indent) ;
Str Ast_const_to_json(Ast *node, size_t indent) ;
Str Ast_trait_to_json(Ast *node, size_t indent) ;
Str Ast_object_literal_to_json(Ast *node, size_t indent) ;
Str Ast_block_to_json(Ast *node, size_t indent) ;
Str Ast_if_to_json(Ast *node, size_t indent) ;
Str Ast_fn_def_to_json(Ast *node, size_t indent) ;
Str Ast_fn_call_to_json(Ast *node, size_t indent) ;
Str Ast_binary_op_to_json(Ast *node, size_t indent) ;
Str Ast_unary_op_to_json(Ast *node, size_t indent) ;
Str Ast_literal_to_json(Ast *node, size_t indent) ;
Str Ast_identifier_to_json(Ast *node, size_t indent) ;
Str Ast_fn_anon_to_json(Ast *node, size_t indent) ;
Str Ast_method_call_to_json(Ast *node, size_t indent) ;
Str Ast_member_access_to_json(Ast *node, size_t indent) ;
Str Ast_let_to_json(Ast *node, size_t indent) ;
Str Ast_loop_to_json(Ast *node, size_t indent) ;
Str Ast_for_to_json(Ast *node, size_t indent) ;
Str Ast_binding_to_json(Ast *node, size_t indent) ;
Str Ast_return_to_json(Ast *node, size_t indent) ;
Str Ast_match_to_json(Ast *node, size_t indent) ;
Str Ast_mutation_to_json(Ast *node, size_t indent) ;

#pragma endregion
//#endregion

//#region Parsing_Context
#pragma region ParseContext

typedef struct ParseContext ParseContext;
typedef struct Source Source;
typedef struct Lexer Lexer;
typedef struct Parser Parser;

struct ParseContext {
    struct Source {
        struct Str *lines;
        size_t size;
    } source;

    struct Lexer {
        struct {
            size_t   size;
            size_t   capacity;
        } meta;

        struct Token *elements;
        Str      indent;
    } lexer;

    struct Parser {
        struct {
            size_t  size;
            size_t  capacity;
        } meta;
        struct Ast *elements;
        size_t depth; 
        size_t current_token; 
        CallStack traces;
    } parser;


} __glo_parsing_context = {0};


#define parser_update_ast(node) \
    __glo_parsing_context.parser.meta.size++;\
    __glo_parsing_context.parser.elements = node

#define pctx() (&__glo_parsing_context)
#define pctx_trace(arg) \
    Tracer_push(&pctx()->parser.traces, s(__func__), arg, s(__FILE__), __LINE__)

/// @todo rationalize this better, eg., wrt sizeof's
#define PARSING_PREALLOC (128 * 1024)

void parser_ctx_anew(ParseContext *pctx, Arena *lexer, Arena *parser) {
    require_not_null(pctx);
    require_not_null(lexer);
    require_not_null(parser);

    array_init(anew, pctx->lexer, PARSING_PREALLOC);
    array_init(anew, pctx->parser, PARSING_PREALLOC);
}


#pragma endregion
//#endregion

//#region Parsing_Header
#pragma region ParsingHeader
void lex(Lexer *lex, Source *src);
bool lex_is_keyword(Str s);

void parse(ParseContext *pctx);
// void parse_print_ast(Ast *ast);
// void parse_print_Ast_node(Ast *ast, size_t depth);
// void parse_Ast_from_source(ParseContext *p, Source *src);
void parse_error(ParseContext *pctx, Token *token, Str message);

Token *parse_consume_type(ParseContext *p, Str value, TokenType type, Str error_message);
Ast *parse_statement(ParseContext *p);
// uint8_t parse_peek_precedence(ParseContext *p);
Ast *parse_expression(ParseContext *p, size_t precedence);
Ast *parse_expression_part(ParseContext *p);
Ast *parse_keyword_statement(ParseContext *p,bool in_expr);
Ast *parse_identifier(ParseContext *p);
Ast *parse_macro_def(ParseContext *p);
Ast *parse_loop(ParseContext *p);
Ast *parse_for(ParseContext *p);
Ast *parse_if(ParseContext *p);
Ast *parse_binding(ParseContext *p);
Ast **parse_bindings(ParseContext *p, size_t *out_num_bindings);
void     parse_post_bindings(ParseContext *p,Ast *node);
Ast *parse_block(ParseContext *p);
Ast *parse_bracketed(ParseContext *p);
// Ast *parse_dereference(ParseContext *p);
Ast *parse_unary(ParseContext *p);
Ast *parse_literal(ParseContext *p);
Ast *parse_post_call(ParseContext *p,Ast *calleeNode);
Ast *parse_object_literal(ParseContext *p);
Ast *parse_match(ParseContext *p);
Ast *parse_vec(ParseContext *p);
Ast *parse_dict(ParseContext *p);
Ast *parse_return(ParseContext *p);
Ast *parse_yield(ParseContext *p);
Ast *parse_ignore(ParseContext *p);
Ast *parse_trait(ParseContext *p);
Ast *parse_struct(ParseContext *p);
Ast *parse_use(ParseContext *p);
Ast *parse_let(ParseContext *p);
Ast *parse_const(ParseContext *p);
Ast *parse_keyword(ParseContext *p,Str keyword, AstType type);
Ast *parse_function(ParseContext *p,bool is_loop);
Ast *parse_post_anon(ParseContext *p);
Ast *parse_mutation(ParseContext *p);

#pragma endregion
//#endregion


/// @headerfile native.h

//#region Native_BoxHeader 
#pragma region BoxHeader 

/// @note Box is the internal type of elements for the interpreter
///     NULL, INT, BOOL, FLOAT are actually all unboxed (the payload is the value)
///     all others are boxed (the payload is a ptr to the value)
///     A distinction is made between BOX_ collections and DYN_ collections
///         the latter have dynamic capacities, (todo?) always heap allocated
///         the former have fixed capacities and (todo?) should always been on an arena
///         and, at least, are emitted and used by the interpreter internally
///             eg., function args are packed into BoxArrays

/// @note UNBOXED TYPES GO HERE, cf. `interpreter.h`


// max 16 types
typedef enum BoxType {
    /// @brief Unboxed types 
    ///     (ie.,  copy/return-safe, copy-by-value, fixed size, no alloc/GC)
    BT_STATE = 0, 
    BT_INT = 1,
    BT_FLOAT = 2,
    BT_TAG = 3, /// @note a tag is a 12-char (unboxed) string


    /// @brief Boxed struct types
    ///    (ie., copy/return unsafe, copy-by-ref, arena allocated, no GC)
    /// Typically created by the interpreter for internal use
    /// Two relevant arenas: global and local
    ///  Function stack frames are allocated on the local arena
    ///  Global variables and constants are allocated on the global arena
    ///     The global arena is never freed, 
    ///         the local arena is freed on function exit.
    BT_BOX_PTR = 4,     // generic internal pointer 
    BT_BOX_STRING = 5,  
    BT_BOX_ARRAY = 6,   //eg., function arguments
    BT_BOX_DICT = 7,    //eg., scopes, function locals
    BT_BOX_STRUCT = 8,
    BT_BOX_FN = 9,

    /// @brief Garbage collected types
    ///    (ie., copy-by-ref, heap allocated, GC)
    BT_DYN_DOUBLE = 10,
    BT_DYN_OBJECT = 11,
    BT_DYN_STR = 12,
    BT_DYN_COLLECTION = 13,
    BT_DYN_FN = 14,

    /// @brief Error type
    BT_BOX_ERROR = 15,
} BoxType;

Str bt_nameof(BoxType bt) {
    static const char *names[] = {
        [BT_STATE] = "unbox(state)",
        [BT_INT] = "unbox(int)",
        [BT_FLOAT] = "unbox(float)",
        [BT_TAG] = "unbox(tag)",
        [BT_DYN_DOUBLE] = "box(double)",
        [BT_BOX_STRING] = "box(string)",
        [BT_BOX_ARRAY] = "box(array)",
        [BT_BOX_DICT] = "box(dict)",
        [BT_BOX_STRUCT] = "box(struct)",
        [BT_BOX_FN] = "box(fn)",
        [BT_DYN_OBJECT] = "box(dyn object)",
        [BT_DYN_STR] = "box(dyn string)",
        [BT_DYN_COLLECTION] = "box(dyn collection)",
        [BT_DYN_FN] = "box(dyn fn)",
        [BT_BOX_ERROR] = "box(error)",
    };

    return Str_from_cstr(names[bt]);
}

/// @brief For boxing pointers as (Number|Pointer) types
typedef struct Box {
    uint64_t payload : 60;      
    uint8_t type : 4;           
} Box;

typedef enum BoxStateType {
    BS_NULL = 0,
    BS_FALSE = 'f',
    BS_TRUE = 't',
    BS_UNKNOWN = 'u',
    BS_MISSING = '?',
    BS_CONTINUE = 'C',
    BS_BREAK = 'B',
    BS_RETURN ='R',
    BS_YIELD = 'Y',
    BS_DONE = 'D',
    BS_GOTO = 'G',
    BS_HALT = 'H',
    BS_ENUM_MAX = 'Z'
    /// @note maybe add suspend, resume, etc.
} BoxStateType;

Str st_nameof(BoxStateType cs) {
    static const char *names[] = {
        [BS_NULL] = "null",
        [BS_BREAK] = "break",
        [BS_CONTINUE] = "continue",
        [BS_RETURN] = "return",
        [BS_YIELD] = "yield",
        [BS_DONE] = "done",
        [BS_GOTO] = "goto",
        [BS_HALT] = "exit"
    };

    return Str_from_cstr(names[cs]);
}


Str Box_to_str(Box b);

bool Box_try_numeric(Box box, double *out_value);

size_t Box_hash(Box key);
bool Box_eq(Box a, Box b);
void Box_adel(Box b);

#pragma endregion
//#endregion

//#region Native_ComplexBoxTypes
#pragma region ComplexBoxTypes


typedef enum BoxErrorType {
    RT_ERR_NONE = 0,
    RT_ERR_NATIVE = 1,
    RT_ERR_INTERPRETER,
    RT_ERR_NOT_IMPLEMENTED,
} BoxErrorType;

typedef struct BoxError {
    BoxErrorType code;         
    Str message;    
    Str location;

    /// @brief  
    /// @todo 
    struct BoxErrorDetails {
        Str user_file;
        size_t user_line;
        size_t user_col;
    } info;            
} BoxError;

typedef struct BoxKvPair {
    size_t hash;
    Str key; 
    Box value;
    struct BoxKvPair *next; // Pointer to the next entry in the chain
} BoxKvPair;


// typedef struct BoxHashMap {
//     size_t size;      
//     BoxKvPair **elements; // Array of pointers to BoxKvPair (chains)
//     size_t capacity;        // Number of elements
// } BoxHashMap;

typedef struct BoxDict {
    struct {
        size_t size;  
        size_t capacity;        // Number of elements
    } meta;
    BoxKvPair **elements; // Array of pointers to BoxKvPair (chains)
} BoxDict;



#define BoxDict_iter_items(fd, entry) for(size_t __i = 0; __i < cap(fd); __i++)\
    for(BoxKvPair *entry = fd.elements[__i]; entry != NULL; entry = entry->next)

typedef struct BoxArray {
    struct {
        size_t size;      // Current number of elements
        size_t capacity;  // Total capacity of the array
    } meta;
    Box *elements;    // Array of Box elements
} BoxArray;


typedef enum DynCollectionType {
    /// @brief generic collection type
    DYNCOL_ARRAY = 'a',
    DYNCOL_QUEUE = 'q',
    DYNCOL_STACK = 'k',
    DYNCOL_DICT = 'd',      /// @note (Str, Box)
    DYNCOL_SET = 's',

    DYNCOL_VEC_BOOL = 'B',
    DYNCOL_VEC_DOUBLE = 'D',
    DYNCOL_VEC_INT = 'I',
    DYNCOL_VEC_STR = 'S',
    DYNCOL_VEC_CHAR = 'C',
} DynCollectionType;

typedef struct DynCollection {
    HeapMetaGc gc;
    MetaData meta;
    void *elements;
} DynCollection;

#define dyn_size(dc) (len_ref(dc))
#define dyn_capacity(dc) (capacity_ref(dc))
#define dyn_type(dc) (dc->meta.type)




/// @brief A generic collection type
/// @note Supports vector (random access), set, queue and stack APIs
///     ... API methods check the `type` to ensure its correct
typedef struct DynArray {
    HeapMetaGc gc;
    MetaData meta;
    Box *elements;          
} DynArray;

typedef struct DynDictEntry {
    size_t hash;
    Box key;
    Box value;
    bool occupied;  
} DynDictEntry;

typedef struct DynDict {
    HeapMetaGc gc;
    MetaData meta;
    DynDictEntry *elements;        // Array of dictionary elements
} DynDict;


typedef struct DynSet {
    HeapMetaGc gc;
    MetaData meta;
    Box *elements;          // Array of Box elements for separate chaining
} DynSet;


// Structure representing a dynamic string
typedef struct DynStr {
    HeapMetaGc gc;
    MetaData meta;
    char *cstr;       // Pointer to the string data (null-terminated)
} DynStr;

DynStr *DynStr_from_Str_gcnew(Str str);


typedef struct DynVecDouble {
    HeapMetaGc gc;
    MetaData meta;
    double *elements;          
} DynVecDouble;

typedef struct DynVecInt {
    HeapMetaGc gc;
    MetaData meta;
    int *elements;          
} DynVecInt;

typedef struct DynVecStr {
    HeapMetaGc gc;
    MetaData meta;
    Str *elements;          
} DynVecStr;

typedef struct DynVecCat {
    HeapMetaGc gc;
    MetaData meta;
    int *elements;
    Str *categories;
} DynVecCat;

DynVecCat DynVecCat_gcnew(size_t num_categories);
DynDict DynVecCat_histogram(DynVecCat *cat);
Str DynVecCat_to_str(DynVecCat *cat);



typedef struct DynVecChar {
    HeapMetaGc gc;
    MetaData meta;
    char *elements;          
} DynVecChar;

typedef struct DynVecBool {
    HeapMetaGc gc;
    MetaData meta;
    bool *elements;          
} DynVecBool;


typedef struct BoxScope {
    struct BoxScope *parent;
    BoxDict locals;
    Str doc_string;
} BoxScope;


typedef struct BoxStruct {
    Str name;
    Str doc_string;
    BoxDict fields;
    BoxScope namespace;
} BoxStruct;


/// @brief 
/// @todo consider whether there should be object types
///     .... eg., class elements, internal elements... cf. PyObject
typedef struct DynObject {
    HeapMetaGc gc;
    MetaData meta;

    /// @todo -- maybe de-nest 
    BoxStruct *data;
} DynObject;

typedef struct BoxIterator {
    size_t iter_index;
    size_t iter_from;
    size_t iter_to;
    BoxFn iter_fn;
    Box iter_value;
    Box iter_key;
    Box iter_state;
    Box iter_result;
} BoxIterator;

Box BoxIterator_update(BoxIterator *iter, Box result);
Box BoxIterator_get(BoxIterator *iter);

typedef enum BoxFnType {
    FN_USER = 'u',
    FN_MACRO = 'm',
    FN_ASYNC = 'a',
    FN_LOOP = 'l',
    FN_GENERATOR = 'g',
    FN_NATIVE = 'n',
    FN_NATIVE_0 = '0',
    FN_NATIVE_1 = '1',
    FN_NATIVE_2 = '2',
    FN_NATIVE_3 = '3',
} BoxFnType;

typedef struct BoxFn  {
    Str name; 
    BoxDict signature;
    BoxScope enclosure; 
    BoxFnType type;
    void *fnptr;
    void *code;
} BoxFn;


typedef struct DynFn {
    HeapMetaGc gc;
    MetaData meta;
    BoxFn callable;
} DynFn;

#define BoxFnFromNative(fn_type, fn_name, fn_native_ptr) (BoxFn) {\
    .type = fn_type, .name = fn_name,\
    .fnptr = (void *)fn_native_ptr, .code = NULL, \
    .signature = {0}, .enclosure = {0} }

// typedef Box (*BoxFnUser)(BoxFn *fn, BoxScope *parent, BoxArray *args);
typedef Box (*BoxFnGenerator)(BoxFn *fn, BoxScope parent, BoxArray args, BoxIterator iter);
// typedef Box (*BoxFnAsync)(BoxFn *fn, BoxScope *parent, BoxArray *args);
// typedef Box (*BoxFnMacro)(BoxFn *fn, BoxScope *parent, BoxArray *args);
typedef Box (*BoxFnUser)(BoxFn *fn, BoxScope parent, BoxArray args);
typedef Box (*BoxFnNative)(BoxFn *fn, BoxScope parent, BoxArray args);
typedef Box (*BoxFnNative0)(BoxFn *fn, BoxScope parent); 
typedef Box (*BoxFnNative1)(BoxFn *fn, BoxScope parent, Box one);
typedef Box (*BoxFnNative2)(BoxFn *fn, BoxScope parent, Box one, Box two);
typedef Box (*BoxFnNative3)(BoxFn *fn, BoxScope parent, Box one, Box two, Box three);


#define BoxFn_typed_call(type, ffn, ...) (((type) ffn->fnptr)(ffn, __VA_ARGS__))


#define BoxFn_is_native(ffn) (ffn->type == FN_NATIVE) ||     \
    (ffn->type == FN_NATIVE_0) || (ffn->type == FN_NATIVE_1) \
    (ffn->type == FN_NATIVE_2) || (ffn->type == FN_NATIVE_3)



// typedef struct BoxGenFn  {
//     BoxFn next;
//     BoxIterator iter;
// } BoxGenFn;

/// @note castable to BoxFn
typedef struct BoxAsyncFn  {
    BoxFn next;
    /// @todo etc.
} BoxAsyncFn;

#pragma endregion
//#endregion ComplexBoxTypes

//#region Native_BoxApiHeader
#pragma region BoxApiHeader


#define BoxScope_empty(dstr) (BoxScope){.locals = {0}, .parent = NULL, .doc_string = dstr}

#define BoxScope_define_local(scope_ptr, name, value)\
    native_log_debug(s("Defining in Scope: %.*s"), fmt((scope_ptr)->doc_string));\
    BoxDict_set(&((scope_ptr)->locals), name, value)

int BoxArray_test_main();
int BoxDict_test_main();
int DynArray_test_main();
int DynDict_test_main();
int BoxScope_test_main();
Box BoxFn_test_native2(BoxFn *fn, BoxScope parent, Box one, Box two);
int BoxFn_test_main();
void test_wrap_unwrap_double();
void test_wrap_unwrap_float();
void Box_test_main();
void interp_init();
bool interp_precache_getstr(Str str, BoxKvPair *out);
void interp_precache_setstr(Str str);
void interp_precache_test_main();



bool Box_eq(Box a, Box b);
Str Box_to_str(Box b);
bool Box_try_numeric(Box box, double *out_value);
size_t Box_hash(Box box);


Str DynCollection_to_repr(DynCollection *col);

void BoxArray_elements_anew(BoxArray *array, size_t size);
Str BoxArray_to_repr(BoxArray *array);
Box BoxArray_set(BoxArray *box_array, Box element, size_t index);
Box BoxArray_append(BoxArray *array, Box element);
BoxArray *BoxArray_anew(size_t fixed_capacity);
Box BoxArray_get(BoxArray array, size_t index);
Box BoxArray_overwrite(BoxArray *array, size_t index, Box element);
Box BoxArray_weak_unset(BoxArray *array, size_t index);

DynDict *DynDict_gcnew(size_t initial_capacity);
bool DynDict_find(DynDict *dict, Box key, DynDictEntry *out);
void DynDict_adel(DynDict *dict);
Box DynDict_set(DynDict *dict, Box key, Box val);
DynArray *DynArray_gcnew(size_t initial_capacity);
Box DynArray_append(DynArray *array, Box element);
Box DynArray_get(const DynArray *array, size_t index);
Box DynArray_overwrite(DynArray *array, size_t index, Box element);
Box DynArray_weak_remove(DynArray *array, size_t index);
void DynArray_adel(DynArray *array);
void DynArray_debug_print(const DynArray *array);
Str DynObject_to_repr(DynObject *obj);
Box DynObject_get_attribute(DynObject *obj, Str name);
Box DynObject_get_attribute_checked(DynObject *obj, Str name, BoxType expected_type);
Box DynObject_resolve_method(Box obj, BoxScope *scope, Str name);
Str BoxDict_to_repr(BoxDict *d);
void BoxDict_elements_anew(BoxDict *dict, size_t size);
Box BoxDict_get(BoxDict *dict, Str key);
Str BoxDict_keys_to_str(BoxDict *dict);
void BoxDict_debug_print_entry(BoxKvPair *entry);
void BoxDict_debug_print(BoxDict dict);
bool BoxDict_find(BoxDict *dict, Str key, BoxKvPair *out);
BoxKvPair *BoxKvPair_anew(Str key, Box value);
void BoxDict_from_pairs(BoxDict *empty_dict, BoxKvPair **pairs, size_t num_pairs);
void BoxDict_set(BoxDict *dict, Str key, Box value);
BoxKvPair *BoxDict_remove(BoxDict *dict, Str key);
void BoxDict_adel(BoxDict *dict);
void BoxScope_locals_anew(BoxScope *self, BoxScope *parent);
void BoxScope_sized_anew(BoxScope *self, BoxScope *parent, size_t num_locals);
void BoxScope_debug_print(BoxScope scope);
void BoxScope_merge_local(BoxScope *dest, BoxScope *src);
void BoxDict_merge(BoxDict *dest, BoxDict *src);
bool BoxScope_lookup(BoxScope *scope, Str name, Box *out_value);
bool BoxScope_has(BoxScope *scope, Str name);
void BoxScope_adel(BoxScope *scope);
static BoxFn *BoxFn_anew(BoxFnType type, Str name, 
    BoxDict args_defaults, BoxScope enclosure, void *fnptr, void *code);
Box BoxFn_call(BoxFn *ffn, BoxScope scope, BoxArray args);
BoxStruct *BoxStruct_anew(Str name, size_t num_fields);
DynObject *DynObject_gcnew(BoxStruct *def);
DynObject *DynObject_from_BoxDict(BoxStruct *def, BoxDict fields);
void DynObject_set_field(DynObject *obj, Str field, Box value);
bool DynObject_get_field(DynObject *obj, Str field, Box *out_value);

BoxArray *cliargs_as_BoxArray(int argc, char **argv);
double lib_rand_0to1();
double lib_rand_normal(double mean, double stddev);
Box native_log(BoxFn *self, BoxScope parent, BoxArray args);
Box native_range(BoxFn *self, BoxScope parent, BoxArray args);
Box native_infer(BoxFn *self, BoxScope parent, Box loopFn, Box strategyTag);
Box native_sample(BoxFn *self, BoxScope parent, Box distObject);
Box native_take(BoxFn *self, BoxScope parent, BoxArray args);
void native_add_prelude(BoxScope scope);


void BoxArray_elements_anew(BoxArray *array, size_t capacity);
Box BoxArray_set(BoxArray *box_array, Box element, size_t index);
Box BoxArray_get(BoxArray array, size_t index);
Box BoxArray_weak_unset(BoxArray *box_array, size_t index);
void BoxArray_adel(BoxArray *box_array);
void BoxArray_debug_print(Str name, BoxArray *box_array);
Str BoxArray_to_repr(BoxArray *array);

DynStr* DynStr_init(Heap *heap, const char *initial_str);
Box DynStr_join_anew(DynStr *a, char *data, size_t len);
const char* DynStr_as_cstr(const DynStr *dyn_str);
Box DynStr_set(DynStr *dyn_str, const char *str);
void DynStr_adel(DynStr *dyn_str);
void DynStr_print(const DynStr *dyn_str);


DynArray* DynArray_init(size_t initial_capacity);
void DynArray_adel(DynArray *array);
void DynArray_debug_print(const DynArray *array);

Box DynArray_to_set(DynArray *array);
Box DynArray_to_stack(DynArray *array);
Box DynArray_to_queue(DynArray *array);
Box DynArray_to_vector(DynArray *array);
Box DynArray_to_BoxArray(DynArray *array);
Box DynArray_clear(DynArray *array);
Box DynArray_contains_byhash(DynArray *array);
Box DynArray_contains_byvalue(DynArray *array);
Box DynArray_append(DynArray *array, Box element);
Box DynArray_append_first(DynArray *array);
Box DynArray_append_last(DynArray *array);
Box DynArray_append_offer(DynArray *array, Box element);
Box DynArray_append_unique(DynArray *array, Box element);
Box DynArray_get(const DynArray *array, size_t index);
Box DynArray_set_add(DynArray *array, size_t index, Box element);
Box DynArray_set_overwrite(DynArray *array, size_t index, Box element);

Box DynArray_remove_weak_byindex(DynArray *array, size_t index);
Box DynArray_remove_weak_byvalue(DynArray *array, Box element);
Box DynArray_remove_weak_byhash(DynArray *array, Box element);

Box DynArray_remove_strong_byindex(DynArray *array, size_t index);
Box DynArray_remove_strong_byvalue(DynArray *array, Box element);
Box DynArray_remove_strong_byhash(DynArray *array, Box element);

Box DynArray_is_empty(const DynArray *array);
Box DynArray_first(DynArray *array);
Box DynArray_remove_first(DynArray *array);
Box DynArray_last(DynArray *array);
Box DynArray_remove_last(DynArray *array);
Box DynArray_indexof(DynArray *array);


void BoxDict_elements_anew(BoxDict *dict, size_t num_entires);
void BoxDict_set(BoxDict *dict, Str key, Box value);
void BoxDict_set(BoxDict *dict, Str key, Box value);
Box BoxDict_get(BoxDict *dict, Str key);
bool BoxDict_find(BoxDict *dict, Str key, BoxKvPair *out);
void BoxDict_merge(BoxDict *dest, BoxDict *src);
Str BoxDict_to_repr(BoxDict *array);

Str DynObject_to_repr(DynObject *obj);

bool BoxScope_lookup(BoxScope *scope, Str name, Box *out_value);

#pragma endregion
//#endregion

//#region Native_BoxMacroHelpers
#pragma region BoxMacroHelpers
/// @todo -- check which of these are not in use and remove

/// @brief Used to map box types to c-types at compile-time (ie., via macros).
// typedef void BT_NULL_T;
typedef int BT_INT_T;
// typedef bool BT_BOOL_T;
typedef float BT_FLOAT_T;
typedef Str BT_TAG_T;
typedef double * BT_DYN_DOUBLE_T;
typedef Str * BT_BOX_STRING_T;
typedef BoxArray * BT_BOX_ARRAY_T;
typedef BoxDict * BT_BOX_DICT_T;
typedef BoxStruct * BT_BOX_STRUCT_T;
typedef BoxError * BT_BOX_ERROR_T;
typedef DynObject * BT_DYN_OBJECT_T;
typedef BoxFn * BT_BOX_FN_T;
typedef DynFn * BT_DYN_FN_T;
typedef DynStr * BT_DYN_STR_T;
typedef DynDict * BT_DYN_DICT_T;
typedef DynArray * BT_DYN_ARRAY_T;
typedef uint8_t BT_STATE_T;

#define Box_ctypeof(BoxTypeName) BoxTypeName##_T

#define Box_unwrap(BoxTypeName, box) ((BoxTypeName##_T) box.payload)

#define Box_unwrap_int(box) ((int64_t) box.payload)
#define Box_unwrap_bool(box) ((bool) (box.payload == BS_TRUE))
#define Box_unwrap_ptr(type, box) ((type *) box.payload)
#define Box_is_bool(box) (box.type == BT_STATE && (box.payload == BS_TRUE || box.payload == BS_FALSE))
// #define Box_unwrap_Box_ptr(bx_ptr) Box_unwrap_ptr(Box, bx_ptr)

#define Box_unwrap_error(box) Box_unwrap_ptr(BoxError, box)
#define Box_unwrap_double(box) *Box_unwrap_ptr(double, box)
#define Box_unwrap_str(box) *Box_unwrap_ptr(Str, box)
#define Box_unwrap_array(box) Box_unwrap_ptr(BoxArray, box)
#define Box_unwrap_dict(box) Box_unwrap_ptr(BoxDict, box)
#define Box_unwrap_iter(box) Box_unwrap_ptr(BoxIterator, box)
#define Box_unwrap_fn(box) Box_unwrap_ptr(BoxFn, box)
#define Box_unwrap_gen(box) Box_unwrap_ptr(BoxGenFn, box)
#define Box_unwrap_object(box) Box_unwrap_ptr(DynObject, box)
#define Box_unwrap_struct(box) Box_unwrap_ptr(BoxStruct, box)
#define Box_unwrap_scope(box) Box_unwrap_ptr(BoxScope, box)
#define Box_unwrap_DynCollection(box) Box_unwrap_ptr(DynCollection, box)
#define Box_unwrap_DynArray(box) Box_unwrap_ptr(DynArray, box)
#define Box_unwrap_DynDict(box) Box_unwrap_ptr(DynDict, box)
#define Box_unwrap_DynStr(box) Box_unwrap_ptr(DynStr, box)


#define Box_wrap_int(int_value) ((Box) {.payload = (uint64_t) int_value, .type=BT_INT})
#define Box_wrap_bool(bool_value) ((Box) {.payload = (uint64_t) (bool_value ? BS_TRUE : BS_FALSE), .type=BT_STATE})
#define Box_wrap_ptr(ptr_type, ptr_value) ((Box) {.payload = (uint64_t) ptr_value, .type=ptr_type})

#define Box_wrap_error(ptr) Box_wrap_ptr(BT_BOX_ERROR, ptr)
#define Box_wrap_double(ptr) Box_wrap_ptr(BT_DYN_DOUBLE, ptr)
#define Box_wrap_str(ptr) Box_wrap_ptr(BT_BOX_STRING, ptr)
#define Box_wrap_array(ptr) Box_wrap_ptr(BT_BOX_ARRAY, ptr)
#define Box_wrap_fn(ptr) Box_wrap_ptr(BT_BOX_FN, ptr)
#define Box_wrap_object(ptr) Box_wrap_ptr(BT_DYN_OBJECT, ptr)
#define Box_wrap_struct(ptr) Box_wrap_ptr(BT_BOX_STRUCT, ptr)
#define Box_wrap_dict(ptr) Box_wrap_ptr(BT_BOX_DICT, ptr)
#define Box_wrap_DynCollection(ptr) Box_wrap_ptr(BT_DYN_COLLECTION, ptr)
#define Box_wrap_DynStr(ptr) Box_wrap_ptr(BT_DYN_STR, ptr)
#define Box_wrap_BoxIterator(ptr) Box_wrap_ptr(BT_BOX_PTR, ptr)
#define Box_unwrap_BoxIterator(box) Box_unwrap_ptr(BoxIterator, box)

static inline Box Box_wrap_float(float float_value) {
  uint64_t payload = 0;
  memcpy(&payload, &float_value, sizeof(float));
  return (Box){.payload = payload, .type = BT_FLOAT};
}

static inline float Box_unwrap_float(Box box) {
  float f; memcpy(&f, &box, sizeof(float)); return f;
}


Box Box_wrap_tag(Str tag_string) {
    Box box = {0}; // Initialize all fields to 0

    // Ensure the tag size does not exceed 11 characters
    // if (tag_string.size > 11) {
    //     native_error("Tag '%.*s' exceeds maximum size of 11 characters (truncating).", fmt(tag_string));
    // }

    size_t max_size = tag_string.size > 11 ? 11 : tag_string.size;
    uint64_t encoded = 0;

    for (size_t i = 0; i < max_size; ++i) {
        char c = toupper(tag_string.cstr[i]); // Convert to uppercase
        int value = char_to_base37(c);
        encoded = encoded * 37 + value;
    }

    box.payload = encoded;
    box.type = BT_TAG;
    return box;
}

Str Box_unwrap_tag(Box tag) {
    uint64_t encoded = tag.payload;
    char temp[12]; 
    size_t index = 0;

    while (encoded > 0 && index < 11) {
        uint8_t value = encoded % 37;
        encoded /= 37;
        temp[index++] = base37_to_char(value);
    }

    if (index == 0) {
        // native_error("Tags cannot be empty.");
        temp[index++] = '_'; // Assuming '_' represents an empty tag
    }

    // Reverse the characters since the least significant digit was decoded first
    for (size_t i = 0; i < index / 2; ++i) {
        char swap = temp[i];
        temp[i] = temp[index - i - 1];
        temp[index - i - 1] = swap;
    }

    temp[0] = '#';
    return Str_anew(temp, index);
}


bool Box_tag_eq(Box tag, Str value) {
    if (tag.type != BT_TAG) {
        return false;
    }

    const size_t MAX_LENGTH = 11;
    uint64_t encoded = 0;
    size_t encode_size = value.size > MAX_LENGTH ? MAX_LENGTH : value.size;

    for (size_t i = 0; i < encode_size; ++i) {
        char c = toupper((unsigned char)value.cstr[i]);
        int val = char_to_base37(c);
        encoded = encoded * 37 + val;
    }

    // If the provided string has fewer than MAX_LENGTH characters,
    // it's implicitly padded with zeros (leading characters treated as lower significance)
    // To ensure consistent encoding, we need to shift the encoded value accordingly.
    // However, since encoding processes from left to right, shorter strings have smaller encoded values.

    // To accurately compare, encode the provided string and compare directly
    return (encoded == tag.payload);
}


#define unBox_checked(box, ub_type)\
    Box_is_ptr(box) ? Box_unwrap_##ub_type(Box_unwrap_ptr(box)) : Box_error()

#define box(box, ub_type) Box_unwrap_##ub_type(Box_unwrap_ptr(box)) 

#define Box_typeof(box) bt_nameof(box.type)

#define Box_null()  ((Box) {.payload = (uint64_t) BS_NULL, .type = BT_STATE})
#define Box_missing()  ((Box) {.payload = (uint64_t) BS_MISSING, .type = BT_STATE})
#define Box_state(cs_type) ((Box) {.payload = (uint64_t) cs_type, .type = BT_STATE})
#define Box_halt() Box_state(BS_HALT)
#define Box_true()  ((Box) {.payload = (uint64_t) BS_TRUE, .type = BT_STATE})
#define Box_false() ((Box) {.payload = (uint64_t) BS_FALSE, .type = BT_STATE})
#define Box_error_empty() ((Box) {.payload = (uint64_t) 0, .type = BT_BOX_ERROR})
#define Box_done() Box_state(BS_DONE)

// #define Box_is_ptr(box) (box.type == BT_BOX_PTR)
#define Box_is_error(box) (box.type == BT_BOX_ERROR)
#define Box_is_null(box) (box.type == BT_STATE && box.payload == BS_NULL)
#define Box_is_state(box, cs_type) (box.type == BT_STATE && box.payload == cs_type)
#define Box_is_state_end(box) (box.type == BT_STATE && \
    box.payload == BS_BREAK || box.payload == BS_RETURN || box.payload == BS_YIELD)

#define Box_is_hashof(boxTag, tagStr) (boxTag.type == BT_INT && ((size_t) Box_unwrap_int(boxTag) == Str_hash(tagStr)))

static inline bool Box_is_truthy(Box box) {
    return
        (box.type == BT_STATE && Box_unwrap_bool(box) != false) || 
        (box.type == BT_INT && Box_unwrap_int(box) != 0) || 
        (box.type == BT_FLOAT && Box_unwrap_float(box) != 0.0 ) ||  false;
        // (box.type == BT_BOX_PTR && box.payload != 0) 
}


#pragma endregion
//#endregion

//#region Native_BoxErrorTypeHeader
#pragma region BoxErrorTypeHeader

static BoxError __glo_error_stack[ARRAY_SIZE_SMALL] = {0};
static size_t __glo_error_stack_size = 0;

#define error_print_all() error_print_lastN(__glo_error_stack_size)

BoxError *error_push(BoxError error);
BoxError error_getlast(BoxArray errors);
void error_print_lastN(size_t n);
void error_print(BoxError error);


#define error_push_runtime(erc, erm, erl)\
    error_push((BoxError){.code = (erc), .message = (erm), .location = (erl)})

#define native_error(...) \
    error_push_runtime(RT_ERR_NATIVE, Str_fmt_anew(__VA_ARGS__), s(__FILE__ ":" _STR(__LINE__)))



#pragma endregion
//#endregion

//#region Native_sHeader
#pragma region NativesHeader
void native_add_prelude(BoxScope scope);

// typedef Box (*BoxFnNative)(BoxFn *fn, BoxScope parent, BoxArray args);
// typedef Box (*BoxFnNative0)(BoxFn *fn, BoxScope parent); 
// typedef Box (*BoxFnNative1)(BoxFn *fn, BoxScope parent, Box one);
// typedef Box (*BoxFnNative2)(BoxFn *fn, BoxScope parent, Box one, Box two);
// typedef Box (*BoxFnNative3)(BoxFn *fn, BoxScope parent, Box one, Box two, Box three);
Box native_log(BoxFn *self, BoxScope parent, BoxArray args);


/// @todo make these non-variadic
/// @note useful native prototypes 
Box native_print(BoxFn *self, BoxScope parent, BoxArray args);
Box native_input(BoxFn *self, BoxScope parent, BoxArray args);
Box native_len(BoxFn *self, BoxScope parent, Box arg);
Box native_range(BoxFn *self, BoxScope parent, BoxArray args);
Box native_sum(BoxFn *self, BoxScope parent, BoxArray args);
Box native_max(BoxFn *self, BoxScope parent, BoxArray args);
Box native_min(BoxFn *self, BoxScope parent, BoxArray args);
Box native_abs(BoxFn *self, BoxScope parent, BoxArray args);
Box native_pow(BoxFn *self, BoxScope parent, BoxArray args);
Box native_sqrt(BoxFn *self, BoxScope parent, BoxArray args);
Box native_floor(BoxFn *self, BoxScope parent, BoxArray args);
Box native_ceil(BoxFn *self, BoxScope parent, BoxArray args);
Box native_round(BoxFn *self, BoxScope parent, BoxArray args);

#pragma endregion
//#endregion


/// @headerfile interpreter.h

//#region Interpreter_ErrorHeader 
#pragma region InterpErrorHeader 

#ifdef INTERP_DEBUG_VERBOSE
    #define interp_log_debug(...) log_message(LL_DEBUG, __VA_ARGS__)
#else
    #define interp_log_debug(...)
#endif

#define interp_error(...) \
    error_push((BoxError){\
        .code = RT_ERR_INTERPRETER,\
        .message = Str_fmt_anew(__VA_ARGS__),\
        .location = s(__FILE__ ":" _STR(__LINE__))});\
    interp_graceful_exit()

#pragma endregion
//#endregion

//#region Interpreter_BoxedPrecache
#pragma region BoxedPrecache


struct BoxPreCache {
    /// @todo since ints, floats are just copy-inside-boxes, we dont need to intern
    // ... but doubles & strings would benefit 

    // Box small_positive_integers[256]; // -
    // Box small_negative_integers[32]; // -
    // Box interned_doubles[256]; /// @todo consider this
    BoxDict interned_strings;
} __glo_precache = {0};

#define interp_precache() __glo_precache

#pragma endregion
//#endregion

//#region Interpreter_EvaluatorHeader
#pragma region InterpreterEvaluatorHeader

void interp_graceful_exit();

// Running code
Box interp_run_from_source(ParseContext *p, Str source, int argc, char **argv);
Box interp_run_ast(Ast *ast);

Box interp_eval_if(Ast *node, BoxScope *scope);
Box interp_eval_vec(Ast *node, BoxScope *scope);
Box interp_eval_literal(Ast *node, BoxScope *scope);
Box interp_eval_identifier(Ast *node, BoxScope *scope);
Box interp_eval_call_fn(Ast *node, BoxScope *scope);
Box interp_eval_block(Ast *node, BoxScope *scope);
Box interp_eval_fn_def(Ast *node, BoxScope *scope);
Box interp_eval_loop(Ast *node, BoxScope *scope);

Box interp_eval_loop_with_condition(Ast *node, BoxScope *scope);
Box interp_eval_loop_with_streams(Ast *node, BoxScope *scope);
Box interp_eval_loop_with_infinite(Ast *node, BoxScope *scope);
Box interp_eval_for(Ast *node, BoxScope *scope);
Box interp_eval_expression(Ast *node, BoxScope *scope);
Box interp_eval_const_def(Ast *node, BoxScope *scope);
Box interp_eval_let_def(Ast *node, BoxScope *scope);
Box interp_eval_binary_op(Ast *node, BoxScope *scope);
Box interp_eval_unary_op(Ast *node, BoxScope *scope);
Box interp_eval_match(Ast *node, BoxScope *scope);
Box interp_eval_return(Ast *node, BoxScope *scope);
Box interp_eval_mutation(Ast *node, BoxScope *scope);

Box interp_eval_dict(Ast *node, BoxScope *scope);
Box interp_eval_method_call(Ast *node, BoxScope *scope);
Box interp_eval_member_access(Ast *node, BoxScope *scope);
Box interp_eval_module(Ast *node, BoxScope *scope);
Box interp_eval_fn_anon(Ast *node, BoxScope *scope);
Box interp_eval_use(Ast *node, BoxScope *scope);
Box interp_eval_struct_def(Ast *node, BoxScope *scope);
Box interp_eval_object_literal(Ast *node, BoxScope *scope);

Box interp_eval_load_module(Ast *node);

#pragma endregion
//#endregion

//#region Interpreter_MinimalRuntimeHeader
#pragma region InterpreterMinimalRuntimeHeader
Box interp_eval_fn(Box fn_obj, int num_args, Box *args);
Box interp_eval_uop(Str op, Box operand);
Box interp_eval_bop(Str op, Box left, Box right);

// void interp_eval_def(Str name, Box value, BoxScope *scope);
// bool interp_eval_lookup(Str name, BoxScope *scope, Box *out_value);
Box interp_eval_ast(Ast *node, BoxScope *scope);
Box BoxFn_Ast_call(BoxFn *fn, BoxArray args, BoxScope parent);

#pragma endregion
//#endregion



///---------- ---------- ---------- IMPLEMENTATION ---------- ---------- ----------//


/// @file internal.c

//#region Internal_LoggingImpl
#pragma region LoggingImpl

void logging_test_main() {
    printf("Running Logging Tests...\n");
    log_message(LL_INFO, s("This is an info message."));
    log_message(LL_WARNING, s("This is a warning message."));
    log_message(LL_DEBUG, s("This is a debug message."));
    log_message(LL_RECOVERABLE, s("This is a recoverable message."));
    log_message(LL_TEST, s("This is a test message."));
    
    /// @todo this doesn't work atm, fix it 
    
    // #ifdef DEBUG_ABORT_ON_ERROR
    //     #undef DEBUG_ABORT_ON_ERROR
    //     log_message(LL_ERROR, s("This is an error message."));
    //     #define DEBUG_ABORT_ON_ERROR
    // #else
    //     log_message(LL_ERROR, s("This is an error message."));
    // #endif

    // #ifdef DEBUG_ABORT_ON_ASSERT
    //     #undef DEBUG_ABORT_ON_ASSERT
    //     log_message(LL_ASSERT, s("This is an assert message."));
    //     #define DEBUG_ABORT_ON_ASSERT
    // #else
    //     log_message(LL_ASSERT, s("This is an assert message."));
    // #endif
    
    printf("Logging Tests Completed.\n\n");
}


#pragma endregion
//#endregion Preamble_LoggingImpl

//#region Internal_StringImpl
#pragma region StringImpl

/// @todo rename to indicate allocators vs. views vs. borrows/etc.
/// eg. Str_part_view(), Str_lines_anew()

Str Str_anew(const char *cstr, size_t len) {
    if (!cstr) return Str_empty();

    char *buffer =  (char *) anew(len);

    if (!buffer) { error_oom(); return Str_empty(); }

    memcpy(buffer, cstr, len);
    return (Str) {.cstr = buffer, .size = len};
}

// Str Str_gcnew(const char *cstr) {
//     if (!cstr) return Str_empty();

//     size_t len = strlen(cstr);
//     char *buffer = (char *) gcnew(len);

//     if (!buffer) { error_oom(); return Str_empty(); }

//     memcpy(buffer, cstr, len);
//     return (Str) {.cstr = buffer, .size = len};
// }

Str Str_cnew_from(Str str) {
    if (Str_is_empty(str)) return Str_empty();

    char *buffer = (char *) cnew(str.size);
    if (!buffer) { error_oom(); return Str_empty(); }

    memcpy(buffer, str.cstr, str.size);
    return (Str) {.cstr = buffer, .size = str.size};
}

Str Str_col_anew(const char *color_code, Str str) {
    size_t color_len = strlen(color_code);
    size_t reset_len = strlen(ANSI_COL_RESET);
    size_t new_size = color_len + str.size + reset_len;

    // Allocate memory for the new colored string
    char *buffer = anew(new_size + 1);
    if (!buffer) {
        error_oom();
        return Str_empty();
    }

    memcpy(buffer, color_code, color_len);
    memcpy(buffer + color_len, str.cstr, str.size);
    memcpy(buffer + color_len + str.size, ANSI_COL_RESET, reset_len);
    buffer[new_size] = '\0';

    Str colored_str = { .cstr = buffer, .size = new_size };
    return colored_str;
}


/// @brief Trims leading and trailing repeat delimiters from a string
/// ....  eg., 'quote' -> quote
/// ....  eg., ''quote'' -> quote
/// ....  eg., """quote""" -> quote
/// @param str 
/// @return 
Str Str_trim_delim(Str str) {
    if (str.size < 2) return str;

    while (str.cstr[0] == str.cstr[str.size - 1]) {
        str = (Str) {.cstr = str.cstr + 1, .size = str.size - 2};
    }
    return str;
}

Str Str_firstN(Str str, size_t n) {
    if (n > str.size) {
        n = str.size;
    }
    return (Str) {.cstr = str.cstr, .size = n};
}

Str Str_read_file_anew(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        log_message(LL_ERROR, s("Failed to open file: "), s(filename));
        return Str_empty();
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *) anew(size + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    size_t read = fread(buffer, 1, size, file);
    buffer[read] = '\0';

    fclose(file);
    return (Str) {.cstr = buffer, .size = read};
}



bool Str_starts_with(Str str, Str prefix) {
    if (str.size < prefix.size) return false;
    return memcmp(str.cstr, prefix.cstr, prefix.size) == 0;
}

size_t Str_hash(Str str) {
    const uint64_t FNV_offset_basis = 14695981039346656037UL;
    const uint64_t FNV_prime = 1099511628211UL;
    size_t hash = FNV_offset_basis;

    for (size_t i = 0; i < str.size; i++) {
        hash ^= (unsigned char)(str.cstr[i]);
        hash *= FNV_prime;
    }

    return hash;
}

bool Str_is_empty(Str str) {
    return (str.size == 0) || (str.cstr == NULL);
}

/// @brief 
/// @param str 
/// @return
/// @todo investigate this impl 
double Str_to_double(Str str) {
    return atof(str.cstr);
}

int Str_to_int(Str str) {
    return atoi(str.cstr);
}

Str Str_part(Str line, size_t start, size_t end) {
    return (Str) {.cstr = line.cstr + start, .size = end - start};
} 

Str Str_ncopy(Str str, size_t start, size_t end) {
    return Str_anew(str.cstr + start, end - start);
}

Str Str_copy(Str old) {
    return Str_anew(old.cstr, old.size);
}

Str Str_readline_anew(size_t max) {
    char *buffer = (char *) anew(max + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    if (!fgets(buffer, max, stdin)) {
        return Str_empty();
    }

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    return (Str) {.cstr = buffer, .size = len};
}
// Print a Str to stdout
void Str_puts(Str str) {
    if (str.cstr && str.size > 0) {
        printf("%.*s", (int)str.size, str.cstr);
    } else {
        puts("(NULL)");
    }
}

#define Str_print(...) Str_array_print(\
    ((Str[]) { __VA_ARGS__ }), sizeof(((Str[]) { __VA_ARGS__ }))/sizeof(Str)\
)

void Str_array_print(Str *array, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (Str_is_empty(array[i])) {
            puts("(NULL)");
        } else {
            printf("%.*s ", fmt(array[i]));
        }
    }
    printf("\n");
}



void Str_println(Str str) {
    if (str.cstr && str.size > 0) {
        printf("%.*s\n", (int)str.size, str.cstr);
    } else {
        puts("(NULL)\n");
    }
}
// Print formatted string to stdout
void Str_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

// Create a Str from a single character
/// @todo is static alloc of buffers etc. here right?
Str Str_chr(char chr) {
    // Using a static buffer to hold the character
    static char buffer[1];
    buffer[0] = chr;
    return (Str){ .cstr = buffer, .size = 1 };
}


// Concatenate two Str instances
Str Str_glue_anew(Str left, Str right) {
    if(left.size == 0 && right.size == 0) return Str_empty();
    if(right.size == 0) return left;
    if(left.size == 0) return right;

    size_t new_len = left.size + right.size;
    char *buffer = (char *) anew(new_len);
    if (!buffer) { error_oom(); return Str_empty(); }

    memcpy(buffer, left.cstr, left.size);
    memcpy(buffer + left.size, right.cstr, right.size);
    buffer[new_len] = '\0';

    return (Str){ .cstr = buffer, .size = new_len };
}

Str Str_glue_sep_anew(Str left, Str sep, Str right) {
    size_t new_len = left.size + sep.size + right.size;
    char *buffer = (char *) anew(new_len);
    if (!buffer) { error_oom(); return Str_empty(); }

    memcpy(buffer, left.cstr, left.size);
    memcpy(buffer + left.size, sep.cstr, sep.size);
    memcpy(buffer + left.size + sep.size, right.cstr, right.size);
    buffer[new_len] = '\0';

    return (Str){ .cstr = buffer, .size = new_len };
}


// Check if two Str instances are equal
bool Str_eq(Str left, Str right) {
    if (left.size != right.size) return false;
    return memcmp(left.cstr, right.cstr, left.size) == 0;
}


inline bool Str_eq_cstr(Str left, const char *right) {
    return Str_eq(left, Str_from_cstr(right));
}

Str Str_join_anew(Str one, ...) {
    va_list args;
    va_start(args, one);

    size_t total_len = one.size;
    Str current = one;
    while (current.size > 0) {
        total_len += current.size;
        current = va_arg(args, Str);
    }

    char *buffer = (char *) anew(total_len + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    memcpy(buffer, one.cstr, one.size);
    size_t offset = one.size;

    current = one;
    while (current.size > 0) {
        memcpy(buffer + offset, current.cstr, current.size);
        offset += current.size;
        current = va_arg(args, Str);
    }

    buffer[total_len] = '\0';
    va_end(args);

    return (Str){ .cstr = buffer, .size = total_len };
} 

char *cstr_repeat_anew(const char *cstr, size_t count) {
    size_t len = strlen(cstr);
    size_t new_len = len * count;
    char *buffer = (char *) anew(new_len + 1);
    if (!buffer) { error_oom(); return NULL; }

    for (size_t i = 0; i < count; i++) {
        memcpy(buffer + i * len, cstr, len);
    }
    buffer[new_len] = '\0';

    return buffer;
}

bool write_bytes_to_file(const char *filename, const char *bytes, size_t len) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        log_message(LL_ERROR, s("Failed to open file: "), s(filename));
        return false;
    }

    size_t written = fwrite(bytes, 1, len, file);
    fclose(file);

    return written == len;
}

#define MAX_FILE_SIZE (1 << 30) // 1 GB

unsigned char *read_bytes_from_file_cnew(const char *filename, size_t *out_len) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        log_message(LL_ERROR, s("Failed to open file: "), s(filename));
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if(size < 0 || (size_t) size > MAX_FILE_SIZE) {
        fclose(file);
        log_message(LL_ERROR, s("File too large or invalid size: %ld"), size);
        return false;
    }

    unsigned char *buffer = (unsigned char *) cnew(size);
    if (!buffer) { error_oom(); return NULL; }

    size_t read = fread(buffer, 1, size, file);
    fclose(file);

    if (out_len) *out_len = read;
    return buffer;
}

int char_to_base37(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A'; // 'A' -> 0, 'B' -> 1, ..., 'Z' -> 25
    } else if (c >= '0' && c <= '9') {
        return 26 + (c - '0'); // '0' -> 26, ..., '9' -> 35
    } else if (c == '_') {
        return 36; // '_' -> 36
    } else {
        return 36; // map invalids to _
    }
}

char base37_to_char(uint8_t value) {
    if (value <= 25) {
        return 'A' + value; // 0 -> 'A', ..., 25 -> 'Z'
    } else if (value <= 35) {
        return '0' + (value - 26); // 26 -> '0', ..., 35 -> '9'
    } else if (value == 36) {
        return '_'; // 36 -> '_'
    } else {
        return '?'; // Placeholder for invalid values
    }
}


// Helper function to create indentation string
Str Str_repeat_anew(Str str, size_t count) {
    if(count == 0) return Str_empty();
    size_t new_len = str.size * count;
    char *buffer = (char *) anew(new_len);
    if (!buffer) { error_oom(); return Str_empty(); }

    for (size_t i = 0; i < count; i++) {
        memcpy(buffer + i * str.size, str.cstr, str.size);
    }
    buffer[new_len] = '\0';

    return (Str){ .cstr = buffer, .size = new_len };
}

Str Str_fmt_anew(Str format, ...) {
    va_list args;
    va_start(args, format);
    Str result = Str_fmt_va_anew(format, args);
    va_end(args);
    return result;
}


Str Str_fmt_va_anew(Str format, va_list args) {
    /// @note why do we need to copy the args?
    /// @note why do we need to call vsnprintf twice?
    /// ... because we need to know the size of the string to allocate the buffer
    /// ... and then we need to actually write the string to the buffer
    /// ... so we need to call vsnprintf twice

    va_list args_copy;
    va_copy(args_copy, args);

    int len = vsnprintf(NULL, 0, format.cstr, args);
    if (len < 0) {
        return Str_empty();
    }

    char *buffer = (char *) anew(len + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    vsnprintf(buffer, len + 1, format.cstr, args_copy);
    va_end(args_copy);

    return (Str){ .cstr = buffer, .size = len };
}

// Check if Str equals a single character
bool Str_eq_chr(Str left, char right) {
    return (left.size == 1) && (left.cstr[0] == right);
}

// Get a substring from a Str
Str Str_sub(Str str, size_t start, size_t size) {
    if (start >= str.size) return Str_empty();
    if (start + size > str.size) {
        size = str.size - start;
    }
    return (Str){ .cstr = str.cstr + start, .size = size };
}


bool Str_contains(Str str, Str part) {
    return Str_index_of(str, part, NULL);
}

bool Str_index_of(Str str, Str part, size_t *out_index) {
    if (part.size == 0) {
        return false;
    }
    if (part.size > str.size) {
        return false;
    }

    for (size_t i = 0; i <= str.size - part.size; ++i) {
        if (memcmp(str.cstr + i, part.cstr, part.size) == 0) {
            if(out_index) *out_index = i;
            return true;
        }
    }
    return false;
}

bool Str_lAst_index(Str str, Str part, size_t *out_index) {
    if (part.size == 0) {
        return false;
    }

    if (part.size > str.size) {
        return false;
    }

    for (size_t i = str.size - part.size + 1; i-- > 0;) {
        if (memcmp(str.cstr + i, part.cstr, part.size) == 0) {
            if(out_index) *out_index = i;
            return true;
        }
    }

    return false;
}


// Find the index of a character in Str
int Str_index_of_chr(Str str, char chr) {
    for (size_t i = 0; i < str.size; i++) {
        if (str.cstr[i] == chr) return (int)i;
    }
    return -1;
}

// Find the last index of a character in Str
int Str_lAst_index_of_chr(Str str, char chr) {
    for (size_t i = str.size; i > 0; i--) {
        if (str.cstr[i-1] == chr) return (int)(i-1);
    }
    return -1;
}

// Replace all occurrences of a character in Str
Str Str_replace(Str str, char target, char replacement) {
    char *buffer = (char *) anew(str.size + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    size_t new_len = 0;
    for (size_t i = 0; i < str.size; i++) {
        buffer[new_len++] = (str.cstr[i] == target) ? replacement : str.cstr[i];
    }
    buffer[new_len] = '\0';

    return (Str){ .cstr = buffer, .size = new_len };
}

// Trim whitespace from both ends of Str
Str Str_trim(Str str) {
    size_t start = 0;
    while (start < str.size && isspace((unsigned char)str.cstr[start])) start++;
    
    size_t end = str.size;
    while (end > start && isspace((unsigned char)str.cstr[end - 1])) end--;
    
    return Str_sub(str, start, end - start);
}

// Convert Str to uppercase
Str Str_upper(Str str) {
    char *buffer = (char *) anew(str.size + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    for (size_t i = 0; i < str.size; i++) {
        buffer[i] = toupper((unsigned char)str.cstr[i]);
    }
    buffer[str.size] = '\0';

    return (Str){ .cstr = buffer, .size = str.size };
}

// Convert Str to lowercase
Str Str_lower(Str str) {
    char *buffer = (char *) anew(str.size + 1);
    if (!buffer) { error_oom(); return Str_empty(); }

    for (size_t i = 0; i < str.size; i++) {
        buffer[i] = tolower((unsigned char)str.cstr[i]);
    }
    buffer[str.size] = '\0';

    return (Str){ .cstr = buffer, .size = str.size };
}

// Compare two Str instances lexicographically
int Str_cmp(Str left, Str right) {
    size_t min_len = left.size < right.size ? left.size : right.size;
    int cmp = memcmp(left.cstr, right.cstr, min_len);
    if (cmp != 0) return cmp;
    if (left.size < right.size) return -1;
    if (left.size > right.size) return 1;
    return 0;
}

// Compare two Str instances using natural ordering
int Str_cmp_natural(Str left, Str right) {
    size_t i = 0, j = 0;
    while (i < left.size && j < right.size) {
        if (isdigit(left.cstr[i]) && isdigit(right.cstr[j])) {
            // Extract numbers
            long num1 = strtol(left.cstr + i, NULL, 10);
            long num2 = strtol(right.cstr + j, NULL, 10);
            if (num1 < num2) return -1;
            if (num1 > num2) return 1;
            // Advance i and j past the number
            while (i < left.size && isdigit((unsigned char)left.cstr[i])) i++;
            while (j < right.size && isdigit((unsigned char)right.cstr[j])) j++;
        } else {
            if (left.cstr[i] < right.cstr[j]) return -1;
            if (left.cstr[i] > right.cstr[j]) return 1;
            i++;
            j++;
        }
    }
    if (i < left.size) return 1;
    if (j < right.size) return -1;
    return 0;
}

// Append a character to Str
Str Str_join_chr_anew(Str str, char chr) {
    char *buffer = (char *) anew(str.size + 2);
    if (!buffer) { error_oom(); return Str_empty(); }
    memcpy(buffer, str.cstr, str.size);
    buffer[str.size] = chr;
    buffer[str.size + 1] = '\0';
    return (Str){ .cstr = buffer, .size = str.size + 1 };
}

// Append a C-string to Str
Str Str_join_cstr_anew(Str str, const char *cstr) {
    size_t cstr_len = strlen(cstr);
    size_t new_len = str.size + cstr_len;
    char *buffer = (char *) anew(new_len + 1);
    if (!buffer) { error_oom(); return Str_empty(); }
    memcpy(buffer, str.cstr, str.size);
    memcpy(buffer + str.size, cstr, cstr_len);
    buffer[new_len] = '\0';
    return (Str){ .cstr = buffer, .size = new_len };
}

// Check if Str contains a character
bool Str_contains_chr(Str str, char chr) {
    for (size_t i = 0; i < str.size; i++) {
        if (str.cstr[i] == chr) return true;
    }
    return false;
}


// Split Str by a delimiter, returns an array of Str and sets out_count
Str *Str_split_chr(Str str, char delimiter, size_t *out_count) {
    size_t count = 1;
    for (size_t i = 0; i < str.size; i++) {
        if (str.cstr[i] == delimiter) count++;
    }

    Str *result = (Str *) anew(sizeof(Str) * count);
    if (!result) { error_oom(); *out_count = 0; return NULL; }

    size_t start = 0;
    size_t idx = 0;
    for (size_t i = 0; i < str.size; i++) {
        if (str.cstr[i] == delimiter) {
            size_t len = i - start;
            result[idx++] = (Str){ .cstr = str.cstr + start, .size = len };
            start = i + 1;
        }
    }
    // Last segment
    result[idx++] = (Str){ .cstr = str.cstr + start, .size = str.size - start };
    *out_count = count;
    return result;
}

// Split Str into lines by a delimiter (typically '\n')
Str *Str_lines(Str str, size_t *out_count) {
    return Str_split_chr(str, '\n', out_count);
}

// Free memory allocated for Str
void Str_adel(Str str) {
    if (str.cstr) {
        adel((void*)str.cstr);
    }
}


// Simple test suite for the Str library
int string_test_main(void) {
    // Test Str_from_cstr
    Str hello = Str_from_cstr("Hello");
    assert(hello.size == 5);
    assert(Str_eq(hello, s("Hello")));

    // Test Str_glue_anew
    Str world = s(" World");
    Str hello_world = Str_glue_anew(hello, world);
    assert(hello_world.size == 11);
    assert(Str_eq(hello_world, s("Hello World")));
    
    // Test Str_print
    printf("Str_print: ");
    Str_println(hello_world); // Should print "Hello World"
    printf("\n");
    
    // Test Str_eq
    assert(Str_eq(hello, s("Hello")));
    assert(!Str_eq(hello, s("hello")));
    
    // Test Str_eq_chr
    Str nl = s("\n");
    assert(Str_eq_chr(nl, '\n'));
    assert(!Str_eq_chr(nl, 'h'));
    
    // Test Str_trim
    Str padded = s("  Hello  ");
    Str trimmed = Str_trim(padded);
    assert(Str_eq(trimmed, hello));
    
    // Test Str_upper and Str_lower
    Str upper = Str_upper(hello);
    assert(Str_eq(upper, s("HELLO")));

    Str lower = Str_lower(upper);
    assert(Str_eq(lower, s("hello")));
    
    // Test Str_replace
    Str replaced = Str_replace(hello_world, ' ', '_');
    assert(Str_eq(replaced, s("Hello_World")));
    
    // Test Str_contains
    assert(Str_contains_chr(hello_world, 'W'));
    assert(!Str_contains_chr(hello_world, 'w'));
    
    assert(Str_contains(hello_world, s("World")));
    assert(!Str_contains(hello_world, s("world")));

    // Test Str_split_chr
    size_t count;
    Str *parts = Str_split_chr(s("Hello World"), ' ', &count);
    assert(count == 2);
    assert(Str_eq(parts[0], s("Hello")));
    assert(Str_eq(parts[1], s("World")));
    adel(parts);
    
    // Test Str_cmp
    assert(Str_cmp(s("A"), s("A")) == 0);
    assert(Str_cmp(s("B"), s("A")) > 0);
    assert(Str_cmp(s("A"), s("B")) < 0);
    
    // Test Str_cmp_natural
    Str file2 = s("file2");
    Str file10 = s("file10");
    assert(Str_cmp_natural(file2, file10) < 0);
    assert(Str_cmp_natural(file10, file2) > 0);
    assert(Str_cmp_natural(file2, Str_from_cstr("file2")) == 0);
    
    // Test Str_join_chr_anew
    Str appended_char = Str_join_chr_anew(hello, '!');
    assert(Str_eq(appended_char, Str_from_cstr("Hello!")));
    Str_adel(appended_char);
    
    // Test Str_join_cstr_anew
    Str appended_cstr = Str_join_cstr_anew(hello, ", everyone");
    assert(Str_eq(appended_cstr, Str_from_cstr("Hello, everyone")));
    Str_adel(appended_cstr);
    
    // All tests passed
    printf("All string tests passed.\n");
    return 0;
}

#pragma endregion
//#endregion Preamble_StringImpl

//#region Internal_ArenaImpl
#pragma region ArenaImpl

#ifdef DEBUG_MEMORY
    #define mem_debug_log(...)\
        log_message(LL_DEBUG, __VA_ARGS__);
#else
    #define mem_debug_log(msg, ...) NOOP
#endif

    
static bool _Arena_block_cnew(Arena *a, size_t size) {
    require_not_null(a);
    require_positive(size);

    mem_debug_log(sMSG("Block usage: %zu/%zu"), a->used, a->block_size);

    size_t alloc_size = (size > a->block_size) ? size : a->block_size;

    void *block = cnew(alloc_size);
    if (!block) {
        error_oom();
        return false;
    }

    if (a->num_blocks % 32 == 0) {
        size_t new_capacity = a->num_blocks + 32;

        void **new_blocks = cextend(a->blocks, new_capacity * sizeof(void *));
        if (!new_blocks) { 
            error_oom(); 
            free(block); 
            return false; 
        }

        size_t *new_block_sizes = cextend(a->block_sizes, new_capacity * sizeof(size_t));
        if (!new_block_sizes) {
            error_oom();
            free(block);
            return false;
        }
        
        a->blocks = new_blocks;
        a->block_sizes = new_block_sizes;
    }

    a->used = 0;
    a->blocks[a->num_blocks] = block;
    a->block_sizes[a->num_blocks] = alloc_size;
    a->num_blocks++;

    mem_debug_log(sMSG("Block allocation complete. Number of blocks: %zu"), a->num_blocks);

    return true;
}

bool Arena_init(Arena *a, size_t block_size) {
    require_null(a->blocks);    // ensure it hasn't been init'd already
    require_not_null(a);
    require_positive(block_size);

    a->block_size = block_size;
    a->used = 0;
    a->num_blocks = 0;
    a->blocks = NULL;

    return _Arena_block_cnew(a, block_size);
}



unsigned char *Arena_save_cnew(Arena *a, size_t *out_size) {
    require_not_null(a);
    require_not_null(out_size);

    size_t total_size = 0;
    for (size_t i = 0; i < a->num_blocks; i++) {
        total_size += a->block_sizes[i];
    }

    unsigned char *data = cnew(total_size);

    if (!data) {
        error_oom();
        return NULL;
    }

    size_t offset = 0;
    for (size_t i = 0; i < a->num_blocks; i++) {
        memcpy(data + offset, a->blocks[i], a->block_sizes[i]);
        offset += a->block_sizes[i];
    }

    *out_size = total_size;
    return data;
}

bool Arena_load_cnew(Arena *a, unsigned char *data, size_t size) {
    require_not_null(a);
    require_not_null(data);
    require_positive(size);

    if (a->blocks) {
        log_message(LL_WARNING, s("Arena already contains data. Deallocating existing data."));
        Arena_adel_underlying(a);
    }

    a->block_size = size;
    a->used = size;
    a->num_blocks = 1;
    a->blocks = (void **) cnew(sizeof(void *));
    a->block_sizes = (size_t *) cnew(sizeof(size_t));

    if (!a->blocks || !a->block_sizes) {
        error_oom();
        return false;
    }

    a->blocks[0] = data;
    a->block_sizes[0] = size;

    return true;
}


void Arena_adel(void *p) {
    /// @note compiler hint to ignore unused parameter
    (void)p;
    return; //no-op
}

void Arena_adel_underlying(Arena *a) {
    require_not_null(a);

    for(size_t i = 0; i < a->num_blocks; i++) {
        free(a->blocks[i]);
    }
    free(a->blocks);

    a->blocks = NULL;
    a->num_blocks = 0;
}

void *Arena_anew(Arena *a, size_t size) {
    require_not_null(a);
    require_positive(size);
    
    // Align to 8 bytes 
    size = (size + 7) & ~((size_t)7);
    mem_debug_log(sMSG("Allocating size: %zu (aligned size: %zu)"), size, size);

    if ((a->used + size) > a->block_size) {
        mem_debug_log(sMSG("Growing blocks"));
        if (!_Arena_block_cnew(a, size)) { 
            error_oom(); 
            return NULL; 
        }
    }

    void *ptr = (char *)a->blocks[a->num_blocks - 1] + a->used;
    a->used += size;
    return ptr;
}



/// @brief Not needed since Arena_anew() already reallocates if needed
/// @param a 
/// @param ptr 
/// @param old_size 
/// @param new_size 
/// @return 
void *Arena_cextend(Arena *a, void *ptr, size_t old_size, size_t new_size) {
    require_not_null(a);
    require_positive(new_size);

    // Align sizes to 8 bytes
    old_size = (old_size + 7) & ~((size_t)7);
    new_size = (new_size + 7) & ~((size_t)7);

    mem_debug_log(sMSG("Reallocating ptr: %p, old size: %zu, new size: %zu"), ptr, old_size, new_size);

    // If the new size is less than or equal to the old size, no reallocation is needed
    if (new_size <= old_size) {
        return ptr;
    } else {
        mem_debug_log(sMSG("Reallocation performing memcpy."));
    }

    // Allocate new memory for the larger size
    void *new_ptr = Arena_anew(a, new_size);
    if (!new_ptr) {
        error_oom();
        return NULL;
    }

    // Copy the old data to the new location
    if (ptr) {
        memcpy(new_ptr, ptr, old_size);
    }

    mem_debug_log(sMSG("Reallocation complete. New ptr: %p"), new_ptr);
    return new_ptr;
}


void Arena_reset(Arena *a) {
    require_not_null(a);
    a->used = 0;
}

/// @todo: maybe Arena_reset_withzero()

void Arena_test_main() {
    Arena a = {0};

    Str_println(s("Test 1: Initialization"));
    log_assert(Arena_init(&a, 1024), sMSG("Failed to initialize arena"));
    log_assert(a.block_size == 1024, sMSG("Block size mismatch after initialization"));
    log_assert(a.num_blocks == 1, sMSG("Initial block not allocated"));
    log_assert(a.used == 0, sMSG("Used memory should be 0 after initialization"));

    Str_println(s("Test 2: Small allocations\n"));
    void *p1 = Arena_anew(&a, 100);
    log_assert(p1 != NULL, sMSG("Failed to allocate small block"));
    log_assert(a.used == 104, sMSG("Used memory mismatch after small allocation (aligned to 8 bytes)"));
    
    void *p2 = Arena_anew(&a, 200);
    log_assert(p2 != NULL, sMSG("Failed to allocate second small block"));
    log_assert(a.used == 304, sMSG("Used memory mismatch after multiple small allocations"));

    Str_println(s("Test 3: Block boundary crossing\n"));
    void *p3 = Arena_anew(&a, 800);
    log_assert(p3 != NULL, sMSG("Failed to allocate large block crossing boundary"));
    log_assert(a.num_blocks == 2, sMSG("New block not allocated for boundary crossing"));
    log_assert(a.used == 800, sMSG("Used memory mismatch in new block"));

    Str_println(s("Test 4: Oversized allocation\n"));
    void *p4 = Arena_anew(&a, 2048);
    log_assert(p4 != NULL, sMSG("Failed to allocate oversized block"));
    log_assert(a.num_blocks == 3, sMSG("New block not allocated for oversized block"));
    log_assert(a.block_sizes[2] == 2048, sMSG("Oversized block not allocated with correct size"));

    Str_println(s("Test 5: Reset\n"));
    Arena_reset(&a);
    log_assert(a.used == 0, sMSG("Used memory should be 0 after reset"));
    log_assert(a.num_blocks == 3, sMSG("Number of blocks should remain the same after reset"));

    Str_println(s("Test 6: Reallocation\n"));
    void *p5 = Arena_anew(&a, 64);
    log_assert(p5 != NULL, sMSG("Failed to allocate initial memory for reallocation"));
    memset(p5, 0xAA, 64); // Fill with test data

    void *p5_cextend = Arena_cextend(&a, p5, 64, 128);
    log_assert(p5_cextend != NULL, sMSG("Failed to reallocate to larger size"));
    log_assert(p5_cextend != p5, sMSG("Reallocated pointer should be different if block size exceeded"));

    void *p6 = Arena_cextend(&a, p5_cextend, 128, 64);
    log_assert(p6 == p5_cextend, sMSG("Reallocation to smaller size should return the same pointer"));

    Str_println(s("Test 7: Boundary reallocation\n"));
    void *p7 = Arena_anew(&a, 900);
    log_assert(p7 != NULL, sMSG("Failed to allocate block near boundary"));

    void *p7_cextend = Arena_cextend(&a, p7, 900, 1100);
    log_assert(p7_cextend != NULL, sMSG("Failed to reallocate block that crosses boundary"));
    log_assert(a.num_blocks > 3, sMSG("Additional block not allocated for boundary-crossing reallocation"));

    Str_println(s("Test 8: Free underlying memory\n"));
    Arena_adel_underlying(&a);
    log_assert(a.blocks == NULL, sMSG("Blocks should be freed"));
    log_assert(a.num_blocks == 0, sMSG("Number of blocks should be 0 after freeing"));

    Str_println(s("All tests passed successfully.\n"));
}


#pragma endregion
//#endregion Preamble_ArenaImpl

//#region Internal_HeapImpl
#pragma region HeapImpl



void Heap_test_main() {
    Heap h = {0};
    Heap_elements_cnew(&h, 16, 8);

    // Test 1: Initialization
    log_assert(cap(h) == 16, sMSG("Initial capacity should be 16"));
    log_assert(len(h) == 0, sMSG("Initial size should be 0"));
    log_assert(h.gc_threshold == 8, sMSG("Initial GC threshold should be 8"));
    log_assert(h.generation_count == 0, sMSG("Initial generation count should be 0"));
    log_assert(h.elements != NULL, sMSG("Heap elements array should be initialized"));
}

#pragma endregion
//#endregion Preamble_HeapImpl

//#region Internal_TracebackImpl
#pragma region TracebackImpl

#define Tracer_print_last(ts) (Tracer_print_stack(ts, 16))

void Tracer_print_stack(CallStack *ts, size_t depth) {
    log_message(LL_INFO, s("Traceback (most recent call last):\n"));
    for (int i = ts->top - 1; i >= 0 && depth--; i--) {
        Str arg = Str_is_empty(ts->elements[i].argument) ?
            s("(null)") : ts->elements[i].argument;

        log_message(LL_INFO, s("   %.*s:%d\t\tat %.*s(' %.*s ')"),
            fmt(ts->elements[i].file), ts->elements[i].line,
            fmt(ts->elements[i].fn_name), fmt(arg)
        );
    }

    if (depth > 0) {
        log_message(LL_INFO,  sMSG("   <-- TRACEBACK FINISHED EARLY -->\n"));
    }
}
void Tracer_push(CallStack *traces, Str name, Str arg, Str file, size_t line) {
    if (traces->top >= 512) {
        log_message(LL_INFO, sMSG("Traceback overflow: cycling tracer history!\n"));

        #ifdef TRACEBACK_ABORT_ON_OVERFLOW
            Tracer_print_stack(traces, 16);
            abort();
        #endif
        
        traces->top = 0;
    }

    traces->elements[traces->top].fn_name = name;
    traces->elements[traces->top].argument = arg;
    traces->elements[traces->top].file = file;
    traces->elements[traces->top].line = line;
    traces->top++;
}

// Pop a function from the call stack
void Tracer_pop(CallStack *traces) {
    if (traces->top >= 0) {
        traces->top--;
    } else {
        log_message(LL_INFO, sMSG("Traceback underflow: Mis_matched push/pop calls!\n"));
    }
}


void Tracer_test_main() {
    printf("Running Traceback Tests...\n");
    
    CallStack cs = {0};
    
    // Push some elements
    Tracer_push(&cs, s("main"), s("arg1"), s("main.c"), 10);
    Tracer_push(&cs, s("foo"), s("arg2"), s("foo.c"), 20);
    Tracer_push(&cs, s("bar"), s("arg3"), s("bar.c"), 30);
    
    // Print the stack
    Tracer_print_stack(&cs, 10);
    
    // Pop an entry
    Tracer_pop(&cs);
    
    // Print the stack again
    Tracer_print_stack(&cs, 10);
    
    // Push another entry
    Tracer_push(&cs, s("baz"), s("arg4"), s("baz.c"), 40);
    
    // Print the stack
    Tracer_print_stack(&cs, 10);
    
    printf("Traceback Tests Completed.\n\n");
}


#pragma endregion
//#endregion Preamble_TracebackImpl

//#region Internal_PerformanceImpl
#pragma region PerformanceImpl



typedef struct TimePerfMetric {
    struct timespec start;
    struct timespec end;
} TimePerfMetric;

struct GlobalPerfMetrics {
    TimePerfMetric lAst_time;
    size_t memory;
    size_t cpu;
};

static struct GlobalPerfMetrics __glo_perf_metrics[ARRAY_SIZE_SMALL] = {0};
static size_t __glo_perf_index = 0;

#define perf_lAst_metric __glo_perf_metrics[__glo_perf_index]

void perf_track_memory(size_t memory) {
    perf_lAst_metric.memory = memory;
}

void perf_track_cpu(size_t cpu) {
    perf_lAst_metric.cpu = cpu;
}

void perf_cpu_usage() {
    /// @todo
}

void perf_time_start() {
    clock_gettime(CLOCK_MONOTONIC, &perf_lAst_metric.lAst_time.start);
}

void perf_time_end() {
    clock_gettime(CLOCK_MONOTONIC, &perf_lAst_metric.lAst_time.end);
}

void perf_time_report() {
    struct timespec start = perf_lAst_metric.lAst_time.start;
    struct timespec end = perf_lAst_metric.lAst_time.end;

    long seconds = end.tv_sec - start.tv_sec;
    long ns = end.tv_nsec - start.tv_nsec;
    if (start.tv_nsec > end.tv_nsec) {
        --seconds;
        ns += 1000000000;
    }

    printf("Time elapsed: %ld.%09ld seconds\n", seconds, ns);
}

#pragma endregion
//#endregion Preamble_PerformanceImpl

//#region Internal_CliImpl
#pragma region CliImpl


void cli_parse_args(int argc, char *argv[], CliOption *out_opts, size_t num_opts) {
    /// @note the first entry in out_opts will be the argv elements up until the first option
    /// ... defined by a leading `-` or `--`
    /// ... this is to allow for positional arguments to be parsed
    /// ... before the named options

    // Parse positional arguments
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') break;
        out_opts[0].value = Str_from_cstr(argv[i]);
        out_opts[0].is_set = true;
    }

    for (int i = 1; i < argc; i++) {
        for (size_t j = 0; j < num_opts; j++) {
            if (Str_eq(s(argv[i]), out_opts[j].name)) {
                out_opts[j].is_set = true;
                if (i + 1 < argc) {
                    out_opts[j].value = Str_from_cstr(argv[i + 1]);
                }
            }
        }
    }
}

void cli_print_opts(const CliOption options[], int num_options) {
    printf("Command-Line Options:\n");
    printf("----------------------------\n");
    for (int i = 0; i < num_options; i++) {
        printf("Option: %-10.*s | Set: %-5s | Value: %.*s | Default: %.*s\n",
               fmt(options[i].name),
               options[i].is_set ? "Yes" : "No",
               fmt(options[i].value),
               fmt(options[i].or_else));
    }
    printf("----------------------------\n");
}


#pragma endregion
//#endregion 

//#region Internal_TestImpl
#pragma region TestImpl


void TestReport_push(TestReport *report, bool result, Str message) {
    if (report->num_results > 32) {
        log_message(LL_ERROR, sMSG("Warning: TestSuite capacity exceeded."));
        return;
    }

    report->results[report->num_results] = result;
    report->messages[report->num_results] = Str_copy(message);
    report->num_results++;
}


void TestSuite_register(TestSuite *ts, UnitTestFnPtr fnptr) {
    if (ts->num_tests > 32) {
        log_message(LL_ERROR, sMSG("Warning: TestSuite capacity exceeded."));
        return;
    }

    ts->tests[ts->num_tests] = fnptr;
    ts->num_tests++;
}

void TestSuite_run(TestSuite *ts) {
    Arena_init(mem().arena_static, ARENA_1MB);
    ctx_change_arena(mem().arena_static);

    if (ts->num_tests > 32) {
        log_message(LL_ERROR, sMSG("Warning: TestSuite capacity exceeded."));
        return;
    }

    for (size_t i = 0; i < ts->num_tests; ++i) {
        ts->tests[i](&ts->reports[i]);
    }
}

void TestSuite_print(TestSuite *ts) {
    printf("=== Test Suite Report ===\n");

    for (size_t i = 0; i < ts->num_tests; ++i) {
        printf("Test #%zu:\n", i + 1);
        for (size_t j = 0; j < ts->reports[i].num_results; ++j) {
            printf("  [%s] %.*s\n", 
                ts->reports[i].results[j] ? "PASS" : "FAIL", 
                fmt(ts->reports[i].messages[j])
            );
        }
    }

    printf("=========================\n");
}


void unittest_sample_test_1(TestReport *r) {
    test_assert(r, 1 + 1 == 2, "Basic arithmetic works");
    test_assert(r, 2 * 2 == 5, "Deliberate failure example");
}

void unittest_sample_test_2(TestReport *r) {
    test_assert(r, strlen("test") == 4, "String size is correct");
}

int unittest_test_main() {
    TestSuite suite = test_suite(
        unittest_sample_test_1,
        unittest_sample_test_2
    );

    TestSuite_run(&suite);
    TestSuite_print(&suite);

    return 0;
}


#pragma endregion
//#endregion

//#region Preamble_TestMain
#pragma region TestMain

int internal_test_main(void) {
    logging_test_main();
    Arena_test_main();
    string_test_main();
    // cli_test_main();
    // Box_test_main();
    Heap_test_main();
    Tracer_test_main();
    unittest_test_main();
    return 0;
}

#pragma endregion
//#endregion


/// @file parsing.c

//#region Parsing_AstImpl
#pragma region ParsingAstImpl
#pragma endregion
//#endregion

//#region Parsing_LexerRules
#pragma region ParserLexerRules

#define lex_if_wordlike(c)\
        (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||\
        (c >= '0' && c <= '9') || (c == '_')

#define lex_if_numberlike(c)\
    (c >= '0' && c <= '9')

#define lex_comment_rule(c, d) \
    (c == '/') && (d == '/')

#define lex_word_rule(c, d) \
    ((c >= 'a' && c <= 'z') || (c == '_' && d != ' '))

#define lex_type_rule(c) \
    (c >= 'A' && c <= 'Z' && c != '_')

#define lex_tag_rule(c) \
    (c == '#')

/// lines ending `;` are discards: 
///     they do not define a variable
#define lex_discard_rule(c) \
    (c == ';')

#define lex_rule_ref(c) \
    (c == '.')

#define lex_ignore_rule(c, d, e) \
    ((c == '-') && (d == '-') && (e == '-'))

#define lex_annotation_rule(c)\
    (c == ':')

#define lex_number_rule(c, d) \
    (c >= '0' && c <= '9') || (c == '.' && (d >= '0' && d <= '9'))

#define lex_bra_open_rule(c) \
    (c == '{' || c == '(' || c == '[' )

#define lex_bra_close_rule(c) \
    ( c == '}' || c == ')' || c == ']')

#define lex_string_rule(c) \
    (c == '"' || c == '\'')

#define lex_assign_rule(c, d) \
    (c == ':' && d == '=') || c == '=' ||\
    (c == '<' && d == '-') || (c == '-' && d == '>')

#define lex_op_rule(c) \
    (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || \
    c == '<' || c == '>' || c == '!' || c == '#' || c == '=')


/// @todo -- split `:` away from sep... does it ever emit anyway?
#define lex_sep_rule(c) \
    (c == ',' || c == ':')



#pragma endregion
//#endregion

//#region Parsing_LexerImpl
#pragma region ParsingLexerImpl


void lex_init(ParseContext *pctx, size_t capacity, Str indent) {
    require_not_null(pctx);
    require_positive(capacity);
    require_positive(indent.size);

    pctx->lexer.indent = indent;

    if(pctx->lexer.elements) {
        log_message(LL_WARNING, sMSG("Lexer already initialized, skipping."));
        return;
    }

    array_init(anew, pctx->lexer, capacity);
}


/// @todo think about turning off Str_copy() 
///          by assumption that the source is in the same arena
///          and that we keep it around
///          ... and then consider parser<->lexer sharing

/// @todo this assumes `indent` is spaces, and only really uses its size
///         and so needs to be revised to handle tabs 
/// @todo check for overflow & resize

void lex_source(ParseContext *pctx, Str source, Str indent) {
    require_not_null(pctx);
    ctx_change_arena(mem().arena_static);
    lex_init(pctx, 1024, indent);
    pctx->source.lines = Str_lines(source, &pctx->source.size);
    lex(&pctx->lexer, &pctx->source);
}

void lex_source_file(ParseContext *pctx, Str filename) {
    ///@todo
}


Token* lex_record_token(Lexer *lex, TokenType type, size_t line, size_t col, Str value) {
    require_not_null(lex);
    require_not_null(lex->elements);
    require_positive(capacity_ref(lex) - len_ref(lex));

    Token *tkn = &lex->elements[incr_len_ref(lex)];
    tkn->type = type;
    tkn->line = line;
    tkn->col = col;
    tkn->value = value;
    return tkn;
}



/// @brief 
//  If we keep the lexer arena around, and it holds the source
///    then we dont need to copy into the arena.
/// This is, by default, assumed:
///     would only change if mem perf of holding source seems OTT.
#ifdef LEX_IS_DYNAMIC
    #define emit(tt_type) \
        lex_record_token(lex, tt_type, at_line, start_col, Str_ncopy(line, start_col, at_col))

    #define emit_copy(tt_type, frm_col, value) \
        lex_record_token(lex, tt_type, at_line, at_col, Str_copy(value))
#else
    #define emit(tt_type) \
        lex_record_token(lex, tt_type, at_line, start_col, Str_part(line, start_col, at_col))

    #define emit_copy(tt_type, frm_col, value) \
        lex_record_token(lex, tt_type, at_line, at_col, Str_copy(value))
#endif 


/// @brief the lexer considers up to 3 chars per token to classify
#define fst() Str_chr_at(line, at_col + 0)
#define snd() Str_chr_at(line, at_col + 1)
#define thd() Str_chr_at(line, at_col + 2)

void lex(Lexer *lex, Source *src) {
    require_not_null(lex->elements);
    require_positive(capacity_ref(lex));
    require_not_null(src->lines);
    require_positive(src->size);
    require_not_null(lex->indent.cstr);
    require_positive(lex->indent.size);

    

    bool    supress_nl = false;
    size_t  at_line = 0;
    size_t  at_col = 0;
    size_t  num_indents = 1;
    uint8_t indent_stack[64] = {0};
    #define lAst_indent_level() indent_stack[num_indents - 1]

    for(at_line = 0; at_line < src->size; at_line++, at_col = 0) {
        Str line = src->lines[at_line];

        /// @note and emits indent/dedent elements per `lex->indent.size`
        while(fst() == ' ' || fst() == '\t') at_col++;
        size_t indents = at_col/lex->indent.size;

        while (indents < lAst_indent_level()) {
            num_indents--;
            emit_copy(TT_DEDENT, at_col, lex->indent);
        }

        while (indents > lAst_indent_level()) {
            if (num_indents >= 64) {
                log_message(LL_ERROR, sMSG("Max indent depth of 64, found: %d"), indents);
            }
            indent_stack[num_indents++] = indents;
            emit_copy(TT_INDENT, at_col, lex->indent);
        }

        /// @note skip empty lines
        if(line.size <= at_col) continue;

        #define eol_check() (at_col < line.size)

        /// @note process one character at a time until EOL
        while(at_col < line.size) {
            size_t start_col = at_col;

            if(lex_comment_rule(fst(), snd())) {
                /// @todo process comments by emitting a comment token
                /// ... this will require support in the parser...
                /// ... ie., we'll need to allow doc comments in specific places

                at_col = line.size - 1;
                supress_nl = true;
                break;
                // emit_copy(TT_DOC_COMMENT, 0, s("//")); break;
            } 
            else if(lex_tag_rule(fst())) {
                at_col += 1;
                while(lex_if_wordlike(fst()) && eol_check()) at_col += 1;
                emit(TT_TAG);
            } 
            else if(lex_type_rule(fst())) {
                while(lex_if_wordlike(fst())&& eol_check()) at_col += 1;
                emit(TT_TYPE);
            } 
            else if (lex_word_rule(fst(), snd())) {
                while(lex_if_wordlike(fst())&& eol_check()) at_col += 1;
                emit( 
                    lex_is_keyword(Str_part(line, start_col, at_col)) 
                    ? TT_KEYWORD 
                    : TT_IDENTIFIER
                );
            } 
            else if(lex_number_rule(fst(), snd())) {
                /// @todo parse more variations
                ///  ie., TT_FLOAT, TT_INT, TT_FLOAT_EXP, TT_UINT, TT_DOUBLE
                ///         0.123,   123,     1.2E3,      1234u, 0.123d

                while(lex_if_numberlike(fst())&& eol_check()) at_col += 1;

                if(fst() == '.') { 
                    at_col += 1;
                    while(lex_if_numberlike(fst())&& eol_check()) at_col += 1;
                    emit(TT_DOUBLE);
                } else {
                    emit(TT_INT);
                }

            } 
            else if(lex_bra_open_rule(fst())) {
                at_col += 1;
                emit(TT_BRA_OPEN);
            } 
            else if(lex_bra_close_rule(fst())) {
                at_col += 1;
                emit(TT_BRA_CLOSE);
            } 
            else if(lex_discard_rule(fst())) {
                at_col += 1;
                emit(TT_DISCARD);
            }
            else if(lex_string_rule(fst())) {
                /// @brief 
                /// @todo This is only performs simple single-quote matching
                ///        needs to be expanded to handle triple quotes
                ///         and other string literal syntax 
                /// @todo   handle lack of string termination

                const char quote = Str_chr_at(line, start_col);
                
                at_col += 1;
                while(Str_chr_at(line, at_col++) != quote ) {
                    log_assert(at_col < line.size, sMSG("Unterminated string"));
                }

                emit(TT_STRING);
            } 
            else if(lex_assign_rule(fst(), snd())) {
                if(fst() == ':' && snd() == '=') { at_col += 1; supress_nl = true; } 
                else if(fst() == '-' && snd() == '>') { at_col += 1; }
                else if(fst() == '<' && snd() == '-') { at_col += 1; }

                at_col += 1;
                emit(TT_ASSIGN);
            } 
            else if(lex_ignore_rule(fst(), snd(), thd())) {
                at_col += 3;
                emit(TT_IGNORE);
            } 
            else if(lex_op_rule(fst())) {
                while(lex_op_rule(fst())&& eol_check()) at_col += 1;
                emit(TT_OP);
            } 
            else if(lex_rule_ref(fst())) {
                at_col +=1;
                emit(TT_DEREF);
            }
            else if (lex_sep_rule(fst())) {
                at_col += 1;
                emit(TT_SEP);
            } else if(fst() == '\n') {
                emit(TT_END);
            } else {
                at_col++;
            }
        }

        if(!supress_nl) emit_copy(TT_END, 0, s("\n"));
        supress_nl = false;
    }

    // After processing all lines, 
    // emit DEDENT elements until indent_stack is cleared
    while (num_indents > 1) {
        num_indents--;
        emit_copy(TT_DEDENT, 0, lex->indent);
    }

    /// @note emit final newline token
    /// @todo consider removing this / parsing implications
    emit_copy(TT_END, 0, s("\n"));
}

bool lex_is_keyword(Str str) {
    static const char *kws[] = {
        "const", "fn", "for", "yield", "loop",
        "in", "if", "else", "done", "let",
        "mut*", "mut", "pub", "priv", "mod",
        "try", "return", "match", "mod",
        "where", "use", "as", "dyn",
        "trait", "struct", "macro"
    };

    size_t num = sizeof(kws)/sizeof(kws[0]);
    for (size_t i = 0; i < num; i++) {
        if (Str_eq_cstr(str, kws[i])) return true;
    }
    return false;
}


void lex_print_elements(Token *elements, size_t start, size_t end, size_t marker) {
    printf("\n Tokens (%ld printed):", end - start);
    for (size_t i = start; i < end; i++) {
        printf("\n   %3ld: ", i + 1);
        printf("%.*s", fmt(tt_nameof(elements[i].type)));
        if (elements[i].type == TT_END) {
            printf(" `\\n`");
        } else if (!Str_is_empty(elements[i].value)) {
            printf(" `%.*s`", fmt(elements[i].value));
        }
        printf("@ %zu:%zu", elements[i].line + 1, elements[i].col);
        if (i == marker) printf("\t\t<--- HERE");
    }

    puts("\n");
}



/// @brief Helper function to map TokenType to ANSI color codes
const char* tt_colof(TokenType tt) {
    switch(tt) {
        case TT_KEYWORD:
            return ANSI_COL_BLUE;
        case TT_IDENTIFIER:
            return ANSI_COL_RESET;
        case TT_STRING:
            return ANSI_COL_BRIGHT_YELLOW;
        case TT_INT:
        case TT_FLOAT:
        case TT_DOUBLE:
            return ANSI_COL_BRIGHT_GREEN;
        case TT_TYPE:
            return ANSI_COL_CYAN;
        case TT_OP:
        case TT_ASSIGN:
            return ANSI_COL_RED;
        case TT_TAG:
            return ANSI_COL_BRIGHT_MAGENTA;
        case TT_BRA_OPEN:
        case TT_BRA_CLOSE:
            return ANSI_COL_BRIGHT_BLUE;
        case TT_DOC_COMMENT:
            return ANSI_COL_BRIGHT_BLACK;
        case TT_END:
            // Reset color for end elements (like newlines)
            return ANSI_COL_RESET;
        default:
            // Default color for other elements
            return ANSI_COL_WHITE;
    }
}
Str elements_to_ansi_anew(Lexer lexed) {
    Str result = Str_empty();
    Str reset_str = Str_from_cstr(ANSI_COL_RESET);
    size_t indent_level = 0;
    bool after_nl = true;
        
    for (size_t i = 0; i < len(lexed); i++) {
        Token tok = lexed.elements[i];
        if(!after_nl && (tok.type != TT_SEP  && tok.type != TT_BRA_OPEN))
            result = Str_glue_anew(result, Str_from_cstr(" "));

        switch (tok.type) {
            case TT_INDENT:
                indent_level++;
                break;

            case TT_DEDENT:
                if (indent_level > 0) {
                    indent_level--;
                }
                if(indent_level == 0) {
                    result = Str_glue_anew(result, Str_from_cstr("\n"));
                }
                break;

            case TT_END:
                // Append newline and reset ANSI colors
                result = Str_glue_anew(result, Str_from_cstr("\n"));
                result = Str_glue_anew(result, reset_str);
                after_nl = true;
                break;

            case TT_ASSIGN:
                if(Str_chr_at(tok.value, 0) == ':') {
                    result = Str_glue_anew(result, Str_from_cstr(":=\n"));
                    after_nl = true;
                } else {
                    result = Str_glue_anew(result, tok.value);
                }
                break;
            case TT_IGNORE:
            case TT_DISCARD:
                // Append these elements directly without coloring
                if (!Str_is_empty(tok.value)) {
                    result = Str_glue_anew(result, tok.value);
                }
                break;

            default:
                if (after_nl) {
                    Str indent = Str_repeat_anew(lexed.indent, indent_level);
                    result = Str_glue_anew(result, indent);
                    after_nl = false;
                }

                if (!Str_is_empty(tok.value)) {
                    Str colored = Str_col_anew(tt_colof(tok.type), tok.value);
                    result = Str_glue_anew(result, colored);
                }
                break;
        }
    }

    return Str_glue_anew(result, reset_str);
}



void lex_test_main() {
    Str source = s(
        "const test = 1234\n\n"
        "fn main() :=\n"
        "    log(test)\n"
        "\n"
    );

    Str_println(sMSG("Lexer Tests:"));
    lex_source(pctx(), source, s("    "));
    lex_print_all(pctx()->lexer);
}

#pragma endregion
//#endregion

//#region Parsing_AstApiImpl
#pragma region ParsingAstApiImpl


// Assume Ast_node_anew and Ast are defined elsewhere
Ast *Ast_node_anew(AstType t, size_t line, size_t col) {
    Ast *node = anew(sizeof(Ast));
    if (!node) { error_oom(); return NULL; }
    node->type = t;
    node->line = line;
    node->col = col;
    return node;
}

// static inlined constructors


/// @todo revise to construct from args
/// @brief Creates a new AST node for a type annotation.
/// @param token The token associated with the type annotation (for location tracking).
/// @return A pointer to the newly created Ast.
Ast *Ast_type_annotation_anew(Token *token) {
    Ast *node = anew(sizeof(Ast));
    node->type = AST_TYPE_ANNOTATION;
    node->line = token->line;
    node->col = token->col;
    node->type_annotation.qualifier = Str_empty();
    node->type_annotation.base_type = Str_empty();
    node->type_annotation.type_params = NULL;
    node->type_annotation.num_type_params = 0;
    node->type_annotation.size_constraint = Str_empty();
    return node;
}

/// @brief Creates a new AST node for a destructuring assignment.
Ast *Ast_destructure_assign_anew(Token *token, Ast **variables, size_t num_variables, Ast *expression) {
    Ast *node = anew(sizeof(Ast));
    node->type = AST_DESTRUCTURE_ASSIGN;
    node->line = token->line;
    node->col = token->col;
    node->destructure_assign.variables = variables;
    node->destructure_assign.num_variables = num_variables;
    node->destructure_assign.expression = expression;
    return node;
}


/// @brief Creates a new AST node for a generic type.
Ast *Ast_generic_type_anew(Token *token, Str base_type) {
    return NULL;
    // Ast *node = anew(sizeof(Ast));
    // node->type = AST_GENERIC_TYPE;
    // node->line = token->line;
    // node->col = token->col;
    // node->generic_type.base_type = base_type;
    // node->generic_type.type_params = anew(ARRAY_SIZE_SMALL * sizeof(Ast *));
    // node->generic_type.num_type_params = 0;
    // node->generic_type.constraints = anew(ARRAY_SIZE_SMALL * sizeof(struct TypeConstraint));
    // node->generic_type.num_constraints = 0;
    // node->generic_type.type_params_capacity = ARRAY_SIZE_SMALL;
    // return node;
}

// Identifier
static inline Ast *Ast_identifier_anew(Token *head, Str name) {
    Ast *node = Ast_node_anew(AST_ID, head->line, head->col);
    node->id.name = name;
    return node;
}

// Double Value
static inline Ast *Ast_double_anew(Token *head, double double_val) {
    Ast *node = Ast_node_anew(AST_DOUBLE, head->line, head->col);
    node->dble.value = double_val;
    return node;
}


static inline Ast *Ast_int_anew(Token *head, int integer) {
    Ast *node = Ast_node_anew(AST_INT, head->line, head->col);
    node->integer.value = integer;
    return node;
}

static inline Ast *Ast_discard_anew(Token *head) {
    return Ast_node_anew(AST_DISCARD, head->line, head->col);
}

//// @todo: uint, etc.?

// String Value
static inline Ast *Ast_string_anew(Token *head, Str value) {
    Ast *node = Ast_node_anew(AST_STR, head->line, head->col);
    node->str.value = value;
    return node;
}

// Tag
static inline Ast *Ast_tag_anew(Token *head, Str name) {
    Ast *node = Ast_node_anew(AST_TAG, head->line, head->col);
    node->tag.name = name;
    return node;
}

// Binary Operation
static inline Ast *Ast_bop_anew(Token *head, Str op, Ast *left, Ast *right) {
    Ast *node = Ast_node_anew(AST_BOP, head->line, head->col);
    node->bop.op = op;
    node->bop.left = left;
    node->bop.right = right;
    return node;
}

// Unary Operation
static inline Ast *Ast_uop_anew(Token *head, Str op, Ast *operand) {
    Ast *node = Ast_node_anew(AST_UOP, head->line, head->col);
    node->uop.op = op;
    node->uop.operand = operand;
    return node;
}

// Function Call
static inline Ast *Ast_fn_call_anew(Token *head, Ast *callee, Ast **args, size_t num_args) {
    Ast *node = Ast_node_anew(AST_FN_DEF_CALL, head->line, head->col);
    node->call.callee = callee;
    node->call.args = args;
    node->call.num_args = num_args;
    return node;
}

// Method Call
static inline Ast *Ast_method_call_anew(Token *head,
     Ast *target, Str method, Ast *call) {
    Ast *node = Ast_node_anew(AST_METHOD_CALL, head->line, head->col);
    node->method_call.target = target;
    node->method_call.method = method;
    node->method_call.args = call->call.args;
    node->method_call.num_args = call->call.num_args;
    return node;
}

// Member Access
static inline Ast *Ast_member_access_anew(Token *head,
     Ast *target, Str property) {
    Ast *node = Ast_node_anew(AST_MEMBER_ACCESS, head->line, head->col);
    node->member_access.target = target;
    node->member_access.property = property;
    return node;
}

// Block
static inline Ast *Ast_block_anew(Token *head,
     Ast **statements, size_t num_statements, bool transparent) {
    Ast *node = Ast_node_anew(AST_BLOCK, head->line, head->col);
    node->block.statements = statements;
    node->block.num_statements = num_statements;
    node->block.transparent = transparent;
    return node;
}

// Function 
static inline Ast *Ast_fn_def_anew(Token *head,
     Str name, struct FnParam *params, size_t num_params, Ast *body, 
     bool is_macro, bool is_generator) {
    Ast *node = Ast_node_anew(AST_FN_DEF, head->line, head->col);
    node->fn.name = name;
    node->fn.params = params;
    node->fn.num_params = num_params;
    node->fn.body = body;
    node->fn.is_macro = is_macro;
    node->fn.is_generator = is_generator;
    return node;
}

/// @todo unused -- handled by Ast_fn_def_anew
// Anonymous Function 
// static inline Ast *Ast_fn_anon_def_anew(Token *head,
//      struct AnonFnParam *params, size_t num_params, Ast *body) {
//     Ast *node = Ast_node_anew(AST_FN_DEF_ANON, head->line, head->col);
//     node->fn_anon.params = params;
//     node->fn_anon.num_params = num_params;
//     node->fn_anon.body = body;
//     return node;
// }

// Struct Literal
static inline Ast *Ast_object_literal_anew(Token *head,
     Str struct_name, struct AstObjectField *fields, size_t num_fields) {
    Ast *node = Ast_node_anew(AST_OBJECT_LITERAL, head->line, head->col);
    node->object_literal.struct_name = struct_name;
    node->object_literal.fields = fields;
    node->object_literal.num_fields = num_fields;
    return node;
}

// If 
static inline Ast *Ast_if_anew(Token *head,
     Ast *condition, Ast *body, Ast *else_body) {
    Ast *node = Ast_node_anew(AST_IF, head->line, head->col);
    node->if_stmt.condition = condition;
    node->if_stmt.body = body;
    node->if_stmt.else_body = else_body;
    return node;
}

/// @brief Creates a new AST node for a documentation comment.
/// @todo 
Ast *Ast_doc_comment_anew(Token *token, Str comment) {
    return NULL;
    // Ast *node = anew(sizeof(Ast));
    // node->type = AST_DOC_COMMENT;
    // node->line = token->line;
    // node->col = token->col;
    // node->doc_comment.comment = comment;
    // return node;
}


// Loop
/// @todo: consider whether this should be a different AST structure
static inline Ast *Ast_loop_if_anew(Token *head,
    Ast *condition, Ast *body) {
    Ast *node = Ast_node_anew(AST_LOOP, head->line, head->col);
    node->loop.bindings = NULL;
    node->loop.num_bindings = 0;
    node->loop.body = body;

    /// @todo: given condition != yields.. are both needed?
    ///...... is this assertion correct?
    node->loop.condition = condition;
    node->loop.yields = false;
    return node;
}


static inline Ast *Ast_loop_anew(Token *head, 
    Ast **binds, size_t num_bindings, Ast *body) {
    Ast *node = Ast_node_anew(AST_LOOP, head->line, head->col);
    node->loop.bindings = binds;
    node->loop.num_bindings = num_bindings;
    node->loop.body = body;
    node->loop.condition = NULL;
    node->loop.yields = true;
    return node;
}

// Let 
static inline Ast *Ast_let_anew(Token *head, Ast **bindings, size_t num_bindings, Ast *body) {
    Ast *node = Ast_node_anew(AST_LEF_DEF, head->line, head->col);
    node->let_stmt.bindings = bindings;
    node->let_stmt.num_bindings = num_bindings;
    node->let_stmt.body = body;
    return node;
}

// Binding
static inline Ast *Ast_binding_anew(Token *head,
     Str name, Ast *expression, Str assign_op) {
    Ast *node = Ast_node_anew(AST_BINDING, head->line, head->col);
    node->binding.identifier = name;
    node->binding.expression = expression;
    node->binding.assign_op = assign_op;
    return node;
}

// Return 
static inline Ast *Ast_return_anew(Token *head,
     Ast *value) {
    Ast *node = Ast_node_anew(AST_RETURN, head->line, head->col);
    node->return_stmt.value = value;
    return node;
}

// Match 
static inline Ast *Ast_match_anew(Token *head,
     Ast *expr, struct MatchCase *cases, size_t num_cases) {
    Ast *node = Ast_node_anew(AST_MATCH, head->line, head->col);
    node->match.expression = expr;
    node->match.cases = cases;
    node->match.num_cases = num_cases;
    return node;
}

// Dictionary
static inline Ast *Ast_dict_anew(Token *head,
     struct AstDictEntry *elements, size_t size) {
    Ast *node = Ast_node_anew(AST_DICT, head->line, head->col);
    node->dict.elements = elements;
    node->dict.size = size;
    return node;
}

// Vector
static inline Ast *Ast_vec_anew(Token *head,
     Ast **elements, size_t size) {
    Ast *node = Ast_node_anew(AST_VEC, head->line, head->col);
    node->vec.elements = elements;
    node->vec.size = size;
    return node;
}

// Use 
static inline Ast *Ast_use_anew(Token *head,
     Str module_path, Str alias, bool wildcard) {
    Ast *node = Ast_node_anew(AST_USE, head->line, head->col);
    node->use_stmt.module_path = module_path;
    node->use_stmt.alias = alias;
    node->use_stmt.wildcard = wildcard;
    return node;
}

// Mutation
static inline Ast *Ast_mutation_anew(Token *head,
     Ast *target, Str op, Ast *value, bool is_broadcast) {
    Ast *node = Ast_node_anew(AST_MUTATION, head->line, head->col);
    node->mutation.target = target;
    node->mutation.op = op;
    node->mutation.value = value;
    node->mutation.is_broadcast = is_broadcast;
    return node;
}

// Constant 
static inline Ast *Ast_const_anew(Token *head,
     Str name, Ast *value) {
    Ast *node = Ast_node_anew(AST_CONST_DEF, head->line, head->col);
    node->const_stmt.name = name;
    node->const_stmt.value = value;
    return node;
}

// Yield 
static inline Ast *Ast_yield_anew(Token *head,
     Ast *value) {
    Ast *node = Ast_node_anew(AST_YIELD, head->line, head->col);
    node->yield_stmt.value = value;
    return node;
}

// Ignore 
static inline Ast *Ast_ignore_anew(Token *head) {
    Ast *node = Ast_node_anew(AST_IGNORE, head->line, head->col);
    // No additional fields to set
    return node;
}


/// @todo -- unused?
// Expression 
// static inline Ast *Ast_expression_anew(Token *head,
//      Ast *expression) {
//     Ast *node = Ast_node_anew(AST_EXPRESSION, head->line, head->col);
//     node->exp_stmt.expression = expression;
//     return node;
// }


/// @todo -- to implement
// Module 
// static inline Ast *Ast_module_anew(Token *head, Str name) {
// }




/// @todo
static inline Ast *Ast_struct_anew(Token *head, Str name, struct StructField *fields, size_t num_fields) {
    Ast *node = anew(sizeof(Ast));
    node->type = AST_STRUCT_DEF;
    node->line = head->line;
    node->col = head->col;
    node->struct_def.name = name;
    node->struct_def.fields = fields;
    node->struct_def.num_fields = num_fields;
    return node;
}

static inline Ast *Ast_trait_anew(Token *head, Str name, struct TraitMethod *methods, size_t num_methods) {
    Ast *node = anew(sizeof(Ast));
    node->type = AST_TRAIT;
    node->line = head->line;
    node->col = head->col;
    node->trait.name = name;
    node->trait.methods = methods;
    node->trait.num_methods = num_methods;
    return node;
}



/// @todo -- unused
// static inline struct MatchCase *Ast_match_arm_anew(Token *head, Ast *pattern, Ast *expression) {
//     return NULL;
// }


#pragma endregion
//#endregion 

//#region Parsing_AstStrImpl
#pragma region ParsingAstStrImpl

Str Ast_to_json(Ast *node, size_t indent);
Str Ast_node_to_json(Ast *node, size_t indent);

Str Ast_to_str(Ast *node) {
    if (node == NULL) {
        return Str_empty();
    }
    return Ast_to_json(node, 0);
}

void Ast_print(Ast *node) {
    Str_puts(Ast_to_str(node));
}

Str Ast_to_json(Ast *node, size_t indent) {
    if (node == NULL) {
        return Str_repeat_anew(s(" "), indent);
    }
    return Ast_node_to_json(node, indent);
}

Str Ast_node_to_json(Ast *node, size_t indent) {
    if(node == NULL) return s("null");

    switch (node->type) {
        case AST_BLOCK:
            return Ast_block_to_json(node, indent);
        case AST_IF:
            return Ast_if_to_json(node, indent);
        case AST_FN_DEF:
            return Ast_fn_def_to_json(node, indent);
        case AST_FN_DEF_CALL:
            return Ast_fn_call_to_json(node, indent);
        case AST_BOP:
            return Ast_binary_op_to_json(node, indent);
        case AST_UOP:
            return Ast_unary_op_to_json(node, indent);
        case AST_FN_DEF_ANON:
            return Ast_fn_anon_to_json(node, indent);
        case AST_METHOD_CALL:
            return Ast_method_call_to_json(node, indent);
        case AST_MEMBER_ACCESS:
            return Ast_member_access_to_json(node, indent);
        case AST_CONST_DEF:
            return Ast_const_to_json(node, indent);
        case AST_LEF_DEF:
            return Ast_let_to_json(node, indent);
        case AST_DICT:
            return Ast_dict_to_json(node, indent);
        case AST_VEC:
            return Ast_vec_to_json(node, indent);
        case AST_USE:
            return Ast_use_to_json(node, indent);
        case AST_INT:
        case AST_FLOAT:
        case AST_DOUBLE:
        case AST_STR:
            return Ast_literal_to_json(node, indent);
        case AST_ID:
            return Ast_identifier_to_json(node, indent);
        case AST_LOOP:
            return Ast_loop_to_json(node, indent);
        case AST_FOR:
            return Ast_for_to_json(node, indent);
        case AST_BINDING:
            return Ast_binding_to_json(node, indent);
        case AST_RETURN:
            return Ast_return_to_json(node, indent);
        case AST_MATCH:
            return Ast_match_to_json(node, indent);
        case AST_MUTATION:
            return Ast_mutation_to_json(node, indent);
        case AST_YIELD:
            return Ast_yield_to_json(node, indent);
        case AST_IGNORE:
            return Ast_ignore_to_json(node, indent);
        case AST_EXPRESSION:
            return Ast_expression_to_json(node, indent);
        case AST_MODULE:
            return Ast_module_to_json(node, indent);
        case AST_STRUCT_DEF:
            return Ast_struct_def_to_json(node, indent);
        case AST_OBJECT_LITERAL:
            return Ast_object_literal_to_json(node, indent);
        case AST_TRAIT:
            return Ast_trait_to_json(node, indent);
        case AST_TAG:
            return Ast_literal_to_json(node, indent);
        default:
            // Wrap in an object
            return Str_fmt_anew(
                s("%s{\n%s\"unknown_node\": {\"type\": \"%.*s\"}\n%s}"),
                cstr_repeat_anew(" ", indent),
                cstr_repeat_anew(" ", indent + 1),
                fmt(Ast_nameof(node->type)),
                cstr_repeat_anew(" ", indent)
            );
    }
}

Str Ast_dict_to_json(Ast *node, size_t indent) {
    // { "dict": { "key": value, ... } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"dict\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    for (size_t i = 0; i < node->dict.size; i++) {
        // Each key, value pair
        result = Str_glue_anew(
            result,
            Ast_to_json(node->dict.elements[i].key, indent + 2)
        );
        result = Str_glue_anew(result, s(": "));
        result = Str_glue_anew(
            result,
            Ast_to_json(node->dict.elements[i].value, indent + 2)
        );
        if (i < node->dict.size - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent))
    );
    return result;
}

Str Ast_vec_to_json(Ast *node, size_t indent) {
    // { "vec": [ elements... ] }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"vec\": [\n"), cstr_repeat_anew(" ", indent + 1))
    );
    for (size_t i = 0; i < node->vec.size; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->vec.elements[i], indent + 2)
        );
        if (i < node->vec.size - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent))
    );
    return result;
}

Str Ast_use_to_json(Ast *node, size_t indent) {
    // { "use": {"module_path": "...", "alias": "...", "wildcard": true/false } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"use\": {\"module_path\": \"%.*s\", \"alias\": \"%.*s\", \"wildcard\": %s}\n"),
            cstr_repeat_anew(" ", indent + 1),
            fmt(node->use_stmt.module_path),
            fmt(node->use_stmt.alias),
            node->use_stmt.wildcard ? "true" : "false"
        )
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_ignore_to_json(Ast *node, size_t indent) {
    // { "ignore": {} }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"ignore\": {}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_yield_to_json(Ast *node, size_t indent) {
    // { "yield": {} } – if there's no sub-data
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"yield\": {}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_expression_to_json(Ast *node, size_t indent) {
    // { "expression": <child> }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"expression\": "), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Ast_to_json(node->exp_stmt.expression, indent + 2));
    result = Str_glue_anew(result, s("\n"));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent))
    );
    return result;
}

Str Ast_module_to_json(Ast *node, size_t indent) {
    // { "module": {"name": "..."} }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"module\": {\"name\": \"%.*s\"}\n"),
            cstr_repeat_anew(" ", indent + 1),
            fmt(node->mod_stmt.name)
        )
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_struct_def_to_json(Ast *node, size_t indent) {
    // { "struct": { "name": "...", "fields": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"struct\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"name\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->struct_def.name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"fields\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->struct_def.num_fields; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->struct_def.fields[i].type, indent + 3)
        );
        if (i < node->struct_def.num_fields - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s]\n%s}\n"),
            cstr_repeat_anew(" ", indent + 2),
            cstr_repeat_anew(" ", indent + 1)
        )
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_const_to_json(Ast *node, size_t indent) {
    // { "const": { "name": "...", "value": <child> } }
    Str value = Ast_to_json(node->const_stmt.value, indent + 2);
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"const\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"name\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->const_stmt.name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"value\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, value);
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_trait_to_json(Ast *node, size_t indent) {
    // { "trait": {"name": "...", "methods": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"trait\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"name\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->trait.name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"methods\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->trait.num_methods; i++) {
        // Example placeholder
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s\"TODO\""), cstr_repeat_anew(" ", indent + 3))
        );
        if (i < node->trait.num_methods - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_object_literal_to_json(Ast *node, size_t indent) {
    // { "object_literal": { "struct_name": "...", "fields": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"object_literal\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"struct_name\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->object_literal.struct_name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"fields\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->object_literal.num_fields; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->object_literal.fields[i].value, indent + 3)
        );
        if (i < node->object_literal.num_fields - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_block_to_json(Ast *node, size_t indent) {
    // { "block": { "statements": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"block\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"statements\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->block.num_statements; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->block.statements[i], indent + 3)
        );
        if (i < node->block.num_statements - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_if_to_json(Ast *node, size_t indent) {
    // { "if": { "condition": <child>, "then": <child>, "else": <child>? } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"if\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"condition\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->if_stmt.condition, indent + 3));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"then\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->if_stmt.body, indent + 3));

    if (node->if_stmt.else_body) {
        result = Str_glue_anew(result, s(",\n"));
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s\"else\": "), cstr_repeat_anew(" ", indent + 2))
        );
        result = Str_glue_anew(result, Ast_to_json(node->if_stmt.else_body, indent + 3));
    }

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_fn_def_to_json(Ast *node, size_t indent) {
    // { "function_definition": { "name": "...", "parameters": [...], "body": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"function_definition\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );

    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"name\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->fn.name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"parameters\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->fn.num_params; i++) {
        result = Str_glue_anew(
            result,
            Str_fmt_anew(
                s("%s\"%.*s\""),
                cstr_repeat_anew(" ", indent + 3),
                fmt(node->fn.params[i].name)
            )
        );
        if (i < node->fn.num_params - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s],\n"), cstr_repeat_anew(" ", indent + 2))
    );

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"body\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->fn.body, indent + 2));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_fn_call_to_json(Ast *node, size_t indent) {
    // { "function_call": { "callee": <child>, "arguments": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"function_call\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"callee\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->call.callee, indent + 3));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"arguments\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->call.num_args; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->call.args[i], indent + 3)
        );
        if (i < node->call.num_args - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_binary_op_to_json(Ast *node, size_t indent) {
    // { "binary_operation": { "operator": "+", "left": <child>, "right": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"binary_operation\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"operator\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->bop.op)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"left\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->bop.left, indent + 3));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"right\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->bop.right, indent + 3));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_unary_op_to_json(Ast *node, size_t indent) {
    // { "unary_operation": { "operator": "-", "operand": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"unary_operation\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"operator\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->uop.op)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"operand\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->uop.operand, indent + 3));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_literal_to_json(Ast *node, size_t indent) {
    // Wrap each literal in { "integer": 42 }, { "float": 3.14 }, etc.
    switch (node->type) {
        case AST_INT:
            return Str_fmt_anew(
                s("%s{\"integer\": %lld}"),
                cstr_repeat_anew(" ", indent),
                node->integer.value
            );
        case AST_FLOAT:
        case AST_DOUBLE:
            return Str_fmt_anew(
                s("%s{\"float\": %f}"),
                cstr_repeat_anew(" ", indent),
                node->dble.value
            );
        case AST_STR:
            return Str_fmt_anew(
                s("%s{\"string\": \"%.*s\"}"),
                cstr_repeat_anew(" ", indent),
                fmt(node->str.value)
            );
        case AST_TAG:
            return Str_fmt_anew(
                s("%s{\"tag\": \"%.*s\"}"),
                cstr_repeat_anew(" ", indent),
                fmt(node->tag.name)
            );
        default:
            return Str_fmt_anew(
                s("%s{\"unknown_literal\": {\"type\": \"%s\"}}"),
                cstr_repeat_anew(" ", indent),
                Ast_nameof(node->type)
            );
    }
}

Str Ast_identifier_to_json(Ast *node, size_t indent) {
    // { "identifier": "someName" }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"identifier\": \"%.*s\"\n"),
            cstr_repeat_anew(" ", indent + 1),
            fmt(node->id.name)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent))
    );
    return result;
}

Str Ast_fn_anon_to_json(Ast *node, size_t indent) {
    // { "anonymous_function": { "parameters": [...], "body": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"anonymous_function\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"parameters\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->fn_anon.num_params; i++) {
        result = Str_glue_anew(
            result,
            Str_fmt_anew(
                s("%s\"%.*s\""),
                cstr_repeat_anew(" ", indent + 3),
                fmt(node->fn_anon.params[i].name)
            )
        );
        if (i < node->fn_anon.num_params - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s],\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"body\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->fn_anon.body, indent + 2));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_method_call_to_json(Ast *node, size_t indent) {
    // { "method_call": { "target": <child>, "method": "...", "arguments": [ ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"method_call\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"target\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->method_call.target, indent + 3));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"method\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->method_call.method)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"arguments\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->method_call.num_args; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->method_call.args[i], indent + 3)
        );
        if (i < node->method_call.num_args - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_member_access_to_json(Ast *node, size_t indent) {
    // { "member_access": { "target": <child>, "property": "..." } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"member_access\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"target\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->member_access.target, indent + 3));
    result = Str_glue_anew(result, s(",\n"));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"property\": \"%.*s\"\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->member_access.property)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_let_to_json(Ast *node, size_t indent) {
    // { "let": { "bindings": [...], "body": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"let\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"bindings\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->let_stmt.num_bindings; i++) {
        result = Str_glue_anew(
            result,
            Ast_to_json(node->let_stmt.bindings[i], indent + 3)
        );
        if (i < node->let_stmt.num_bindings - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s],\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"body\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->let_stmt.body, indent + 2));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_loop_to_json(Ast *node, size_t indent) {
    // { "loop": { "condition": <child>?, "bindings": [...], "body": <child>, "yields": bool } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"loop\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );

    if (node->loop.condition) {
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s\"condition\": "), cstr_repeat_anew(" ", indent + 2))
        );
        result = Str_glue_anew(result, Ast_to_json(node->loop.condition, indent + 3));
        result = Str_glue_anew(result, s(",\n"));
    }

    if (node->loop.num_bindings > 0) {
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s\"bindings\": [\n"), cstr_repeat_anew(" ", indent + 2))
        );
        for (size_t i = 0; i < node->loop.num_bindings; i++) {
            result = Str_glue_anew(
                result,
                Ast_to_json(node->loop.bindings[i], indent + 3)
            );
            if (i < node->loop.num_bindings - 1) {
                result = Str_glue_anew(result, s(",\n"));
            } else {
                result = Str_glue_anew(result, s("\n"));
            }
        }
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s],\n"), cstr_repeat_anew(" ", indent + 2))
        );
    }

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"body\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->loop.body, indent + 2));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"yields\": %s\n"),
            cstr_repeat_anew(" ", indent + 2),
            node->loop.yields ? "true" : "false"
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_for_to_json(Ast *node, size_t indent) {
    // For now, we treat 'for' the same as 'loop'.
    return Ast_loop_to_json(node, indent);
}

Str Ast_binding_to_json(Ast *node, size_t indent) {
    // { "binding": { "identifier": "...", "assign_op": "...", "expression": <child> } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"binding\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"identifier\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->binding.identifier)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"assign_op\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->binding.assign_op)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"expression\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->binding.expression, indent + 3));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("\n%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_return_to_json(Ast *node, size_t indent) {
    // { "return": <child or null> }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"return\": "), cstr_repeat_anew(" ", indent + 1))
    );
    if (node->return_stmt.value) {
        result = Str_glue_anew(result, Ast_to_json(node->return_stmt.value, indent + 2));
    } else {
        result = Str_glue_anew(result, s("null"));
    }
    result = Str_glue_anew(result, s("\n"));
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_match_to_json(Ast *node, size_t indent) {
    // { "match": { "expression": <child>, "cases": [ { "condition": <child>?, "expression": <child> }, ... ] } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"match\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"expression\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->match.expression, indent + 3));
    result = Str_glue_anew(result, s(",\n"));

    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"cases\": [\n"), cstr_repeat_anew(" ", indent + 2))
    );
    for (size_t i = 0; i < node->match.num_cases; i++) {
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent + 3))
        );
        if (node->match.cases[i].condition) {
            result = Str_glue_anew(
                result,
                Str_fmt_anew(s("%s\"condition\": "), cstr_repeat_anew(" ", indent + 4))
            );
            result = Str_glue_anew(
                result,
                Ast_to_json(node->match.cases[i].condition, indent + 5)
            );
            result = Str_glue_anew(result, s(",\n"));
        }
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("%s\"expression\": "), cstr_repeat_anew(" ", indent + 4))
        );
        result = Str_glue_anew(
            result,
            Ast_to_json(node->match.cases[i].expression, indent + 5)
        );
        result = Str_glue_anew(
            result,
            Str_fmt_anew(s("\n%s}"), cstr_repeat_anew(" ", indent + 3))
        );
        if (i < node->match.num_cases - 1) {
            result = Str_glue_anew(result, s(",\n"));
        } else {
            result = Str_glue_anew(result, s("\n"));
        }
    }
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s]\n"), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

Str Ast_mutation_to_json(Ast *node, size_t indent) {
    // { "mutation": { "target": <child>, "operator": "...", "value": <child>, "broadcast": bool } }
    Str result = Str_fmt_anew(s("%s{\n"), cstr_repeat_anew(" ", indent));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"mutation\": {\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"target\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->mutation.target, indent + 3));
    result = Str_glue_anew(result, s(",\n"));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s("%s\"operator\": \"%.*s\",\n"),
            cstr_repeat_anew(" ", indent + 2),
            fmt(node->mutation.op)
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s\"value\": "), cstr_repeat_anew(" ", indent + 2))
    );
    result = Str_glue_anew(result, Ast_to_json(node->mutation.value, indent + 3));
    result = Str_glue_anew(
        result,
        Str_fmt_anew(
            s(",\n%s\"broadcast\": %s\n"),
            cstr_repeat_anew(" ", indent + 2),
            node->mutation.is_broadcast ? "true" : "false"
        )
    );
    result = Str_glue_anew(
        result,
        Str_fmt_anew(s("%s}\n"), cstr_repeat_anew(" ", indent + 1))
    );
    result = Str_glue_anew(result, Str_fmt_anew(s("%s}"), cstr_repeat_anew(" ", indent)));
    return result;
}

#pragma endregion
//#endregion

//#region Parsing_Impl
#pragma region ParsingParserImpl

#define elements_remain() (p->parser.current_token < p->lexer.meta.size) 
#define depth_is_bounded() (p->parser.depth++ <= 256)
#define token_not_null(t) (t.value.size > 0)

#define peek() parse_peek(p, 0)
#define peek_ahead() parse_peek(p, 1)
#define peek_eq(val) (elements_remain() && Str_eq(parse_peek(p, 0)->value, val))
#define peek_eq_chr(cvar) (elements_remain() && Str_eq_chr(parse_peek(p, 0)->value, cvar))
#define peek_is(tkt) (elements_remain() && (parse_peek(p, 0)->type == tkt))
#define consume(type, err)  parse_consume(p, type, Str_empty(), err)
#define consume_specific(type, value, errmsg) parse_consume(p, type, value, errmsg)
#define consume_expected(tkt) parse_consume(p, tkt, Str_empty(), Str_empty())


#define parse_not_implemented()\
    parse_error(p, peek(), sMSG("Parser: this syntax is not yet implemented!"));\
    consume_expected(peek()->type);\
    return NULL

static inline Token *parse_peek(ParseContext *p, size_t offset) {
    if(elements_remain() && token_not_null(p->lexer.elements[p->parser.current_token + offset])) {
        return &p->lexer.elements[p->parser.current_token + offset];
    } else return NULL;
}

static inline Token *parse_consume(ParseContext *p, TokenType type, Str value, Str errmsg) {
    if((value.size && peek_eq(value) && peek_is(type)) || peek_is(type)) {
        return &p->lexer.elements[p->parser.current_token++];
    } else {
        parse_error(p, peek(), errmsg);
        return NULL;
    }
}





void parse(ParseContext *p) {
    size_t count = 0;
    Ast **statements = anew(ARRAY_SIZE_MEDIUM * sizeof(Ast *));
    Ast *result = NULL;

    while(elements_remain() && depth_is_bounded()) {
        pctx_trace(tt_nameof(peek()->type));
        result = parse_statement(p);
        if(result != NULL) {
            statements[count++] = result;
        }
        p->parser.depth = 0;
        require_medium_array(count);
    }

    p->parser.current_token = 0;
    p->parser.depth = 0;
    parser_update_ast(Ast_block_anew(peek(), statements, count, true));
}


void parse_print_Ast_node(Ast *ast, size_t depth);
void parse_Ast_from_source(ParseContext *p, Source *src);
void parse_error(ParseContext *pctx, Token *token, Str message) {
    Str_println(s("Parser Traceback:"));
    Tracer_print_last(&pctx->parser.traces);

    log_message(LL_INFO, s("Current Token %d: %.*s %.*s\n"), 
        pctx->parser.current_token, fmt(tt_nameof(token->type)), fmt(token->value)
    );
    
    log_message(LL_ERROR, 
        Str_col_anew(ANSI_COL_BRIGHT_YELLOW, s("Parse Error: %.*s\n")), fmt(message)
    );
}



// #define trace(moreinfo) /// @todo

// Token *parse_consume_value(ParseContext *p, Str value, TokenType type, Str error_message);

/// @brief 
/// @example
///     log("Hello World"); // discards return value
///     const result = 10 * 2
///
/// @param p 
/// @return
Ast *parse_statement(ParseContext *p) {
    log_assert(depth_is_bounded(), sMSG("Beyond max depth"));
    log_assert(elements_remain(), sMSG("No elements remain!"));
    require_not_null(peek());


    if(peek_is(TT_END)) {
        pctx_trace(tt_nameof(peek()->type));
        consume_expected(TT_END);
        return NULL;
    } 
    
    pctx_trace(peek()->value);
    if(peek_is(TT_KEYWORD)) {
        return parse_keyword_statement(p, false);
    } else if(peek_is(TT_INDENT)) {
        return parse_block(p);
    } else if(peek_is(TT_DISCARD)) {
        consume_expected(TT_DISCARD);
        return Ast_discard_anew(peek());
    } else {
        /// @todo: expr statements should end on a nl
        return parse_expression(p, 0);
    }
}

static inline uint8_t parse_peek_precedence(ParseContext *p) {
    if (!peek()) return 0;

    if (peek_is(TT_ASSIGN)) return 1;
    if (peek_eq_chr('|')) return 1;
    if (peek_eq_chr('&')) return 2;
    if (peek_eq_chr('=') || peek_eq_chr('!')) return 4;
    if (peek_eq_chr('<') || peek_eq_chr('>')) return 5;
    if (peek_eq_chr('+') || peek_eq_chr('-')) return 6;
    if (peek_eq_chr('*') || peek_eq_chr('/') || peek_eq_chr('%')) return 7;
    if (peek_eq_chr('.')) return 8; 
    if (peek_is(TT_SEP)) return 9;
    if (peek_is(TT_DISCARD)) return 10;

    parse_error(p, peek(), sMSG("Operator precedence not found."));
    return 99;
}



/// @brief 
/// @example 
///     g(10 * (2/3) + 300) 
/// @param p 
/// @param precedence 
/// @return
Ast *parse_expression(ParseContext *p, size_t precedence) {
    #define peek_is_expression_end() (peek_eq_chr(',') || peek_eq_chr(':') ||\
    peek_is(TT_BRA_CLOSE) || peek_is(TT_END) || peek_is(TT_ASSIGN) || peek_is(TT_DISCARD))

    log_assert(depth_is_bounded(), sMSG("Beyond max depth"));
    log_assert(elements_remain(), sMSG("No elements remain!"));
    pctx_trace(peek()->value);

    if (peek_is_expression_end()) return NULL;

    Ast *left = parse_expression_part(p);
    while (elements_remain() && !peek_is_expression_end()) {
        size_t current_prec = parse_peek_precedence(p);
        if (current_prec <= precedence) break;

        if (peek_is(TT_DEREF)) {
            consume(TT_DEREF, sMSG("Expected '.' for member access"));
            Token *right = consume(TT_IDENTIFIER, sMSG("Expected member name after '.'"));
            left = Ast_member_access_anew(peek(), left, right->value);

            if (peek_eq_chr('(')) {
                // If '(' follows, it's a method call
                left = parse_post_call(p, left);
            }
        }
        else if (peek_is(TT_OP)) {
            Token *op = consume(TT_OP, sMSG("Expected a binary operator."));
            Ast *right = parse_expression(p, current_prec);
            if(right != NULL) {
                left = Ast_bop_anew(op, op->value, left, right);
            }
        } else {
            parse_error(p, peek(), sMSG("Expected an operator."));
            break;
        }
    }

    return left;
}



/// @brief 
/// @example
///     10 + x  
/// @param p 
/// @return
Ast *parse_expression_part(ParseContext *p) {
    pctx_trace(peek()->value);

    switch(peek()->type) {
        case TT_KEYWORD: 
            return parse_keyword_statement(p, true);
        case TT_TAG: case TT_DOUBLE: case TT_INT: case TT_STRING: 
            return parse_literal(p);
        // case TT_DEREF:
        //     return parse_dereference(p);
        case TT_IDENTIFIER: 
            return parse_identifier(p);
        case TT_TYPE:
            return parse_object_literal(p);
        case TT_DISCARD:
            return NULL;
        case TT_BRA_OPEN:
            return parse_bracketed(p);
        case TT_OP:
            return parse_unary(p);
        default:
            parse_error(p, peek(), sMSG("Unknown token type in an expression."));
            return NULL;
    }
}


/// @brief 
/// @example 
///     const result = for(x <- range(10)) x * 20
///
/// @param p 
/// @param in_expr 
/// @return
Ast *parse_keyword_statement(ParseContext *p, bool in_expr) {
    pctx_trace(peek()->value);

    if(!peek_is(TT_KEYWORD)) { 
        parse_error(p, peek(), sMSG("Expected a keyword.")); 
        return NULL;
    }

    #define perror_no_expr() if(in_expr)\
        parse_error(p, peek(), sMSG("Keyword not allowed in an expression."))
 
    if(peek_eq(s("use"))) {             
        perror_no_expr(); 
        return parse_use(p);
    } else if(peek_eq(s("trait"))) {    
        perror_no_expr(); 
        return parse_trait(p);
    } else if(peek_eq(s("struct"))) {   
        perror_no_expr(); 
        return parse_struct(p);
    } else if(peek_eq(s("const"))) {    
        perror_no_expr(); 
        return parse_const(p);
    } else if(peek_eq(s("fn"))) {
        return parse_function(p, false);
    } else if(peek_eq(s("for"))) {
        return parse_for(p);
    } else if(peek_eq(s("if"))) {
        return parse_if(p);
    } else if(peek_eq(s("let"))) {
        return parse_let(p);
    } else if(peek_eq(s("mut"))) {
        return parse_mutation(p);
    } else if(peek_eq(s("match"))) {
        return parse_match(p);
    } else if(peek_eq(s("yield"))) {
        return parse_yield(p);
    } else if(peek_eq(s("return"))) {
        return parse_return(p);
    } else if(peek_eq(s("loop"))) {
        return parse_loop(p);
    } else if(peek_eq(s("mod"))) {
        parse_not_implemented();
        // return parse_m(p);
    } else if(peek_eq(s("macro"))) {
        return parse_macro_def(p);
    } else {
        parse_error(p, peek(), sMSG("Unknown keyword."));
        return NULL;
    }
}

/// @brief Parses a block of statements, 
//      typically following keywords like `if`, `for`, etc.
/// @example
///     fn main() :=
///         log("Condition met")
///         execute()
/// @param p The parsing context.
/// @return An AST node representing the block.
Ast *parse_block(ParseContext *p) {
    pctx_trace(peek()->value);

    // Expecting an indentation to start the block
    consume_specific(TT_INDENT, s("INDENT"), sMSG("Expected an indentation to start the block."));

    size_t count = 0;
    Ast **statements = anew(ARRAY_SIZE_MEDIUM * sizeof(Ast *));
    Ast *result = NULL;

    while(elements_remain() && !peek_is(TT_DEDENT)) {
        result = parse_statement(p);
        if(result != NULL) {
            statements[count++] = result;
        }
        p->parser.depth = 0;
        require_medium_array(count);
    }

    // Consume the dedent token
    consume_specific(TT_DEDENT, s("DEDENT"), sMSG("Expected a dedent to end the block."));

    return Ast_block_anew(peek(), statements, count, false);
}


/// @brief 
/// @example
///      
/// @param p 
/// @return
Ast *parse_identifier(ParseContext *p) {
    pctx_trace(peek()->value);

    Token *t = consume_expected(TT_IDENTIFIER);
    Ast *id = Ast_identifier_anew(t, t->value);

    if(peek_eq_chr('(')) {
        return parse_post_call(p, id);
    } else {
        return id;
    }
}


/// @brief 
/// @note the `loop if` syntax is equivalent to `while()`
/// @note the `loop ... in` syntax is a loop over streams
/// @example
///     loop if len(vec) > 0
///         log(pop(vec))
///
///     loop
///         x <- xs
///         y = x * 2
///     in match y
///         if 10 -> log("Yes")
///         else  -> log("No")
/// @param p 
/// @return 
Ast *parse_loop(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("loop"), sMSG("Expected `loop`."));

    if(peek_eq(s("fn"))) return parse_function(p, true);

    if(peek_eq(s("if"))) {
        consume_expected(TT_KEYWORD);
        return Ast_loop_if_anew(peek(), parse_expression(p, 0), parse_block(p));
    } else {
        size_t out_num_bindings = 0;
        Ast **binds = parse_bindings(p, &out_num_bindings);


        /// @todo think about collapsing `in` with one-liners
        /// ....    idea: move `in`-checking to parse_block, which can detect bracketing
        consume_specific(TT_KEYWORD, s("in"), sMSG("`in` missing: loop should have `in` scope."));
        consume_expected(TT_END);

        Ast *body = parse_block(p);
        
        return Ast_loop_anew(peek(), binds, out_num_bindings, body);
    }
}
/// @brief  
/// @note the `for` comprehension is a collecting operation across a stream
/// @note it can be a single-line with parentheses, or per in-block
/// @example
///     for(x <- xs, y <- ys, z = x * y) z + x + y
///
///     for 
///        x <- xs
///        y <- ys
///     in 
///         x * y
///
/// @param p 
/// @return 
Ast *parse_for(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("for"), sMSG("Keyword `for` expected."));

    size_t out_num_bindings = 0;
    Ast **binds = parse_bindings(p, &out_num_bindings);
    Ast *body = peek_is(TT_INDENT) ? parse_block(p) : parse_expression(p, 0);
    return Ast_loop_anew(peek(), binds, out_num_bindings, body);
}

/// @brief 
/// @example
///     if something
///         log(something)
///     else 
////        log("nothing")
///
///     if something == 1
///         log(something)
///     else if something == 2
///         log("nothing")
///
///     const result = if something then 1 else 2
/// @param p 
/// @return 
/// @todo -- support for else if 
Ast *parse_if(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("if"), sMSG("Keyword `for` expected."));

    Ast *cond = parse_expression(p, 0);
    Ast *body = peek_is(TT_INDENT) ? parse_block(p) : parse_expression(p, 0);
    Ast *elif = NULL;

    if(peek_eq(s("else"))) {
        consume_specific(TT_KEYWORD, s("else"), sMSG("Keyword `else` expected"));
        elif = peek_is(TT_INDENT) ? parse_block(p) : parse_expression(p, 0);
    }

    return Ast_if_anew(peek(), cond, body, elif);
}


/// @brief 
/// @note The main kind of block, to be used in binding variables
/// @note Can be abbreviated to a one-liner with parentheses after `let`
/// @note All terms are immutable by default, but `let mut` changes this
/// @example
///     let
///         x = 10
///         mut y = 5
///     in 
///         log(x)
///
///     let( x = 10 ) in log(x)
///
///     let mut
///         y = 5
///     in 
///         log(take(10, for(ever) mut! y += 1))
///         log(y)
///
/// @param p 
/// @return 
Ast *parse_let(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("let"), sMSG("Keyword `let` expected."));

    size_t out_num_bindings = 0;
    Ast **binds = parse_bindings(p, &out_num_bindings);

    /// @todo -- consider whether this should lie with parse_block()
    consume_specific(TT_KEYWORD, s("in"), sMSG("Keyword `in` expected."));
    consume_expected(TT_END);

    Ast *body = peek_is(TT_INDENT) ? parse_block(p) : parse_expression(p, 0);
    return Ast_let_anew(peek(), binds, out_num_bindings, body);
}


/// @brief 
/// @note Defines global constants, therefore never has an `in` block
/// @example
///     const my_constant = 10
/// @param p 
/// @return 
Ast *parse_const(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("const"), sMSG("Keyword `for` expected."));

    if(peek_eq(s("fn"))) {
        /// @todo placeholder for `const fn` impl
        return parse_function(p, false);
    }

    Token *name = consume(TT_IDENTIFIER, sMSG("Expected an identifier for const name."));
    Token *op = consume(TT_ASSIGN, sMSG("Expected `=` or `:=` to define consts."));
    if(Str_chr_at(op->value, 1) == ':')  {
        consume(TT_INDENT, sMSG("Expected an indent after `:=`"));
    }
    Ast *expr = parse_expression(p, 0);
    if(Str_chr_at(op->value, 1) == ':')  {
        consume(TT_DEDENT, sMSG("Expected a dedent after `:=` block."));
    }

    return Ast_const_anew(peek(), name->value, expr);
}


/// @brief 
/// @note Defines functions, use `:=` for a block def
/// @example
///     fn main() = log("Hello World")
/// 
///     fn main(args) :=
///         log("This is a block: " ++ args[0])
///         10 // last line is return value
/// @param p 
/// @return 
Ast *parse_function(ParseContext *p,bool is_loop) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("fn"), sMSG("Keyword `for` expected."));

    if(peek_eq_chr('(')) {
        return parse_post_anon(p);
    }

    Str name = consume(TT_IDENTIFIER, sMSG("Expected function name"))->value;

    consume_specific(TT_BRA_OPEN, s("("), sMSG("Expected an opening parenthesis."));
    struct FnParam *params = anew(sizeof(struct FnParam *) * ARRAY_SIZE_SMALL);
    size_t count = 0;
    while(elements_remain()) {
        if(peek_eq_chr(')')) break;
        if(peek_eq_chr(',')) consume_expected(TT_SEP);

        pctx_trace(peek()->value);
        params[count++].name = consume(TT_IDENTIFIER, sMSG("Expected parameter name"))->value;
    }
    consume_specific(TT_BRA_CLOSE, s(")"), sMSG("Expected a closing parenthesis."));
    
    Token *op = consume(TT_ASSIGN, sMSG("Expected `=` or `:=`"));

    return Ast_fn_def_anew(
        peek(), name, params, count, 
        Str_eq_chr(op->value, '=') ? parse_expression(p, 0) : parse_block(p), 
        false, is_loop
    );
}


/// @brief Parses a generic type with constraints.
//// @todo not yet implemented 
/// @example
///     fn foo<T: Trait1, Trait2>(param: T)
/// @param p The parsing context.
/// @return An AST node representing the generic type.
Ast *parse_generic_type(ParseContext *p) {
    pctx_trace(peek()->value);
    parse_not_implemented();

    // // Parse base type name
    // Token *base_type_token = consume(TT_TYPE, sMSG("Expected base type name for generic type."));

    // Ast *generic_type = Ast_generic_type_anew(peek(), base_type_token->value);

    // // Check for type parameters enclosed in '<' and '>'
    // if (peek_eq_chr('<')) {
    //     consume_specific(TT_BRA_OPEN, s("<"), sMSG("Expected '<' to start generic type parameters."));

    //     while (elements_remain() && !peek_eq_chr('>')) {
    //         // Parse each type parameter
    //         Ast *type_param = parse_type_annotation(p);
    //         generic_type->generic_type.type_params[generic_type->generic_type.num_type_params++] = type_param;

    //         // Handle comma-separated type parameters
    //         if (peek_eq_chr(',')) {
    //             consume_specific(TT_SEP, s(","), sMSG("Expected ',' between type parameters."));
    //         } else {
    //             break;
    //         }

    //         require_small_array(generic_type->generic_type.type_params);
    //     }

    //     consume_specific(TT_BRA_OPEN, s(">"), sMSG("Expected '>' to end generic type parameters."));
    // }

    // // Check for type constraints after ':'
    // if (peek_eq_chr(':')) {
    //     consume_specific(TT_OP, s(":"), sMSG("Expected ':' to start type constraints."));

    //     while (elements_remain() && !peek_eq_chr(';') && !peek_eq_chr(')')) {
    //         // Parse each constraint
    //         Str constraint_name = consume(TT_TYPE, sMSG("Expected trait name for type constraint.")).->value;
    //         generic_type->generic_type.constraints[generic_type->generic_type.num_constraints++] = Ast_type_constraint_anew(peek(), constraint_name);

    //         // Handle comma-separated constraints
    //         if (peek_eq_chr(',')) {
    //             consume_specific(TT_SEP, s(","), sMSG("Expected ',' between type constraints."));
    //         } else {
    //             break;
    //         }
    //     }
    // }

    // return generic_type;
}


/// @brief Parses a macro def.
/// @example
///     macro fn create_struct($name, $$fields) :=
///             emit `struct $name :=`
///                 emit* `$$fields`
/// @param p The parsing context.
/// @return An AST node representing the macro def.
Ast *parse_macro_def(ParseContext *p) {
    pctx_trace(peek()->value);
    parse_not_implemented();

    // // Consume 'macro_rules!' keyword
    // consume_specific(TT_KEYWORD, s("macro"), sMSG("Expected 'macro' keyword."));
    // consume_specific(TT_KEYWORD, s("fn"), sMSG("Expected 'fn' keyword."));

    // // Parse macro name
    // Token *macro_name = consume(TT_IDENTIFIER, sMSG("Expected macro name."));

    // // Expecting ':=' to start macro def
    // consume_specific(TT_ASSIGN, s(":="), sMSG("Expected ':=' to start macro def."));

    // // Parse macro rules as a block
    // Ast *macro_body = parse_block(p);

    // // Create and return macro def AST node
    // return Ast_macro_def_anew(peek(), macro_name->value, macro_body->macro_def.rules, macro_body->macro_def.num_rules);
}

/// @brief Parses a documentation comment.
/// @example
///     /// This function adds two numbers.
///     fn add(a: Int, b: Int): Int :=
///         a + b
/// @param p The parsing context.
/// @return An AST node representing the documentation comment.
Ast *parse_doc_comment(ParseContext *p) {
    pctx_trace(peek()->value);

    // Consume documentation comment token
    Token *comment_token = consume(TT_DOC_COMMENT, sMSG("Expected documentation comment."));

    // Create and return documentation comment AST node
    return Ast_doc_comment_anew(peek(), comment_token->value);
}


/// @brief Parses anonymous functions or lambda expressions.
/// @example
///     fn(x, y) -> x + y
/// @param p The parsing context.
/// @return An AST node representing the anonymous function.
Ast *parse_post_anon(ParseContext *p) {
    // Parse parameters
    consume_specific(TT_BRA_OPEN, s("("), sMSG("Expected '(' after 'fn' for anonymous function parameters."));
    
    struct FnParam *params = anew(sizeof(struct FnParam) * ARRAY_SIZE_SMALL);
    size_t count = 0;

    while(elements_remain() && !peek_eq_chr(')')) {
        // require_small_array(count);
        if(count >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many parameters for anonymous function."
                    "The maximum number of parameters is 16."
                    "Consider using a struct or collection to group parameters."
                )
            );
            break;
        }

        if(peek_eq_chr(',')) {
            consume_specific(TT_SEP, s(","), sMSG("Expected a comma between parameters."));
        }

        pctx_trace(peek()->value);
        params[count].name = consume(TT_IDENTIFIER, sMSG("Expected parameter name"))->value;
        params[count].type = NULL;
        params[count].default_value = NULL;  /// @todo
        
        count++;
    }

    consume_specific(TT_BRA_CLOSE, s(")"), sMSG("Expected ')' after anonymous function parameters."));
    consume(TT_ASSIGN, sMSG("Expected '->' after anonymous function parameters."));

    Ast *body = peek_is(TT_INDENT) ? parse_block(p) : parse_expression(p, 0);

    return Ast_fn_def_anew(peek(), s(""), params, count, body, false, false);
}


/// @brief Parses a destructuring assignment.
/// @example
///     let [x, y] = get_coordinates() in x
/// @param p The parsing context.
/// @return An AST node representing the destructuring assignment.
Ast *parse_destructure_assignment(ParseContext *p) {
    pctx_trace(peek()->value);

    // Consume 'let' keyword
    consume_specific(TT_KEYWORD, s("let"), sMSG("Expected 'let' keyword for destructuring assignment."));

    /// @todo handle indent 

    // Expecting '(' to start variable list
    consume_specific(TT_BRA_OPEN, s("["), sMSG("Expected '(' to start variable list in destructuring assignment."));

    // Parse variables
    Ast **variables = anew(ARRAY_SIZE_SMALL * sizeof(Ast *));
    size_t num_variables = 0;

    while (elements_remain() && !peek_eq_chr(']')) {
        if (peek_eq_chr(',')) {
            consume_specific(TT_SEP, s(","), sMSG("Expected ',' between variables in destructuring assignment."));
        }

        // Parse variable (identifier)
        Token *var_token = consume(TT_IDENTIFIER, sMSG("Expected variable name in destructuring assignment."));
        Ast *var_node = Ast_identifier_anew(var_token, var_token->value);
        variables[num_variables++] = var_node;
        require_small_array(num_variables);
    }

    // Consume ')' to end variable list
    consume_specific(TT_BRA_CLOSE, s("]"), sMSG("Expected ')' to end variable list in destructuring assignment."));

    // Consume '=' operator
    consume_specific(TT_ASSIGN, s("="), sMSG("Expected '=' in destructuring assignment."));

    // Parse the expression being destructured
    Ast *expr = parse_expression(p, 0);

    // Create and return destructure assignment AST node
    return Ast_destructure_assign_anew(peek(), variables, num_variables, expr);
}


/// @brief Parses an asynchronous function def.
/// @example
///     async fn fetch_data(url: String) := 
///         // asynchronous operations
/// @param p The parsing context.
/// @return An AST node representing the async function def.
Ast *parse_async_function(ParseContext *p) {
    pctx_trace(peek()->value);
    parse_not_implemented();
}

/// @brief 
/// @note parses groups of bindings (assignment: =  :=  streaming: <-)
/// @example
///     (x <- range(2, 100, 2), y <- repeat(1), z = x * y)
///
///         x <- range(2, 100, 2)
///         y <- repeat(1)
///         z = x * y
///
/// @param p 
/// @return 
Ast **parse_bindings(ParseContext *p, size_t *out_num_bindings) {
    pctx_trace(peek()->value);

    bool is_bracketed = peek_eq_chr('(');

    if(is_bracketed) {  
        consume_specific(TT_BRA_OPEN, s("("), sMSG("A bracket expected."));
    } else {
        consume(TT_END, sMSG("Bindings blocks follow keywords on their own line."));
        consume(TT_INDENT, sMSG("Bindings blocks should be indented or bracketed."));
    }

    Ast **bindings = anew(ARRAY_SIZE_SMALL* sizeof(Ast *));
    *out_num_bindings = 0;
    while(elements_remain()) {
        if(peek_eq_chr(')') || peek_is(TT_DEDENT)) break;

        Str id = consume(TT_IDENTIFIER, sMSG("Expected an identifier"))->value;
        Str op = consume(TT_ASSIGN, sMSG("Expected an assignment operator"))->value;
        Ast *expr = parse_expression(p, 0);
        if(is_bracketed && *out_num_bindings >= 1) {
            consume_specific(TT_SEP, s(","), sMSG("Bracketed bindings are comma separated"));
        } else if(!is_bracketed) {
            consume(TT_END, s("Block bindings should each end on a new line"));
        }
        bindings[*out_num_bindings] = Ast_binding_anew(peek(), id, expr, op);
        *out_num_bindings += 1;

        // require_small_array(*out_num_bindings);

        if(*out_num_bindings >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many bindings. The maximum number of bindings is 16."
                    "You can nest binding scopes."
                    "Or, consider using a struct or collection to group bindings."
                )
            );
            break;
        }
    }

    if(is_bracketed) {
        consume_specific(TT_BRA_CLOSE, s(")"), sMSG("Expected a closing bracket."));
    } else {
        consume(TT_DEDENT, sMSG("Expected a dedent."));
    }
    return bindings;
}

Ast *parse_bracketed(ParseContext *p) {
    pctx_trace(peek()->value);

    if(peek_eq_chr('(')) {
        consume_expected(TT_BRA_OPEN);
        Ast *expr = parse_expression(p, 0);
        consume_expected(TT_BRA_CLOSE);
        return expr;
    } else if(peek_eq_chr('[')) {
        return parse_vec(p);
    } else if(peek_eq_chr('{')) {
        return parse_dict(p);
    } else {
        parse_error(p, peek(), sMSG("Unknown bracket type"));
        return NULL;
    }
}

/// @brief Parses a set or dictionary literal.
/// @note dictionaries cannot have sets as keys, so any `:` is dispositive
/// @example
///     {1, 2, 3} -> set literal
///     {1: 2, 3: 4} -> dictionary literal
/// @param p The parse context.
/// @return An AST node representing the parsed set or dictionary.
Ast *parse_set_or_dict(ParseContext *p) {
    pctx_trace(peek()->value);

    // Consume the opening `{`
    consume_specific(TT_BRA_OPEN, s("{"), sMSG("Expected `{` to start set or dictionary."));

    // Prepare containers for keys and values
    Ast **keys = anew(sizeof(Ast *) * ARRAY_SIZE_SMALL);
    Ast **values = anew(sizeof(Ast *) * ARRAY_SIZE_SMALL);
    size_t count_keys = 0, count_values = 0;

    // Parse elements
    bool is_dict = false;
    while (!peek_eq(s("}"))) {
        Ast *key = parse_expression(p, 0);

        if (peek_eq(s(":"))) {
            // Colon detected, it's a dictionary
            is_dict = true;
            consume_specific(TT_SEP, s(":"), sMSG("Expected `:` in dictionary literal."));
            Ast *value = parse_expression(p, 0);
            keys[count_keys++] = key;
            values[count_values++] = value;
        } else if (is_dict) {
            // Error: mixing dictionary and set syntax
            parse_error(p, peek(), sMSG("Mixed set and dictionary syntax is not allowed."));
        } else {
            // No colon, assume it's a set element
            keys[count_keys++] = key;
        }

        // Handle commas or the closing `}`
        if (peek_eq(s(","))) {
            consume_specific(TT_SEP, s(","), sMSG("Expected `,` or `}`."));
        } else if (!peek_eq(s("}"))) {
            parse_error(p, peek(), sMSG("Expected `,` or `}` in set or dictionary literal."));
        }
    }

    // Consume the closing `}`
    consume_specific(TT_BRA_CLOSE, s("}"), sMSG("Expected `}` to close set or dictionary."));

    // Construct the appropriate AST node
    if (is_dict) {

        /// @todo 
        struct AstDictEntry *kv_pairs =  anew(sizeof(*kv_pairs) * count_keys);
        for(size_t i = 0; i < count_keys; i++) {
            kv_pairs[i].key = keys[i];
            kv_pairs[i].value = values[i];
        }

        return Ast_dict_anew(peek(), kv_pairs, count_keys);
    } else {
        parse_not_implemented();
        return NULL;
        // return Ast_set_anew(peek(), keys, count_keys);
    }
}


/// @brief Parses a type annotation.
/// @todo Implement parsing for complex type annotations as per language specifications.
/// @example
///     const dv : Vec(Int; 32) = range(32)
///     const dd : Dict(Int, Int; 32) = for(v1 <- dv, v2 <- dv) {v1: v2}
///     const stt : Vec(Char; ?) = "Hello World" // static inference
///     const ref st1 : Vec(Char; dyn) = "Hello World"
///     const ref st2 : Vec? = dyn "Hello World"
///     const ref st3 = dyn "Hello World" // infers String(Char; dyn)
///     const dc : Dict(?, ?; ?) = {ch(c): "Hello"}
///     const dcc : Dict? = dc // infer all 
/// @param p The parsing context.
/// @return An AST node representing the type annotation.
Ast *parse_type_annotation(ParseContext *p) {
    pctx_trace(peek()->value);

    // Initialize a new AST node for type annotation
    Ast *type_ann = Ast_type_annotation_anew(peek());

    // Check for qualifiers like 'ref' or 'mut'
    if (peek_eq(s("ref")) || peek_eq(s("mut"))) {
        // Consume the qualifier
        Token *qual_token = consume_expected(TT_KEYWORD);
        type_ann->type_annotation.qualifier = qual_token->value;
    } else {
        // No qualifier present
        type_ann->type_annotation.qualifier = Str_empty();
    }

    // Parse the base type name
    Token *base_type_token = consume(TT_TYPE, sMSG("Expected a base type identifier."));
    type_ann->type_annotation.base_type = base_type_token->value;

    // Initialize type parameters
    type_ann->type_annotation.type_params = NULL;
    type_ann->type_annotation.num_type_params = 0;

    // Check for generic type parameters enclosed in '('
    if (peek_eq_chr('(')) {
        consume_specific(TT_BRA_OPEN, s("("), sMSG("Expected '(' to start type parameters."));

        // Initialize type parameters array
        size_t params_capacity = ARRAY_SIZE_SMALL;
        type_ann->type_annotation.type_params = anew(params_capacity * sizeof(Ast *));
        type_ann->type_annotation.num_type_params = 0;

        while (elements_remain() && !peek_eq_chr(')')) {
            // Parse each type parameter (recursive call)
            Ast *param = parse_type_annotation(p);
            if (param != NULL) {
                type_ann->type_annotation.type_params[type_ann->type_annotation.num_type_params++] = param;
            }

            // Handle comma-separated type parameters
            if (peek_eq_chr(',')) {
                consume_specific(TT_SEP, s(","), sMSG("Expected ',' between type parameters."));
            } else {
                break;
            }

            // Ensure capacity
            require_small_array(params_capacity);
        }

        consume_specific(TT_BRA_CLOSE, s(")"), sMSG("Expected ')' to end type parameters."));
    }

    // Check for size constraint after ';'
    if (peek_eq_chr(';')) {
        consume_specific(TT_SEP, s(";"), sMSG("Expected ';' before size constraint."));
        
        // Size constraint can be an identifier (e.g., 'dyn') or a number (e.g., '32') or '?'
        if (peek_is(TT_IDENTIFIER)) {
            Token *size_token = consume(TT_IDENTIFIER, sMSG("Expected size constraint identifier or value."));
            type_ann->type_annotation.size_constraint = size_token->value;
        }
        else if (peek_eq_chr('?')) {
            // Handle '?' as a size constraint
            consume_expected(TT_OP);
            type_ann->type_annotation.size_constraint = s("?");
        }
        else {
            parse_error(p, peek(), sMSG("Invalid size constraint in type annotation."));
            type_ann->type_annotation.size_constraint = s("?");
        }
    } else {
        type_ann->type_annotation.size_constraint = Str_empty();
    }

    // Check for optional '?', indicating optional type
    if (peek_eq_chr('?')) {
        consume_specific(TT_OP, s("?"), sMSG("Expected '?' indicating an optional type."));
        /// @todo: probably make this a specific flat
        ///     here we use NULL as an indicative value
        type_ann->type_annotation.type_params[0] = NULL;
        type_ann->type_annotation.num_type_params = 1;
    }

    return type_ann;
}



/// @brief 
/// @todo -- is this required? should DEREF always be part of another rule?
/// @note to support universal function calling syntax (monomorphic)
/// @note and field access on structs
/// @example
///     vec.pop()       // becomes typeof(vec)##_pop(vec)
///     record.field 
/// 
///     /// explicit runtime polymorphic resolution 
///     for(x <- [letters, dictionary]) of(x).pop()
///
///     /// becomes
///     for(x <- [letters, dictionary]) call(methodof(typeof(x))[#pop], [x])
/// @param p s
/// @return 
// Ast *parse_dereference(ParseContext *p) {
//     pctx_trace(peek()->value);

//     consume_expected(TT_DEREF);
    


//     Ast *target = parse_expression(p, 0);
//     Str attr = s("debug_placeholder_attr");
//     // Str attr = consume(TT_IDENTIFIER, sMSG("Expected identifier"))->value;
    
//     if(peek_eq_chr('(')) {
//         return Ast_method_call_anew(peek(), target, attr, parse_post_call(p, NULL));
//     } else {
//         return Ast_member_access_anew(peek(), target, attr);
//     }
// }

Ast *parse_unary(ParseContext *p) {
    pctx_trace(peek()->value);

    if(!(peek_eq_chr('#') || peek_eq_chr('!') || peek_eq_chr('-'))) {
        parse_error(p, peek(), sMSG("Unexpected operator."));
        return NULL;
    }

    uint8_t prec = parse_peek_precedence(p);
    Token *t = consume_expected(TT_OP);
    return Ast_uop_anew(peek(), t->value, parse_expression(p, prec));
}


/// @brief 
/// @note vectors are fixed-sized (16 elements)
///         and statically allocated to local arena by default
/// @note if they exceed 16, they will be placed on heap
/// @note use `dyn` to place on to GC'd heap directly
///         marking the variable as `ref` to note it now does not copy when reassigned
///         and `ref` variables are reference counted
///
/// @example
///     let( xs = [2, 4, 6] ) in log(xs)
///     let( ref xs = dyn [2, 4, 6] ) in log(xs.append(10))
/// @param p 
/// @return 
Ast *parse_vec(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_BRA_OPEN, s("["), sMSG("Expected a `[` bracket for vectors."));

    size_t count = 0;
    Ast **items = anew(ARRAY_SIZE_SMALL* sizeof(Ast *));

    while(elements_remain() && !peek_eq_chr(']')) {
        Ast *item = parse_expression(p, 0);
        items[count++] = item;
         
        if(peek_eq_chr(',')) {
            consume_specific(TT_SEP, s(","), sMSG("Expected a comma here"));
        }
        
        require_small_array(count);
        if(count >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many items in vector literal."
                    "The maximum number of items in a literal is 16."
                    "You can create a vector and use `vec.append()` to add more items."
                    "Or, if you want to import data use load_data_file() and a datafile"
                )
            );
            break;
        }
        /// @todo -- consider reallocation or limiting literals
    }

    consume_specific(TT_BRA_CLOSE, s("]"), sMSG("Expected a closing `]` here."));

    return Ast_vec_anew(peek(), items, count);

}

/// @brief 
/// @note dicts are fixed-sized (16 elements)
///         and statically allocated to local arena by default
/// @note if they exceed 16, they will be placed on heap
/// @note use `dyn` to place on to GC'd heap directly
///         marking the variable as `ref` to note it now does not copy when reassigned
///         and `ref` variables are reference counted
///
/// @example
///     let( xs = {"a": "10", "b": "20"} ) in log(xs["a"])
///     let( ref xs = dyn {"a": "10", "b": "20"} ) in log(xs - "a")
/// @param p 
/// @return 
Ast *parse_dict(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_BRA_OPEN, s("{"), sMSG("Expected a `{` bracket for dictionaries."));

    size_t count = 0;
    struct AstDictEntry *items = 
        /// @todo: why is this broken, should we move all the subfields of Ast out?
        ///.... or perhaps move the property parsing into Ast_* ?
        anew(ARRAY_SIZE_SMALL* sizeof(struct AstDictEntry));

    while(elements_remain() && !peek_eq_chr('}')) {
        /// @todo: this expects dict keys to be ...?
        ///     ... but we should generalize this to keys involving ..? 
        items[count].key = parse_expression(p, 0);
        
        consume_specific(TT_SEP, s(":"), sMSG("Expected a colon."));

        items[count].value = parse_expression(p, 0);
         
        if(peek_eq_chr(',')) {
            consume_specific(TT_SEP, s(","), sMSG("Expected a comma here"));
        }
        
        if(count >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many items in dictionary literal."
                    "The maximum number of items in a literal is 16."
                    "You can create a dictionary and use `dict.append()` to add more items."
                    "Or, if you want to import data use load_data_file() and a datafile"
                )
            );
            break;
        }
        require_small_array(count);
    }

    consume_specific(TT_BRA_CLOSE, s("}"), sMSG("Expected a closing `}` here."));
    return Ast_dict_anew(peek(), items, count);
}


/// @brief 
/// @example 
/// @param p 
/// @return 
Ast *parse_literal(ParseContext *p) {
    pctx_trace(peek()->value);

    if(peek_is(TT_TAG)) {
        return Ast_tag_anew(peek(), consume_expected(TT_TAG)->value);
    } 
    else if(peek_is(TT_DOUBLE)) {
        return Ast_double_anew(peek(), Str_to_double(consume_expected(TT_DOUBLE)->value));
    }
    else if(peek_is(TT_INT)) {
        return Ast_int_anew(peek(), Str_to_int(consume_expected(TT_INT)->value));
    }
    else if(peek_is(TT_STRING)) {
        return Ast_string_anew(peek(), Str_trim_delim(consume_expected(TT_STRING)->value));
    }
    else {
        parse_error(p, peek(), sMSG("No such kind of literal."));
        return NULL;
    }
}



/// @brief 
/// @todo -- consider strategy for detecting unmatched brackets
/// @param p 
/// @param callee 
/// @return 
Ast *parse_post_call(ParseContext *p, Ast *callee) {
    pctx_trace(peek()->value);

    consume_specific(TT_BRA_OPEN, s("("), sMSG("Expected `(` for function call."));

    Ast **args = anew(ARRAY_SIZE_SMALL * sizeof(Ast *));
    size_t count = 0;

    while (elements_remain() && !peek_eq_chr(')')) {
        if(count >= 1) {
            consume_specific(TT_SEP, s(","), sMSG("Expected `,` between function arguments. Perhaps missing a closing bracket?"));
        }
        args[count++] = parse_expression(p, 0);
        require_not_null(args[count-1]);
        require_small_array(count);
    }

    consume_specific(TT_BRA_CLOSE, s(")"), sMSG("Expected `)` for function call."));

    if (callee->type == AST_MEMBER_ACCESS) {
        // It's a method call
        return Ast_method_call_anew(peek(), callee->member_access.target, callee->member_access.property, Ast_fn_call_anew(peek(), NULL, args, count));
    } else {
        // It's a regular function call
        return Ast_fn_call_anew(peek(), callee, args, count);
    }
}




/// @brief Parses struct literals (object initialization).
/// @example
///     Person { .name = "Alice", .age = 30 }
/// @param p The parsing context.
/// @return An AST node representing the struct literal.
Ast *parse_object_literal(ParseContext *p) {
    pctx_trace(peek()->value);

    // Parse struct type name
    Str type_name = consume(TT_TYPE, sMSG("Struct literals must begin with a type name."))->value;

    // Expecting opening brace
    consume_specific(TT_BRA_OPEN, s("{"), sMSG("Expected '{' to start struct literal."));

    size_t count = 0;
    struct AstObjectField *fields = anew(ARRAY_SIZE_SMALL * sizeof(struct AstObjectField));

    while(elements_remain() && !peek_eq_chr('}')) {
        /// @todo consider whether we want fields to start with `.`, may need to modify lexer
        // consume_specific(TT_DEREF, s("."), sMSG("Expected '.' before struct field name."));
        Str field_name = consume(TT_IDENTIFIER, sMSG("Expected field name in struct literal."))->value;

        consume_specific(TT_ASSIGN, s("="), sMSG("Expected '=' after struct field name."));
        Ast *field_value = parse_expression(p, 0);
        fields[count].name = field_name;
        fields[count].value = field_value;
        count++;
        require_small_array(count);
        if(count >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many fields in object literal."
                    "The maximum number of fields in a literal is 16."
                    "You can create an object and set fields one-by-one."
                    "Or, if you want to import data use load_data_file() with hydrate()"
                )
            );
            break;
        }

        // Optional comma
        if(peek_eq_chr(',')) {
            consume_specific(TT_SEP, s(","), sMSG("Expected ',' between struct fields."));
        }
    }

    consume_specific(TT_BRA_CLOSE, s("}"), sMSG("Expected '}' to close struct literal."));

    return Ast_object_literal_anew(peek(), type_name, fields, count);
}


/// @brief Parses `match` expressions for pattern matching.
/// @example
///     match value
///         if 1 -> "One"
///         else -> "Other"
/// @param p The parsing context.
/// @return An AST node representing the match expression.
Ast *parse_match(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("match"), sMSG("Expected 'match' keyword."));

    // Parse the expression to match against
    Ast *matched_expr = parse_expression(p, 0);
    consume_expected(TT_END);
    consume_expected(TT_INDENT);

    // Expecting the start of match arms
    struct MatchCase *arms = anew(ARRAY_SIZE_SMALL * sizeof(struct MatchCase));
    size_t arm_count = 0;

    while(elements_remain() && !peek_is(TT_DEDENT)) {
        require_small_array(arm_count);

        // Parse 'if' or 'else' keyword
        if(peek_eq(s("if"))) {
            consume_specific(TT_KEYWORD, s("if"), sMSG("Expected 'if' in match arm."));
            arms[arm_count].condition = parse_expression(p, 0);
            consume_specific(TT_ASSIGN, s("->"), sMSG("Expected '->' after match condition."));
            arms[arm_count].expression = parse_expression(p, 0);
            arm_count++;
            consume_expected(TT_END);
        } else if(peek_eq(s("else"))) {
            consume_specific(TT_KEYWORD, s("else"), sMSG("Expected 'else' in match arm."));
            consume_specific(TT_ASSIGN, s("->"), sMSG("Expected '->' after 'else' in match arm."));
            arms[arm_count].condition =  NULL;
            arms[arm_count].expression = parse_expression(p, 0);
            arm_count++;
            consume_expected(TT_END);
        } else {
            parse_error(p, peek(), sMSG("Unexpected token in match expression."));
            return NULL;
        }

        /// @todo -- something about an optional comma if we allow collapsing match
        ///...... eg., match(x) if T -> 1, if F -> 2
        // if(peek_eq_chr(',')) {
        //     consume_specific(TT_SEP, s(","), sMSG("Expected ',' between match arms."));
        // }
    }

    consume_expected(TT_DEDENT);
    return Ast_match_anew(peek(), matched_expr, arms, arm_count);
}



/// @brief Parses `return` statements for early exits from functions.
/// @example
///     return x + y
/// @param p The parsing context.
/// @return An AST node representing the return statement.
Ast *parse_return(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("return"), sMSG("Expected 'return' keyword."));

    // Optionally parse the return expression
    Ast *ret_expr = NULL;
    if(!peek_is(TT_END) && !peek_eq_chr('\n')) {
        ret_expr = parse_expression(p, 0);
    }

    return Ast_return_anew(peek(), ret_expr);
}


/// @brief Parses `yield` statements used in generator functions.
/// @example
///     yield value
/// @param p The parsing context.
/// @return An AST node representing the yield statement.
Ast *parse_yield(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("yield"), sMSG("Expected 'yield' keyword."));

    // Parse the yield expression
    Ast *yield_expr = parse_expression(p, 0);

    return Ast_yield_anew(peek(), yield_expr);
}



/// @brief Parses `ignore` statements indicating that a block or region should be empty.
/// @example
///     ignore
/// @param p The parsing context.
/// @return An AST node representing the ignore statement.
Ast *parse_ignore(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_expected(TT_IGNORE);

    // Typically, 'ignore' doesn't have an associated expression or block
    return Ast_ignore_anew(peek());
}



/// @brief Parses `trait` defs for defining interfaces or behavior contracts.
/// @example
///     trait Stringy :=
///         fn to_string()
/// @param p The parsing context.
/// @return An AST node representing the trait def.
Ast *parse_trait(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("trait"), sMSG("Expected 'trait' keyword."));

    // Parse trait name
    Str trait_name = consume(TT_IDENTIFIER, sMSG("Expected trait name."))->value;

    // Expecting ':=' to start trait body
    consume_specific(TT_ASSIGN, s(":="), sMSG("Expected ':=' to start trait body."));


    parse_not_implemented();
    return NULL;


    /// @todo
    struct TraitMethod *trait_methods = NULL; 
    size_t num_methods = 0;

    return Ast_trait_anew(peek(), trait_name, trait_methods, num_methods);

    // return Ast_trait_anew(peek(), trait_name, body);
}


/// @brief Parses `struct` defs for defining new data types.
/// @example
///     struct Person :=
///         .name : String = "Default"
///         .age : Int = 0
/// @param p The parsing context.
/// @return An AST node representing the struct def.
Ast *parse_struct(ParseContext *p) {
    pctx_trace(peek()->value);

    consume_specific(TT_KEYWORD, s("struct"), sMSG("Expected 'struct' keyword."));

    // Parse struct name
    Str struct_name = consume(TT_TYPE, sMSG("Expected struct name."))->value;

    // Expecting ':=' to start struct body
    consume_specific(TT_ASSIGN, s(":="), sMSG("Expected ':=' to start struct body."));
    consume_expected(TT_INDENT);

    // parse struct bindings
    // parse_not_implemented();
    // return NULL;

    struct StructField *fields = anew(ARRAY_SIZE_SMALL * sizeof(struct StructField));
    size_t num_fields = 0;

    while(elements_remain() && !peek_is(TT_DEDENT)) {
        // require_small_array(num_fields);

        if(num_fields >= ARRAY_SIZE_SMALL) {
            parse_error(p, peek(), 
                sMSG("Too many fields in struct definition (max 16)."
                    "Group fields into sub-structs using indented struct definitions."
                    "Or consider using a collection type."
                )
            );

            return NULL;
        }

        // @todo -- consider whether `.` should prefix fields -- if so, modify lexer
        // consume_specific(TT_DEREF, s("."), sMSG("Expected '.' before struct field name."));
        Str field_name = consume(TT_IDENTIFIER, sMSG("Expected field name in struct def."))->value;
        Ast *field_type = NULL;
        Ast *field_value = NULL;


        if (peek_eq_chr(':')) {
            consume_specific(TT_SEP, s(":"), sMSG("Expected ':' after struct field name."));
            field_type = parse_type_annotation(p);
        }

        if(peek_eq_chr('=')) {
            consume_specific(TT_ASSIGN, s("="), sMSG("Expected '=' after struct field type."));
            field_value = parse_expression(p, 0);
        }

        consume_expected(TT_END);

        fields[num_fields].name = field_name;
        fields[num_fields].type = field_type;
        fields[num_fields].default_value = field_value;
        num_fields++;
    }

    consume_expected(TT_DEDENT);
    return Ast_struct_anew(peek(), struct_name, fields, num_fields);
}



/// @brief Parses `use` statements for importing modules or specific items.
/// @example
///     use core.collections.*
///     use core.collections as c
/// @param p The parsing context.
/// @return An AST node representing the use statement.
Ast *parse_use(ParseContext *p) {
    pctx_trace(peek()->value);

    parse_not_implemented();
    return NULL;

    consume_specific(TT_KEYWORD, s("use"), sMSG("Expected 'use' keyword."));

    // Parse the module path
    Str module_path = consume(TT_IDENTIFIER, sMSG("Expected module name."))->value;

    // Handle possible sub-paths (e.g., core.collections)
    while(peek_eq_chr('.')) {
        consume_specific(TT_OP, s("."), sMSG("Expected '.' in module path."));
        Str sub_module = consume(TT_IDENTIFIER, sMSG("Expected sub-module name."))->value;
        module_path = Str_fmt_anew(s("%.*s.%.*s"), fmt(module_path), fmt(sub_module));
    }

    // Optional alias
    Str alias = Str_empty();
    if(peek_eq(s("as"))) {
        consume_specific(TT_KEYWORD, s("as"), sMSG("Expected 'as' keyword for alias."));
        alias = consume(TT_IDENTIFIER, sMSG("Expected alias name."))->value;
    }

    // Optional wildcard import
    bool wildcard = false;
    if(peek_eq_chr('*')) {
        consume_specific(TT_OP, s("*"), sMSG("Expected '*' for wildcard import."));
        wildcard = true;
    }

    return Ast_use_anew(peek(), module_path, alias, wildcard);
}



/// @brief Parses mutation operations, marking variables as mutable.
/// @example
///     mut! x += 1
///     mut* y += 2  // Vectorized mutation
/// @param p The parsing context.
/// @return An AST node representing the mutation operation.
Ast *parse_mutation(ParseContext *p) {
    pctx_trace(peek()->value);

    parse_not_implemented();
    return NULL;

    consume_specific(TT_KEYWORD, s("mut"), sMSG("Expected 'mut' keyword."));

    // Determine mutation type: single or vectorized
    bool is_vector = false;
    if(peek_eq_chr('!')) {
        consume_specific(TT_OP, s("!"), sMSG("Expected '!' for vectorized mutation."));
        is_vector = true;
    }
    else if(peek_eq_chr('*')) {
        consume_specific(TT_OP, s("*"), sMSG("Expected '*' for vectorized mutation."));
        is_vector = true;
    }

    // Parse the target of mutation (e.g., variable or field)
    Ast *target = parse_expression(p, 0);

    // Parse the mutation operator (e.g., +=, -=)
    Token *op = consume(TT_OP, sMSG("Expected mutation operator (e.g., '+=', '-=')."));

    // Parse the mutation expression
    Ast *value = parse_expression(p, 0);

    return Ast_mutation_anew(peek(), target, op->value, value, is_vector);
}


void parse_test_main() {
    Str source = s(
        "const test = 1234\n\n"
        "fn main() :=\n"
        "    log(test)\n"
        "\n"
    );

    Str_println(sMSG("Parse Tests:"));
    lex_source(pctx(), source, s("    "));
    parse(pctx());
    Ast_print(pctx()->parser.elements);
    Tracer_print_last(&pctx()->parser.traces);
    
}

// void source_init(Source *src, Str source) {
//     src->lines = Str_lines(source, &src->size);

//     require_not_null(src->lines);
//     require_positive(src->size);
// }

// void parse(ParseContext *pctx, Str source, Str indent) {
//     source_init(&pctx->source, source);

//     require_not_null(pctx->lexer.elements);
//     require_positive(pctx->lexer.size);
// }

#pragma endregion
//#endregion

//#region Parsing_TestMain
#pragma region TestMain
void parsing_test_main() {
    lex_test_main();
    parse_test_main();
}
#pragma endregion
//#endregion


/// @file native.c

//#region Native_ErrorImpl
#pragma region NativeErrorImpl

#ifdef DEBUG_REQUIRE_ASSERTS
    #define require_scope_has(scope, key) assert(BoxScope_has(scope, key))
#else
    /// @todo swap out all checks
    #define require_not_null(expr) 
#endif

// #define NATIVE_LOG_CALLS
// #define NATIVE_DEBUG_VERBOSE 

#ifdef NATIVE_DEBUG_VERBOSE
    #define native_log_debug(...)\ 
        log_message(LL_DEBUG, __VA_ARGS__)
#else
    #define native_log_debug(...) NOOP
#endif

#ifdef NATIVE_LOG_CALLS
    #define native_log_call(self, args)\
        log_message(LL_INFO, s("Native fn: %.*s(...)"), fmt(self->name));\
        BoxArray_debug_print(s("Args"), &args);
    #define native_log_call0(self, one)\
        log_message(LL_INFO, s("Native fn: %.*s()"), fmt(self->name));
    #define native_log_call1(self, one)\
        log_message(LL_INFO, s("Native fn: %.*s(%.*s)"),\
            fmt(self->name), fmt(Box_to_str(one)));
    #define native_log_call2(self, one, two)\
        log_message(LL_INFO, s("Native fn: %.*s(%.*s, %.*s)"),\
            fmt(self->name), fmt(Box_to_str(one)), fmt(Box_to_str(two)));
#else
    #define native_log_call(self, args)
    #define native_log_call1(self, one)
    #define native_log_call2(self, one, two)
#endif

#define native_return_error_if(test, ...) \
    if (test) return Box_wrap_error(native_error(__VA_ARGS__));\

#pragma endregion
//#endregion

//#region Native_BoxGenericImpl
#pragma region BoxGenericImpl

#define Box_return_noimpl()\
    native_return_error_if(true, sMSG("Not yet implemented"));
 

bool Box_eq(Box a, Box b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case BT_INT:
            return Box_unwrap_int(a) == Box_unwrap_int(b);
        case BT_STATE:
            return Box_unwrap_bool(a) == Box_unwrap_bool(b);
        case BT_FLOAT:
            return Box_unwrap_float(a) == Box_unwrap_float(b);
        default:
            return a.payload == b.payload;
    }

}

Str Box_to_str(Box b) {
    /// @todo refactor as _to_repr for all types
    switch (b.type) {
        case BT_INT:
            return Str_fmt_anew(s("%lld"), Box_unwrap_int(b));
        case BT_STATE:
            if(Box_is_state(b, BS_TRUE)) return s("true");
            if(Box_is_state(b, BS_FALSE)) return s("false");
            if(Box_is_state(b, BS_NULL)) return s("null");
            return Str_fmt_anew(s("State(%d)"), Box_unwrap_int(b));
        case BT_TAG:
            return Str_fmt_anew(s("Tag(%.*s)"), fmt(Box_unwrap_tag(b)));
        case BT_FLOAT:
        case BT_DYN_DOUBLE: /// @todo impl doubles
            return Str_fmt_anew(s("%.6f"), Box_unwrap_float(b));
        // case BT_BOX_PTR:
            // return Str_fmt_anew(s("BoxPtr(%p)"), Box_unwrap_Box_ptr(b));
        case BT_BOX_ERROR:
            return Str_fmt_anew(s("Error(%.*s)"), fmt(Box_unwrap_error(b)->message));
        case BT_BOX_STRING:
        case BT_DYN_STR :
            Str str = Box_unwrap_str(b);
            return str;
            // return Str_fmt_anew(s("'%.*s'"), fmt(str));
        case BT_BOX_FN:
            BoxFn *f = Box_unwrap_fn(b);
            return Str_fmt_anew(s("Fn(%.*s)"), fmt(f->name));
        case BT_BOX_ARRAY:
            return BoxArray_to_repr(Box_unwrap_array(b));
        case BT_BOX_DICT:
            return BoxDict_to_repr(Box_unwrap_dict(b));
        case BT_DYN_COLLECTION:
            return DynCollection_to_repr(Box_unwrap_DynCollection(b));
        case BT_DYN_OBJECT :
            return DynObject_to_repr(Box_unwrap_object(b));
        case BT_BOX_STRUCT:
            return sMSG("struct-todo");
        default:
            return Str_fmt_anew(s("Box?(%d)"), b.type);
    }
}

bool Box_try_numeric(Box box, double *out_value) {
    switch(box.type) {
        case BT_INT: 
            *out_value = (double)Box_unwrap_int(box);
            return true;
        case BT_FLOAT:
            *out_value = (double)Box_unwrap_float(box);
            return true;
        case BT_DYN_DOUBLE:
            *out_value = (double)Box_unwrap_double(box);
            return true;
        default:
            return false;
    }
}


/// @brief Combines BoxType and payload into a single 64-bit key and applies bit mixing
size_t Box_hash(Box box) {
    uint64_t key;

    /// @note strings can be hashed on their pointers if they've been interned
    /// ....    ie., they're guaranteed to be pointer-stable 
    /// @todo ensure all strings used as keys on Box'd maps are interned
    /// ....    unclear on pro/con for similar on dynamic dicts
    /// ....    rules for interning: (key of fixed dict) || (< 16 char)
    if(box.type == BT_BOX_STRING) {
        return Str_hash(Box_unwrap_str(box));

        #ifdef INTERP_PRECACHE_INTERNED_STRINGS
        /// @todo
        // BoxKvPair out;
        // if(!interp_precache_getstr(Box_unwrap_str(box), &out)) {
        //     /// @note implementing FNV-1a 64-bit hash for the string
        //     log_message(LL_INFO, sMSG("Hashing an non-interned string"));
        //     return Str_hash(Box_unwrap_str(box));
        // }
        #endif
    } 
    

    /// @note doubles are the only numeric types which are allocated
    ///     ... so we deref to find their value
    if(box.type == BT_DYN_DOUBLE) {
        key = *(double *)box.payload;
    } else {
        /// everything else can be hashed on their pointers
        /// @todo ... this assumes their pointers won't move
        key = ((uint64_t)box.type << 60) | (box.payload & 0x0FFFFFFFFFFFFFFFUL);
    }

    key ^= key >> 33;
    key *= 0xff51afd7ed558ccd;
    key ^= key >> 33;
    key *= 0xc4ceb9fe1a85ec53;
    key ^= key >> 33;

    return (size_t)key;
}




void test_wrap_unwrap_double() {
    double original = 3.141592653589793;

    /// @todo should original be an address?
    Box boxed = Box_wrap_double(&original);
    double ref = Box_unwrap_double(boxed);

    log_assert(original == ref, sMSG("Doubles not unpacking"));
}


void test_wrap_unwrap_float() {
    float original = 3.141592653589793;
    Box boxed = Box_wrap_float(original);
    
    float unboxed = Box_unwrap_float(boxed);
    log_assert(original == unboxed, sMSG("Floats not unpacking"));
}


void Box_test_main() {
    test_wrap_unwrap_double();
    test_wrap_unwrap_float();
}

#pragma endregion
//#endregion 

//#region Native_BoxApiImpl
#pragma region BoxApiImpl


DynStr *DynStr_from_Str_gcnew(Str str) {
    DynStr *dynstr = gcnew(sizeof(DynStr), BT_DYN_STR);
    Str gc = Str_cnew_from(str);
    dynstr->cstr = gc.cstr;
    dynstr->meta.size = gc.size;
    return dynstr;
}

/// @brief
/// @note 
///     most boxes are going to come from user land, so will arrive as a boxed pointer
///     ... so the API may as well assume everything is a pointer
///     Even though we will create some of these on the stack
/// @todo -- change everything to Box * 


void Box_print(Box box) {
    Str str = Box_to_str(box);
    log_message(LL_INFO, sMSG("Box: %.*s"), fmt(str));
    // Str_adel(str);
}

void BoxArray_elements_anew(BoxArray *array, size_t size) {
    if(size == 0) {
        array->elements = NULL;
        return;
    }
    
    array_init_ref(anew, array, size);
}

Str BoxArray_to_repr(BoxArray *array) {
    if(len_ref(array) == 0) {
        return s("[]");
    }

    Str repr = s("[");
    for(size_t i = 0; i < len_ref(array); i++) {
        repr = Str_glue_sep_anew(repr, Box_to_str(array->elements[i]), s(", "));
    }
    repr = Str_glue_anew(repr, s("]"));
    return repr;
}


/// @brief 
/// @todo -- does this make sense, does size change?
/// @param box_array 
/// @param element 
/// @param index 
/// @return 
Box BoxArray_set(BoxArray *box_array, Box element, size_t index) {
    native_return_error_if(index > capacity_ref(box_array), s("BoxArray.set(): Index out of bounds."));

    /// @note BoxArrays do not resize, 
    // nor can we change their size, so we need to know the index
    box_array->elements[index] = element;
    return element;
}

Box BoxArray_append(BoxArray *array, Box element) {
    require_not_null(array);
    require_positive(capacity_ref(array));

    native_return_error_if(len_ref(array) >= capacity_ref(array), s("BoxArray.append(): Array is full."));

    array->elements[incr_len_ref(array)] = element;
    return element;
}

/// @brief 
/// @todo -- maybe refactor with _to_repr
/// @param array 
void BoxArray_debug_print(Str name, BoxArray *array) {
    if(array == NULL) {
        log_message(LL_INFO, s("%.*s: NULL"), fmt(name));
        return;
    }

    if(len_ref(array) == 0) {
        log_message(LL_INFO, s("%.*s: []"), fmt(name));
        return;
    }

    for(size_t i = 0; i < len_ref(array); i++) {
        Str str = Box_to_str(array->elements[i]);
        log_message(LL_INFO, s("%.*s[%d]: %.*s"), fmt(name), i, fmt(str));
        // Str_adel(str);
    }
}

BoxArray *BoxArray_anew(size_t fixed_capacity) {
    BoxArray *array = anew(sizeof(BoxArray));
    if(array == NULL) {error_oom(); }
    array->meta.capacity = fixed_capacity;
    array->meta.size = 0;
    if(fixed_capacity > 0) {
        BoxArray_elements_anew(array, fixed_capacity);
    }
    return array;
}


Box BoxArray_get(BoxArray array, size_t index) {
    native_return_error_if(index >= len(array), s("BoxArray.get(): Index out of bounds."));
    return array.elements[index];
}

Box BoxArray_overwrite(BoxArray *array, size_t index, Box element) {
    native_return_error_if(index >= len_ref(array), s("BoxArray.overwrite(): Index out of bounds."));
    array->elements[index] = element;
    return element; 
}

Box BoxArray_weak_unset(BoxArray *array, size_t index) {
    native_return_error_if(index >= len_ref(array), s("BoxArray.weak_unset(): Index out of bounds."));
    array->elements[index] = Box_null();
    return Box_null(); // Added return statement
}




/// @brief 
/// @example
///     const vec1 = [10, 20, 30]
///     const vec2 = Vector {10, 20, 30}
///     const set1 = {1, 1, 2, 3}
///     const set2 = Set {1, 1, 2, 3}
///     const dict1 = {1: "Hello"}
///     const dict2 = Dict {{1, "Hello"}, {2, "Goodbye"}}
///     const stack1 = Stack {1, 2, 3}
///     const queue1 = Queue {1, 2, 3}
/// @param initial_capacity 
/// @return 

DynArray *DynArray_gcnew(size_t initial_capacity) {
    DynArray *array = gcnew(sizeof(DynArray), BT_DYN_COLLECTION);
    if(array == NULL) { error_oom(); return NULL; }

    array->meta.type = DYNCOL_ARRAY;


    // array->meta.capacity = initial_capacity;
    // array->meta.size = 0;
    // array->elements = cnew(initial_capacity, sizeof(Box));
    // if(array->elements == NULL) { error_oom(); return NULL;}

    array_init_ref(cnew, array, initial_capacity);
    return array;
}

/// @brief Append an element to the end of the array.
/// @note Use reallocation and HeapArray_next_capacity 
///     ... to resize the array
Box DynArray_append(DynArray *array, Box element) {
    require_not_null(array);
    require_positive(capacity_ref(array));
    if(is_empty_ref(array)) {
        size_t new_capacity = HeapArray_next_capacity(capacity_ref(array));
        Box *new_elements = cextend(array->elements, new_capacity * sizeof(Box));
        if(new_elements == NULL) { error_oom(); return Box_error_empty(); }
        array->elements = new_elements;
        capacity_ref(array) = new_capacity;
    }

    push_ref(array, element);
    return element;
}

Box DynArray_get(const DynArray *array, size_t index) {
    require_not_null(array);
    native_return_error_if(index >= len_ref(array), s("DynArray.get(): Index out of bounds."));
    return array->elements[index];
}

Box DynArray_overwrite(DynArray *array, size_t index, Box element);
Box DynArray_weak_remove(DynArray *array, size_t index) ;

void DynArray_adel(DynArray *array) ;

void DynArray_debug_print(const DynArray *array);


DynDict *DynDict_gcnew(size_t initial_capacity) {
    DynDict *dict = gcnew(sizeof(DynDict), BT_DYN_COLLECTION);
    if(dict == NULL) { error_oom(); return NULL; }

    dict->meta.type = DYNCOL_DICT;
    array_init_ref(cnew, dict, initial_capacity);
    return dict;
}



bool DynDict_find(DynDict *dict, Box key, DynDictEntry *out);

void DynDict_adel(DynDict *dict);

/// @brief  Set a key-value pair in the dictionary.
/// @note   If the key already exists, the value will be updated.
/// @note       uses probing and open addressing to handle collisions
/// @note   struct DynDictEntry { size_t hash; Box key; Box value; bool occupied; }
/// @param DynDict *dict 
/// @param Box key 
/// @param Box val 
/// @return Box 
Box DynDict_set(DynDict *dict, Box key, Box val) {
    require_not_null(dict);

    size_t hash = Box_hash(key);
    size_t index = hash % capacity_ref(dict);
    size_t start = index;

    while(dict->elements[index].occupied) {
        if(Box_eq(dict->elements[index].key, key)) {
            dict->elements[index].value = val;
            return val;
        }
        index = (index + 1) % capacity_ref(dict);

        /// @note reallocate
        if(index == start) {
            size_t new_capacity = HeapArray_next_capacity(capacity_ref(dict));
            DynDictEntry *new_elements = cextend(dict->elements, new_capacity * sizeof(DynDictEntry));
            if(new_elements == NULL) { error_oom(); return Box_error_empty(); }
            dict->elements = new_elements;
            dict->meta.capacity = new_capacity;
            index = hash % capacity_ref(dict);
        }
    }

    dict->elements[index].hash = hash;
    dict->elements[index].key = key;
    dict->elements[index].value = val;
    dict->elements[index].occupied = true;
    incr_len_ref(dict);
    return val;
}

bool DynDict_find(DynDict *dict, Box key, DynDictEntry *out) {
    require_not_null(dict);

    size_t hash = Box_hash(key);
    size_t index = hash % capacity_ref(dict);
    size_t start = index;

    while(dict->elements[index].occupied) {
        if(Box_eq(dict->elements[index].key, key)) {
            *out = dict->elements[index];
            return true;
        }
        index = (index + 1) % capacity_ref(dict);
        if(index == start) {
            return false;
        }
    }

    return false;
}

Box DynDict_get(DynDict *dict, Box key) {
    require_not_null(dict);

    DynDictEntry entry;
    if(DynDict_find(dict, key, &entry)) {
        return entry.value;
    }

    return Box_missing();
}



Str DynObject_to_repr(DynObject *obj) {
    Str template_name = obj->data->name;
    // Str doc_string = obj->data->doc_string;
    Str fields = BoxDict_to_repr(&obj->data->namespace.locals);
    Str repr = Str_fmt_anew(s("<%.*s: %.*s>"), fmt(template_name), fmt(fields));
    return repr;
}

Str DynArray_to_repr(DynArray *array) {
    Str repr = s("[");
    for(size_t i = 0; i < dyn_size(array); i++) {
        repr = Str_glue_sep_anew(repr, Box_to_str(array->elements[i]), s(", "));
    }
    repr = Str_glue_anew(repr, s("]"));
    return repr;
}

Str DynDict_to_repr(DynDict *dict) {
    Str repr = s("{");
    for(size_t i = 0; i < dyn_size(dict); i++) {
        repr = Str_glue_sep_anew(repr, Box_to_str(dict->elements[i].key), s(": "));
        repr = Str_glue_sep_anew(repr, Box_to_str(dict->elements[i].value), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}

Str DynSet_to_repr(DynSet *set) {
    Str repr = s("{");
    for(size_t i = 0; i <  dyn_size(set); i++) {
        repr = Str_glue_sep_anew(repr, Box_to_str(set->elements[i]), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}


Str DynVecDouble_to_repr(DynVecDouble *vec) {
    Str repr = s("VecDouble {");
    for(size_t i = 0; i < dyn_size(vec); i++) {
        double value = vec->elements[i];
        repr = Str_glue_sep_anew(repr, Str_fmt_anew(s("%.6f"), value), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}

Str DynVecBool_to_repr(DynVecBool *vec) {
    Str repr = s("VecBool {");
    for(size_t i = 0; i < dyn_size(vec); i++) {
        bool value = vec->elements[i];
        repr = Str_glue_sep_anew(repr, value ? s("true") : s("false"), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}

Str DynVecInt_to_repr(DynVecInt *vec) {
    Str repr = s("VecInt {");
    for(size_t i = 0; i < dyn_size(vec); i++) {
        int value = vec->elements[i];
        repr = Str_glue_sep_anew(repr, Str_fmt_anew(s("%d"), value), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}

Str DynVecChar_to_repr(DynVecChar *vec) {
    Str repr = s("VecChar {");
    for(size_t i = 0; i < dyn_size(vec); i++) {
        char cstr = vec->elements[i];
        repr = Str_glue_sep_anew(repr, Str_fmt_anew(s("'%c'"), cstr), s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}

Str DynVecStr_to_repr(DynVecStr *vec) {
    Str repr = s("VecStr {");
    for(size_t i = 0; i < dyn_size(vec); i++) {
        Str value = vec->elements[i];
        repr = Str_glue_sep_anew(repr, value, s(", "));
    }
    repr = Str_glue_anew(repr, s("}"));
    return repr;
}


Str DynCollection_to_repr(DynCollection *col) {
    switch(dyn_type(col)) {
        case DYNCOL_ARRAY:
            return DynArray_to_repr((DynArray *) col);
        case DYNCOL_DICT:
            return DynDict_to_repr((DynDict *) col);
        case DYNCOL_SET:
            return DynSet_to_repr((DynSet *) col);
        case DYNCOL_QUEUE:
            return s("dyn(queue-todo)");
        case DYNCOL_STACK:
            return s("dyn(stack-todo)");
        case DYNCOL_VEC_DOUBLE:
            return DynVecDouble_to_repr((DynVecDouble *) col);
        case DYNCOL_VEC_BOOL:
            return DynVecBool_to_repr((DynVecBool *) col);
        case DYNCOL_VEC_INT:
            return DynVecInt_to_repr((DynVecInt *) col);
        case DYNCOL_VEC_CHAR:
            return DynVecChar_to_repr((DynVecChar *) col);
        case DYNCOL_VEC_STR:
            return DynVecStr_to_repr((DynVecStr *) col);
        default:
            return s("dyncol(unknown)");
    }
}

/// @brief Retrieves an attribute from a DynObject without specifying the type.
/// @param obj The DynObject from which to retrieve the attribute.
/// @param name The name of the attribute.
/// @return The Box representing the attribute's value, or Box_error() if not found.
Box DynObject_get_attribute(DynObject *obj, Str name) {
    require_not_null(obj);

    // Lookup the attribute in the object's namespace
    Box value = BoxDict_get(&obj->data->namespace.locals, name);

    native_return_error_if(Box_is_error(value), s("Attribute `%.*s` not found in object"), fmt(name));

    return value;
}

/// @brief Retrieves an attribute from a DynObject with  type checking.
/// @param obj The DynObject from which to retrieve the attribute.
/// @param name The name of the attribute.
/// @param expected_type The expected BoxType of the attribute. Use BT_INVALID if no type checking is needed.
/// @return The Box representing the attribute's value, or Box_error() if not found or type mismatch.
Box DynObject_get_attribute_checked(DynObject *obj, Str name, BoxType expected_type) {
    require_not_null(obj);

    Box value = BoxDict_get(&obj->data->namespace.locals, name);

    native_return_error_if(Box_is_error(value), s("Attribute `%.*s` not found in object"), fmt(name));
    
    native_return_error_if(value.type != expected_type, 
        s("Attribute `%.*s` expected to be of type %s, but got %s"), 
        fmt(name), bt_nameof(expected_type), bt_nameof(value.type));

    return value;
}


/// @brief 
/// @todo -- tidy up
/// @param obj 
/// @param scope 
/// @param name 
/// @return 
Box DynObject_resolve_method(Box obj, BoxScope *scope, Str name) {
    require_not_null(scope);

    Box fn; 
    if(obj.type == BT_DYN_OBJECT)  {
        /// @note we first look for a method in the object's namespace
        if(BoxScope_lookup(&Box_unwrap_object(obj)->data->namespace, name, &fn)) {
            native_return_error_if(fn.type != BT_BOX_FN, s("Attribute `%.*s` is not a function"), fmt(name));
            Box_unwrap_fn(fn)->enclosure.parent = &Box_unwrap_object(obj)->data->namespace;
            return fn;
        } 

        /// @note then we look for a global/parent-scope function to support universal calling
        Str method_name = Str_glue_sep_anew(Box_unwrap_object(obj)->data->name, s("_"), name);
        if(BoxScope_lookup(scope, method_name, &fn)) {
            native_return_error_if(fn.type != BT_BOX_FN, s("Attribute `%.*s` is not a function"), fmt(method_name));
            Box_unwrap_fn(fn)->enclosure.parent = &Box_unwrap_object(obj)->data->namespace;
            return fn;
        } 
    } else {    
        /// @note we then look for a global/parent-scope generic method
        if(BoxScope_lookup(scope, name, &fn)) {
            native_return_error_if(fn.type != BT_BOX_FN, s("Attribute `%.*s` is not a function"), fmt(name));
            return fn;
        }    
    }

    native_return_error_if(true, s("Method `%.*s` not found in object"), fmt(name));
}


Str BoxDict_to_repr(BoxDict *d) {
    require_not_null(d);

    if(len_ref(d) == 0) {
        return s("{}");
    }

    Str repr = s("{");
    for(size_t i = 0; i < capacity_ref(d); i++) {
        BoxKvPair *current = d->elements[i];
        while(current != NULL) {
            repr = Str_glue_sep_anew(repr, s(" "), current->key);
            repr = Str_glue_sep_anew(repr, s(": "), Box_to_str(current->value));
            current = current->next;

            if(current != NULL) {
                repr = Str_glue_anew(repr, s(", "));
            }
        }
    }
    repr = Str_glue_anew(repr, s(" }"));
    return repr;
}

#define BoxDict_bucket_for_key(dict, key) ((dict)->elements[Str_hash(key) % (dict)->meta.capacity])

/// @brief Initialize a BoxDict with a specified number of elements for separate chaining.
void BoxDict_elements_anew(BoxDict *dict, size_t size) {
    require_not_null(dict);
    if(size == 0) size = ARRAY_SIZE_SMALL;
    array_init_ref(anew, dict, size);
}



/// @brief Get a value from a BoxDict by key, return Box_null() if not found.
Box BoxDict_get(BoxDict *dict, Str key) {
    require_not_null(dict);

    BoxKvPair entry;
    if (BoxDict_find(dict, key, &entry)) {
        return entry.value;
    } else {
        return Box_missing();
    }
}

Str BoxDict_keys_to_str(BoxDict *dict) {
    require_not_null(dict);

    Str result = Str_empty();
    for (size_t i = 0; i < capacity_ref(dict); i++) {
        BoxKvPair *current = dict->elements[i];
        while (current != NULL) {
            result = Str_glue_sep_anew(result, s(", "), current->key);
            current = current->next;
        }
    }
    return result;
}

void BoxDict_debug_print_entry(BoxKvPair *entry) {
    printf("%.*s: %.*s\n", fmt(entry->key), fmt(Box_to_str(entry->value)));
}

void BoxDict_debug_print(BoxDict dict) {
    printf("BoxDict (size: %zu, capacity: %zu):\n", len(dict), cap(dict));
    for (size_t i = 0; i < cap(dict); i++) {
        BoxKvPair *current = dict.elements[i];
        while (current != NULL) {
            BoxDict_debug_print_entry(current);
            current = current->next;
        }
    }
}

bool BoxDict_find(BoxDict *dict, Str key, BoxKvPair *out) {
    require_not_null(dict);
    if(len_ref(dict) == 0) return false;

    // require_positive(dict->meta.capacity);

    size_t hash = Str_hash(key);
    size_t index = hash % capacity_ref(dict);

    BoxKvPair *current = dict->elements[index];
    while (current != NULL) {
        if (current->hash == hash && Str_eq(current->key, key)) {
            *out = *current;
            native_log_debug(sMSG("Found key '%.*s' in bucket %zu"), fmt(key), index);
            return true;
        }
        current = current->next;
    }

    native_log_debug(sMSG("Key '%.*s' not found (looking in bucket %zu)"), fmt(key), index);
    return false;
}


void BoxDict_append(BoxDict *dict, Str key, Box value)  {
    require_not_null(dict);
    require_positive(capacity_ref(dict));

    /// @note we require the key to be unique
    BoxKvPair entry = {0};
    if(BoxDict_find(dict, key, &entry)) {
        native_error(s("`%.*s` already exists"), fmt(key));
    } else {
        /// @note if it is, we set 
        BoxDict_set(dict, key, value);
    }

}

BoxKvPair *BoxKvPair_anew(Str key, Box value) {
    BoxKvPair *pair = anew(sizeof(BoxKvPair));
    if(pair == NULL) {
        error_oom();
    }
    pair->key = key;
    pair->value = value;
    pair->hash = Str_hash(key);
    pair->next = NULL;
    return pair;
}

/// @brief initialises a new dict with `pairs` as key-value pairs
///     faster than inserting each pair individually since 
///         we don't check for existing keys
void BoxDict_from_pairs(BoxDict *empty_dict, BoxKvPair **pairs, size_t num_pairs) {
    require_not_null(empty_dict);
    require_positive(capacity_ref(empty_dict));
    require_not_null(pairs);
    
    if(num_pairs == 0) {
        return;
    }

    /// @note we don't check for existing keys, so we don't use BoxDict_set
    for(size_t i = 0; i < num_pairs; i++) {
        size_t index = pairs[i]->hash % capacity_ref(empty_dict);
        pairs[i]->next = empty_dict->elements[index];
        empty_dict->elements[index] = pairs[i];
        len_ref(empty_dict)++;
    }
}

/// @brief Insert or update a key-value pair in BoxDict using separate chaining.
void BoxDict_set(BoxDict *dict, Str key, Box value) {
    require_not_null(dict);
    require_positive(capacity_ref(dict));

    #ifdef INTERP_PRECACHE_INTERNED_STRINGS
        /// @note Intern all dict keys for BoxDicts
        interp_precache_setstr(key);
    #endif

    size_t hash = Str_hash(key);
    size_t index = hash % capacity_ref(dict);

    BoxKvPair *current = dict->elements[index];
    while (current != NULL) {
        if (current->hash == hash && Str_eq(current->key, key)) {
            // Key exists, update the value
            current->value = value;
            return;
        }
        current = current->next;
    }

    // Key not found, create a new entry and add it to the front of the chain
    BoxKvPair *new_entry = (BoxKvPair *)anew(sizeof(BoxKvPair));
    if (new_entry == NULL) {
        error_oom();
    }
    new_entry->hash = hash;
    new_entry->key = key;
    new_entry->value = value;
    new_entry->next = dict->elements[index];
    dict->elements[index] = new_entry;
    incr_len_ref(dict);
}


BoxKvPair *BoxDict_remove(BoxDict *dict, Str key) {
    require_not_null(dict);
    require_positive(capacity_ref(dict));

    size_t hash = Str_hash(key);
    size_t index = hash % capacity_ref(dict);

    BoxKvPair *current = dict->elements[index];
    BoxKvPair *prev = NULL;
    while (current != NULL) {
        if (current->hash == hash && Str_eq(current->key, key)) {
            if (prev == NULL) {
                dict->elements[index] = current->next;
            } else {
                prev->next = current->next;
            }
            decr_len_ref(dict);
            return current;
        }
        prev = current;
        current = current->next;
    }
    return NULL;
}

/// @brief Free all memory associated with BoxDict.
/// @todo consider this -- fixes should be on an arena and freed in bulk
void BoxDict_adel(BoxDict *dict) {
    require_not_null(dict);

    for (size_t i = 0; i < capacity_ref(dict); i++) {
        BoxKvPair *current = dict->elements[i];
        while (current != NULL) {
            BoxKvPair *to_adel = current;
            current = current->next;
            // If keys or values need special handling, free them here
            adel(to_adel);
        }
    }
    adel(dict->elements);
    dict->elements = NULL;
    dict->meta.size = 0;
    dict->meta.capacity = 0;
}



void BoxScope_locals_anew(BoxScope *self, BoxScope *parent) {
    require_not_null(self);
    require_not_null(parent);

    self->parent = parent;
    BoxDict_elements_anew(&self->locals, ARRAY_SIZE_SMALL);
}


void BoxScope_sized_anew(BoxScope *self, BoxScope *parent, size_t num_locals) {
    self->parent = parent;
    BoxDict_elements_anew(&self->locals, num_locals);
}

void BoxScope_debug_print(BoxScope scope) {
    /// @note print all the locals and the parent
    log_message(LL_INFO, sMSG("Printing Scope"));
    if(scope.doc_string.size > 0) Str_println(scope.doc_string);

    /// @note we will use `BoxDict_debug_print_entry` to print each entry
    for(size_t i = 0; i < scope.locals.meta.capacity; i++) {
        BoxKvPair *current = scope.locals.elements[i];
        while(current != NULL) {
            BoxDict_debug_print_entry(current);
            current = current->next;
        }
    }

    if(scope.parent != NULL) {
        printf("Parent Scope:\n");
        BoxScope_debug_print(*scope.parent);
    }
}

/// @brief 
/// @param dest 
/// @param src 
/// @todo -- is a parent merge necessary?
void BoxScope_merge_local(BoxScope *dest, BoxScope *src) {
    require_not_null(dest);
    require_not_null(src);

    BoxDict_merge(&dest->locals, &src->locals);
}

void BoxDict_merge(BoxDict *dest, BoxDict *src) {
    require_not_null(dest);
    require_not_null(src);

    for(size_t i = 0; i < capacity_ref(src); i++) {
        BoxKvPair *current = src->elements[i];
        while(current != NULL) {
            /// @todo this should be checked?
            BoxDict_set(dest, current->key, current->value);
            current = current->next;
        }
    }
}

bool BoxScope_lookup(BoxScope *scope, Str name, Box *out_value) {
    BoxKvPair entry;
    while (scope != NULL) {
        if (BoxDict_find(&scope->locals, name, &entry)) {
            *out_value = entry.value;
            native_log_debug(sMSG("Found variable '%.*s'"), fmt(name));
            return true;
        }
        scope = scope->parent;
    }

    native_log_debug(sMSG("Variable '%.*s' not found"), fmt(name));
    return false;
}


bool BoxScope_has(BoxScope *scope, Str name) {
    Box value;
    return BoxScope_lookup(scope, name, &value);
}

/// @brief 
/// @param scope 
/// @todo all fixed stuff should be on an arena and freed in bulk?
void BoxScope_adel(BoxScope *scope) {
    return; 
    /// @todo free all the strings in the dict
    
}

static BoxFn *BoxFn_anew(BoxFnType type, Str name, 
    BoxDict args_defaults, BoxScope enclosure, void *fnptr, void *code) {

    BoxFn *fn = anew(sizeof(BoxFn));
    if (!fn) { 
        error_oom();
    }
    fn->type = type;
    fn->name = name;
    fn->signature = args_defaults;
    fn->enclosure = enclosure;
    fn->fnptr = fnptr;
    fn->code = code;
    return fn;
}
 

Box BoxFn_call(BoxFn *ffn, BoxScope scope, BoxArray args) {
    require_not_null(ffn);
    require_not_null(ffn->fnptr);

    switch(ffn->type) {
        case FN_USER:
            return BoxFn_typed_call(BoxFnUser, ffn, scope, args);
        case FN_GENERATOR:
            /// @note calling convention for generators requires
            ///     the last argument to be an iterator
            /// this is handled by interpreter when constructing the call
            BoxIterator *_iter = Box_unwrap_BoxIterator(last(args));
            return BoxFn_typed_call(BoxFnGenerator, ffn, scope, args, *_iter);
        case FN_NATIVE:
            return BoxFn_typed_call(BoxFnNative, ffn, scope, args);
        case FN_NATIVE_0:
            return BoxFn_typed_call(BoxFnNative0, ffn, scope);
        case FN_NATIVE_1:
            return BoxFn_typed_call(BoxFnNative1, ffn, scope, at(args, 0));
        case FN_NATIVE_2:
            return BoxFn_typed_call(BoxFnNative2, ffn, scope, at(args, 0), at(args, 1));
        case FN_NATIVE_3:
            return BoxFn_typed_call(BoxFnNative3, ffn, scope, at(args, 0), at(args, 1), at(args, 2));
        default:
            native_return_error_if(true, sMSG("Unknown function type: %d"), ffn->type);
    }
}

BoxStruct *BoxStruct_anew(Str name, size_t num_fields) {
    BoxStruct *s = anew(sizeof(BoxStruct));
    if (!s) { 
        error_oom();
    }
    s->name = name;
    // s->fields;
    BoxDict_elements_anew(&s->fields, num_fields);
    return s;
}

DynObject *DynObject_gcnew(BoxStruct *def) {
    DynObject *obj = anew(sizeof(DynObject));
    if (!obj) { 
        error_oom();
    }
    obj->data = def;

    /// @todo consider revising struct to have a BoxScope
    /// ... and then we can just copy the struct's fields into the object's locals?
    /// ... this would also allow for inheritance from struct to object 
    BoxScope_sized_anew(&obj->data->namespace, NULL, def->fields.meta.size);
    return obj;
}

DynObject *DynObject_from_BoxDict(BoxStruct *def, BoxDict fields) {
    DynObject *obj = DynObject_gcnew(def);
    BoxDict_merge(&obj->data->namespace.locals, &fields);
    return obj;
}

void DynObject_set_field(DynObject *obj, Str field, Box value) {
    BoxDict_set(&obj->data->namespace.locals, field, value);
}

bool DynObject_get_field(DynObject *obj, Str field, Box *out_value) {
    return BoxScope_lookup(&obj->data->namespace, field, out_value);
}




int BoxArray_test_main() {
    BoxArray array;
    BoxArray_elements_anew(&array, 10);
    log_assert(array.elements != NULL, sMSG("Failed to initialize BoxArray elements"));

    Box val = Box_wrap_int(42);
    Box added = BoxArray_overwrite(&array, 0, val);
    log_assert(added.type == BT_INT, sMSG("BoxArray_overwrite did not set type to BT_INT"));
    log_assert(Box_unwrap_int(added) == 42, sMSG("BoxArray_overwrite did not set correct value"));

    Box retrieved = BoxArray_get(array, 0);
    log_assert(retrieved.type == BT_INT, sMSG("BoxArray_get did not return BT_INT"));
    log_assert(Box_unwrap_int(retrieved) == 42, sMSG("BoxArray_get returned incorrect value"));

    Box unset = BoxArray_weak_unset(&array, 0);
    log_assert(Box_is_null(unset), sMSG("BoxArray_weak_unset did not set type to BT_NULL"));

    return 0;
}

int BoxDict_test_main() {
    BoxDict dict;
    BoxDict_elements_anew(&dict, 10);
    log_assert(dict.elements != NULL, sMSG("Failed to initialize BoxDict elements"));

    Str key = s("key1");
    Box val = Box_wrap_int(100);
    BoxDict_set(&dict, key, val);
    Box set_result = BoxDict_get(&dict, key);

    log_assert(set_result.type == BT_INT, sMSG("BoxDict_set did not return BT_INT"));
    log_assert(Box_unwrap_int(set_result) == 100, sMSG("BoxDict_set returned incorrect value"));

    BoxKvPair found_entry;
    bool found = BoxDict_find(&dict, key, &found_entry);
    log_assert(found, sMSG("BoxDict_find failed to locate existing key"));
    log_assert(found_entry.value.type == BT_INT, sMSG("BoxDict_find returned incorrect type"));
    log_assert(Box_unwrap_int(found_entry.value) == 100, sMSG("BoxDict_find returned incorrect value"));

    // Test finding a non-existent key
    Str missing_key = s("missing");
    found = BoxDict_find(&dict, missing_key, &found_entry);
    log_assert(!found, sMSG("BoxDict_find incorrectly found a non-existent key"));

    return 0;
}

int DynArray_test_main() {
    return 0;

}
int DynDict_test_main() {
    return 0;
}

int BoxScope_test_main() {
    BoxScope global_scope = BoxScope_empty(s("Global scope"));

    BoxScope_locals_anew(&global_scope, NULL);
    log_assert(global_scope.locals.meta.capacity > 0, sMSG("Failed to create global BoxScope"));

    Str var1 = s("var1");
    Box val1 = Box_wrap_int(500);
    BoxScope_define_local(&global_scope, var1, val1);

    log_assert(Box_unwrap_int(BoxDict_bucket_for_key(&global_scope.locals, var1)->value) == 500, sMSG("BoxScope_define_local failed to set value"));
    
    BoxScope_debug_print(global_scope);

    Box retrieved;
    bool found = BoxScope_lookup(&global_scope, var1, &retrieved);

    log_assert(found, sMSG("BoxScope_lookup failed to find defined variable"));
    log_assert(retrieved.type == BT_INT && Box_unwrap_int(retrieved) == 500, sMSG("BoxScope_lookup returned incorrect value"));

    // Test nested scope
    BoxScope child_scope = BoxScope_empty(sMSG("$Scope"));;
    BoxScope_locals_anew(&child_scope, &global_scope);
    log_assert(child_scope.locals.meta.capacity > 0, sMSG("Failed to create child BoxScope"));

    Str var2 = s("var2");
    Box val2 = Box_wrap_bool(true);
    BoxScope_define_local(&child_scope, var2, val2);

    found = BoxScope_lookup(&child_scope, var1, &retrieved);
    log_assert(found, sMSG("BoxScope_lookup failed to find variable from parent scope"));
    log_assert(retrieved.type == BT_INT && Box_unwrap_int(retrieved) == 500, sMSG("BoxScope_lookup returned incorrect value from parent scope"));

    found = BoxScope_lookup(&child_scope, var2, &retrieved);
    log_assert(found, sMSG("BoxScope_lookup failed to find variable in child scope"));
    // log_assert(Box_is_bool(found) && Box_unwrap_bool(retrieved) == true, sMSG("BoxScope_lookup returned incorrect value from child scope"));

    // Test lookup failure
    Str missing_var = s("missing_var");
    found = BoxScope_lookup(&child_scope, missing_var, &retrieved);
    log_assert(!found, sMSG("BoxScope_lookup incorrectly found a non-existent variable"));

    // Cleanup
    // BoxScope_adel(child_scope);
    // BoxScope_adel(global_scope);
    return 0;
}



Box BoxFn_test_native2(BoxFn *fn, BoxScope parent, Box one, Box two) {
    log_message(LL_INFO, sMSG("Running test_native2!"));

    /// @note print the function name
    log_message(LL_INFO, sMSG("Function name: %.*s"), fmt(fn->name));

    /// @note print the scope 
    BoxScope_debug_print(parent);

    /// @note print the types of the two boxes
    Str a = Box_to_str(one);
    Str b = Box_to_str(two);
    printf("    Box 1: %.*s\n", fmt(a));
    printf("    Box 2: %.*s\n", fmt(b));

    return Box_null();
}

int BoxFn_test_main() {
    // Create a BoxFn representing a user-defined function
    Str fn_name = s("test_fn");
    BoxDict args;
    BoxDict_elements_anew(&args, 2);
    BoxDict_set(&args, s("param1"), Box_null());
    BoxDict_set(&args, s("param2"), Box_null());

    BoxScope enclosure = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&enclosure, NULL);

    BoxFn *fn = BoxFn_anew(FN_NATIVE, fn_name, args, enclosure, (void *) BoxFn_test_native2, NULL);
    log_assert(fn != NULL, sMSG("Failed to create BoxFn"));

    // Create a BoxArray with arguments
    BoxArray args_array;
    BoxArray_elements_anew(&args_array, 2);
    Box arg1 = Box_wrap_int(10);
    Box arg2 = Box_wrap_int(20);
    BoxArray_append(&args_array, arg1);
    BoxArray_append(&args_array, arg2);
    BoxFn_call(fn, enclosure, args_array);

    // Cleanup
    // free(fn);
    // BoxScope_adel(&enclosure);
    return 0;
}


#pragma endregion
//#endregion

//#region Native_BoxHelpersAndErrors
#pragma region BoxHelpersAndErrors

BoxArray *cliargs_as_BoxArray(int argc, char **argv) {
    BoxArray *args = BoxArray_anew(argc);
    for (int i = 0; i < argc; i++) {
        Box arg = Box_wrap_str(&Str_from_cstr(argv[i]));
        BoxArray_append(args, arg);
    }
    return args;
}

BoxError *error_push(BoxError error) {
    __glo_error_stack[__glo_error_stack_size++] = error;
    return &__glo_error_stack[__glo_error_stack_size - 1];
}



void error_print_lastN(size_t n) {
    for(size_t i = 0; i < n; i++) {
        error_print(__glo_error_stack[__glo_error_stack_size - i - 1]);
    }
}

void error_print(BoxError error) {
    log_message(LL_INFO,  s("Error Code: %d"), error.code);
    log_message(LL_INFO,  s("Message: %.*s"), fmt(error.message));
    log_message(LL_INFO,  s("Location: %.*s"), fmt(error.location));
    if (error.info.user_file.cstr != NULL) {
        log_message(LL_INFO,  s("User File: %.*s"), fmt(error.info.user_file));
        log_message(LL_INFO,  s("User Line: %zu"), error.info.user_line);
        log_message(LL_INFO,  s("User Column: %zu"), error.info.user_col);
    }
}

#pragma endregion
//#endregion

//#region Native_StdLibMath
#pragma region NativeStdLibMath

void native_stdlib_math_test_main() {}

typedef enum {
    DIST_UNKNOWN, 
    DIST_USER_DEFINED,
    DIST_NORMAL,
    DIST_GAMMA,
    DIST_BETA,
    DIST_POISSON,
    DIST_BINOMIAL,
    DIST_BERNOULLI,
    DIST_EXPONENTIAL,
    DIST_UNIFORM,
    DIST_BETA_BINOMIAL,
    DIST_DIRICHLET,
    DIST_GAMMA_POISSON,
    DIST_INV_GAMMA,
    DIST_LAPLACE,
    DIST_LOG_NORMAL,
    DIST_NEG_BINOMIAL,
    DIST_PARETO,
    DIST_TRIANGULAR,
    DIST_UNIFORM_INT,
    DIST_WEIBULL,
    DIST_BOLTZMANN,
    DIST_GEV,
    DIST_GUMBEL,
    DIST_RAYLEIGH,
    DIST_WALD,
    DIST_MAXWELL,
    DIST_POWER_LAW,
} DistributionType;

typedef struct {
    DistributionType type;
    Str *param_names;
    Str *param_descriptions;
    size_t num_params;
    double *params;
    (*sample_fn_ptr)(struct Distribution *dist);
    (*pdf_fn_ptr)(struct Distribution *dist, double x);
    (*log_pdf_fn_ptr)(struct Distribution *dist, double x);
} Distribution;

typedef enum {
    INFERENCE_MCMC,
    INFERENCE_VI,
    INFERENCE_MAP
} InferenceMethod;

typedef struct {
    double ** samples;   
    size_t *num_samples; 
} InferenceResult;

/**
 * Structure representing a probabilistic model.
 * This structure should encapsulate the model's parameters, observed data, and any other relevant information.
 */
typedef struct Model {
    Distribution **priors;
    Distribution *likelihood;
    double *params;
    size_t num_params;
    double *data;
    size_t num_data;
    size_t num_target_samples;
} Model;


double sample(const Distribution *dist) {
    return dist->sample_fn_ptr(dist);
}


/**
 * Initializes a probabilistic model with default or specified parameters.
 *
 * @param model   Pointer to the Model structure representing the probabilistic model.
 * @param method  The inference algorithm to use (e.g., MCMC).
 * @return        Pointer to an InferenceResult structure containing posterior samples.
 */
InferenceResult *InferenceResult_anew(const Model *model, InferenceMethod method) {
    InferenceResult *result = anew(sizeof(InferenceResult));
    if (!result) {  error_oom(); return NULL; }

    result->samples = anew(model->num_target_samples * sizeof(double *));
    result->num_samples = anew(model->num_target_samples * sizeof(size_t));

    if (!result->samples) {  error_oom(); return NULL; }
    if (!result->num_samples) {  error_oom(); return NULL; }

    for (size_t i = 0; i < model->num_target_samples; i++) {
        result->samples[i] = anew(model->num_params * sizeof(double));
        if (!result->samples[i]) { 
            error_oom();
        }
    }

    return result;
}


/// @brief Performs inference on the given probabilistic model using the specified inference method.

InferenceResult *InferenceResult_infer(const Model *model, InferenceMethod method);

/// @brief Initializes a probabilistic model with default or specified parameters.
Model *Model_gcnew(Distribution *prior, Distribution *likelihood, size_t num_target_samples);

Model *Model_set_params(Model *model, double *params, size_t num_params) {
    model->params = params;
    model->num_params = num_params;
    return model;
}

Model *Model_set_data(Model *model, double *data, size_t num_data) {
    model->data = data;
    model->num_data = num_data;
    return model;
}


/**
 * Records an observation within the probabilistic model.
 *
 * @param model
 *      includes pointer to a Distribution struct defining the observation's distribution.
 * @param value The observed value to condition on.
 */
void Model_observe(Model *model, double value) {
    model->data[model->num_data++] = value;
}

/**
 * Initializes the random number generator with a given seed.
 *
 * @param seed  The seed value for the RNG.
 */
void initialize_rng(unsigned int seed);

double lib_rand_0to1() {
    return (double) rand() / (double) RAND_MAX;
}

double lib_rand_normal(double mean, double stddev) {
    double u1 = lib_rand_0to1();
    double u2 = lib_rand_0to1();
    double z = sqrt(-2 * log(u1)) * cos(2 * M_PI * u2);
    return mean + z * stddev;
}

/**
 * Computes the Probability Density Function (PDF) of the Normal distribution.
 *
 * @param x       The value at which to evaluate the PDF.
 * @param mean    The mean (μ) of the Normal distribution.
 * @param stddev  The standard deviation (σ) of the Normal distribution.
 * @return        The PDF value at x for N(mean, stddev).
 */
double normal_pdf(double x, double mean, double stddev);
double normal_log_pdf(double x, double mean, double stddev);

/**
 * Computes the Probability Density Function (PDF) of the Gamma distribution.
 *
 * @param x      The value at which to evaluate the PDF.
 * @param shape  The shape parameter (α) of the Gamma distribution.
 * @param rate   The rate parameter (β) of the Gamma distribution.
 * @return       The PDF value at x for Gamma(shape, rate).
 */
double gamma_pdf(double x, double shape, double rate);
double gamma_log_pdf(double x, double shape, double rate);

/**
 * Samples a value from a Normal (Gaussian) distribution.
 *
 * @param mean    The mean (μ) of the distribution.
 * @param stddev  The standard deviation (σ) of the distribution.
 * @return        A random sample from N(mean, stddev).
 */
double sample_normal(double mean, double stddev);

/**
 * Samples a value from a Gamma distribution.
 *
 * @param shape  The shape parameter (α) of the Gamma distribution.
 * @param rate   The rate parameter (β) of the Gamma distribution.
 * @return       A random sample from Gamma(shape, rate).
 */
double sample_gamma(double shape, double rate);


#pragma endregion 
//#endregion Native_StdLibMath

//#region Native_BoxedNative
#pragma region BoxedNative

/// @todo consider the *double problem
///     ... ie maybe have a declare_ for a double arg?
///         ... since it would require a deref 

#define declare_arity0_to_native(raw_name, return_packer) \
    Box native_##raw_name(BoxFn *self, BoxScope parent, Box one, Box two) { \
        native_log_call0(self);\
        return return_packer(raw_name()); \
    }

#define declare_arity1_to_native(raw_name, btype_1st, return_packer) \
    Box native_##raw_name(BoxFn *self, BoxScope parent, Box one) { \
        native_log_call1(self, one);\
        native_return_error_if(one.type != btype_1st, sMSG("Expected argument type %s"), bt_nameof(btype_1st)); \
        Box_ctypeof(btype_1st) first;\
        memcpy(&first, &one, sizeof(Box_ctypeof(btype_1st)));\
        return return_packer(raw_name(first)); \
    }

#define declare_arity2_to_native(raw_name, btype_1st, btype_2nd, return_packer) \
    Box native_##raw_name(BoxFn *self, BoxScope parent, Box one, Box two) { \
        native_log_call2(self, one, two);\
        native_return_error_if(one.type != btype_1st, sMSG("Expected argument type %s"), bt_nameof(btype_1st)); \
        native_return_error_if(two.type != btype_2nd, sMSG("Expected argument type %s"), bt_nameof(btype_2nd)); \
        Box_ctypeof(btype_1st) first;\
        memcpy(&first, &one, sizeof(Box_ctypeof(btype_1st)));\
        Box_ctypeof(btype_1st) second;\
        memcpy(&second, &one, sizeof(Box_ctypeof(btype_1st)));\
        return return_packer(raw_name(first, second));\
    }


#define declare_arity2_to_native_numeric(raw_name, return_packer) \
    Box native_##raw_name(BoxFn *self, BoxScope parent, Box one, Box two) { \
        native_log_call2(self, one, two);\
        double first, second;\
        native_return_error_if(!Box_try_numeric(one, &first), \
            sMSG("Expected numeric argument type, got %s"), bt_nameof(one.type)); \
        native_return_error_if(!Box_try_numeric(two, &second), \
            sMSG("Expected numeric argument type, got %s"), bt_nameof(two.type)); \
        return return_packer(raw_name(first, second));\
    }




declare_arity1_to_native(sqrtf, BT_FLOAT, Box_wrap_float);
declare_arity2_to_native_numeric(lib_rand_normal, Box_wrap_float);

Box native_log(BoxFn *self, BoxScope parent, BoxArray args) {
    native_log_call(self, args);

    for(size_t i = 0; i < len(args); i++) {
        Str_puts(Box_to_str(BoxArray_get(args, i)));
        putchar(' ');
    }

    puts("\n");
    return Box_null();
}

/// @brief 
/// @example 
///     range(3) -> [0, 1, 2, 3]
///     range(1, 3) -> [1, 2, 3]
///     range(0, 4, 2) -> [0, 2, 4]
/// @param self 
/// @param parent 
/// @param args 
/// @return BoxArray as Box
Box native_range(BoxFn *self, BoxScope parent, BoxArray args) {
    BoxArray *result = NULL; 
    double from = 0, to = 0, step = 1; 
    switch(args.meta.size) {
        case 3:
            Box_try_numeric(args.elements[0], &from);
            Box_try_numeric(args.elements[1], &to);
            Box_try_numeric(args.elements[2], &step); 
            break;
        case 2:
            Box_try_numeric(args.elements[0], &from);
            Box_try_numeric(args.elements[1], &to);  
            break;
        case 1:
            Box_try_numeric(args.elements[0], &to); 
            break;
        default:
            native_return_error_if(true, 
                sMSG("Incorrect number of args (%zu): `range(start, end, step)`"), len(args));
    }

    size_t _len = 1 + (size_t) fabs(ceil((to - from)/step));

    native_return_error_if(_len == 0, sMSG("Cannot create an empty range"));
    native_return_error_if(_len > UINT64_MAX, sMSG("Cannot create a range of this size: %zu"), _len);
    native_return_error_if(step == 0, sMSG("Cannot create a range with step 0"));
    native_return_error_if(step < 0 && from < to, sMSG("Cannot create a range with negative step and positive start"));
    native_return_error_if(_len > ARRAY_SIZE_SMALL, sMSG("TODO: Move to DynArray. Range too large: %zu"), _len);

    result = BoxArray_anew(_len);
    for(size_t i = from; i <= to; i++) {
        BoxArray_append(result, Box_wrap_int(i));
    }
    return Box_wrap_array(result);
}

/// @brief 
/// @example 
///     log(infer(model(), #MCMC).take(3))
/// @param self 
/// @param parent 
/// @param loopFn 
/// @param strategyTag 
/// @return 
Box native_infer(BoxFn *self, BoxScope parent, Box loopFn, Box strategyTag) {
    native_log_call2(self, loopFn, strategyTag);

    // native_return_error_if(loopFn.type != BT_BOX_FN, sMSG("Expected function argument type, got %s"), bt_nameof(loopFn.type));
    // native_return_error_if(strategyTag.type != BT_INT, sMSG("Expected int argument type, got %s"), bt_nameof(strategyTag.type));

    if(Box_tag_eq(strategyTag, s("#MCMC"))) {
        printf("MCMC\n");
        return Box_wrap_int(2);
    } else if(Box_tag_eq(strategyTag, s("#HMC"))) {
        printf("HMC\n");
        return Box_wrap_int(1);
    } else {
        native_error(sMSG("Unknown inference strategy: %.*s"), fmt(Box_to_str(strategyTag)));
        return Box_null();
    }
}

/// @brief
/// @example
///     let 
///         m = sample(normal(0, 2))
///     in
///         observe(normal(f(10, m, c), sigma));
///         
Box native_sample(BoxFn *self, BoxScope parent, Box distObject) {
    native_log_call1(self, distObject);

    if(distObject.type == BT_FLOAT) {
        return distObject;
    }

    native_return_error_if(distObject.type != BT_DYN_OBJECT, 
        sMSG("Expected object argument type, got %s"), bt_nameof(distObject.type));

    Box fn = DynObject_get_attribute_checked(
        Box_unwrap_object(distObject), s("sample"), BT_BOX_FN
    );

    native_return_error_if(Box_is_error(fn), 
        sMSG("Expected object to have a `sample` method"));

    return BoxFn_call(Box_unwrap_fn(fn), parent, (BoxArray) {
        .meta.capacity = 1,
        .meta.size = 1,
        .elements = &distObject
    });
}

Box native_take(BoxFn *self, BoxScope parent, BoxArray args) {
    native_log_call(self, args);

    BoxArray *result = BoxArray_anew(ARRAY_SIZE_SMALL);

    // @todo implement take
    BoxArray_append(result, Box_wrap_float(1.1));
    BoxArray_append(result, Box_wrap_float(2.2));
    BoxArray_append(result, Box_wrap_float(3.3));
    BoxArray_append(result, Box_wrap_float(4.4));

    return Box_wrap_array(result);
}


Box native_input(BoxFn *self, BoxScope parent, BoxArray args) {
    native_log_call(self, args);

    require_not_null(self);
    require_not_null(parent.locals.elements);
    require_not_null(args.elements);

    native_return_error_if(args.meta.size != 1, sMSG("Expected 1 argument, got %zu"), len(args));
    native_return_error_if(args.elements[0].type != BT_BOX_STRING, sMSG("Expected string argument type, got %s"), bt_nameof(args.elements[0].type));

    Str prompt = Box_unwrap_str(args.elements[0]);
    Str_puts(prompt);

    return Box_wrap_DynStr(DynStr_from_Str_gcnew(Str_readline_anew(1024)));
}

Box native_len(BoxFn *self, BoxScope parent, Box arg) {
    native_log_call1(self, arg);

    switch(arg.type) {
        case BT_BOX_ARRAY:
            BoxArray *a = Box_unwrap_array(arg);
            return Box_wrap_int(len_ref(a));
        case BT_BOX_STRING:
            Str s = Box_unwrap_str(arg);
            return Box_wrap_int(Str_len(s));
        default:
            native_error(sMSG("Expected array or string argument type, got %s"), bt_nameof(arg.type));
            return Box_null();
    }
}

Box native_sum(BoxFn *self, BoxScope parent, BoxArray args) {
    native_log_call(self, args);

    double sum = 0;
    for(size_t i = 0; i < len(args); i++) {
        native_return_error_if(args.elements[i].type != BT_FLOAT, sMSG("Expected float argument type, got %s"), bt_nameof(args.elements[i].type));
        sum += Box_unwrap_float(args.elements[i]);
    }

    return Box_wrap_float(sum);
}

Box native_sum_DynVecInt(BoxFn *self, BoxScope parent, Box vec) {
    native_log_call1(self, vec);
    DynVecInt *a = Box_unwrap_ptr(DynVecInt, vec);

    native_return_error_if(vec.type != BT_DYN_COLLECTION, sMSG("Expected collection argument type, got %s"), bt_nameof(vec.type));
    native_return_error_if(a == NULL, sMSG("Expected non-null DynVecInt argument"));
    native_return_error_if(a->meta.type != DYNCOL_VEC_INT, sMSG("Expected DynVecInt argument type, got %s"), dyn_ct_nameof(a->meta.type));
    native_return_error_if(len_ref(a) == 0, sMSG("Cannot sum an empty array"));

    int sum = 0;
    for(size_t i = 0; i < len_ref(a); i++) {
        sum += at_ref(a, i);
    }

    return Box_wrap_int(sum);
}

Box native_sum_DynVecDouble(BoxFn *self, BoxScope parent, Box vec) {
    native_log_call1(self, vec);
    DynVecDouble *a = Box_unwrap_ptr(DynVecDouble, vec);

    native_return_error_if(vec.type != BT_DYN_COLLECTION, sMSG("Expected collection argument type, got %s"), bt_nameof(vec.type));
    native_return_error_if(a == NULL, sMSG("Expected non-null DynVecFloat argument"));
    native_return_error_if(a->meta.type != DYNCOL_VEC_DOUBLE, sMSG("Expected DynVecFloat argument type, got %s"), dyn_ct_nameof(a->meta.type));
    native_return_error_if(len_ref(a) == 0, sMSG("Cannot sum an empty array"));

    double sum = 0;
    for(size_t i = 0; i < len_ref(a); i++) {
        sum += at_ref(a, i);
    }

    return Box_wrap_double(sum);
}

Box native_max(BoxFn *self, BoxScope parent, BoxArray args);
Box native_min(BoxFn *self, BoxScope parent, BoxArray args);
Box native_abs(BoxFn *self, BoxScope parent, BoxArray args);
Box native_pow(BoxFn *self, BoxScope parent, BoxArray args);
Box native_sqrt(BoxFn *self, BoxScope parent, BoxArray args);
Box native_floor(BoxFn *self, BoxScope parent, BoxArray args);
Box native_ceil(BoxFn *self, BoxScope parent, BoxArray args);
Box native_round(BoxFn *self, BoxScope parent, BoxArray args);
Box native_log10(BoxFn *self, BoxScope parent, BoxArray args);
Box native_logB(BoxFn *self, BoxScope parent, BoxArray args);
Box native_logE(BoxFn *self, BoxScope parent, BoxArray args);
Box native_exp(BoxFn *self, BoxScope parent, BoxArray args);
Box native_sin(BoxFn *self, BoxScope parent, BoxArray args);
Box native_cos(BoxFn *self, BoxScope parent, BoxArray args);
Box native_tan(BoxFn *self, BoxScope parent, BoxArray args);
Box native_asin(BoxFn *self, BoxScope parent, BoxArray args);
Box native_acos(BoxFn *self, BoxScope parent, BoxArray args);
Box native_atan(BoxFn *self, BoxScope parent, BoxArray args);
Box native_atan2(BoxFn *self, BoxScope parent, BoxArray args);
Box native_sinh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_cosh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_tanh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_asinh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_acosh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_atanh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_rand(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_int(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_range(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_choice(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_shuffle(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_sample(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_seed(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_normal(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_int_normal(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_gamma(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_beta(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_poisson(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_binomial(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_bernoulli(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_exponential(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_uniform(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_beta_binomial(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_dirichlet(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_gamma_poisson(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_inv_gamma(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_laplace(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_log_normal(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_neg_binomial(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_pareto(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_triangular(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_uniform_int(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_weibull(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_boltzmann(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_gev(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_gumbel(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_rayleigh(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_wald(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_maxwell(BoxFn *self, BoxScope parent, BoxArray args);
Box native_random_power_law(BoxFn *self, BoxScope parent, BoxArray args);






void native_add_prelude(BoxScope scope) {
    static BoxFn prelude[] = {
        BoxFnFromNative(FN_NATIVE, s("log"), native_log),
        BoxFnFromNative(FN_NATIVE, s("observe"), native_log),
        BoxFnFromNative(FN_NATIVE, s("range"), native_range),
        BoxFnFromNative(FN_NATIVE_1, s("sqrt"), native_sqrtf),
        BoxFnFromNative(FN_NATIVE_2, s("infer"), native_infer),
        BoxFnFromNative(FN_NATIVE_1, s("sample"), native_sample),
        BoxFnFromNative(FN_NATIVE, s("take"), native_take),
        BoxFnFromNative(FN_NATIVE_2, s("normal"), native_lib_rand_normal),
        BoxFnFromNative(FN_NATIVE_2, s("gamma"), native_lib_rand_normal)
    };

    size_t n = sizeof(prelude) / sizeof(BoxFn);
    for(size_t i = 0; i < n; i++) {
        BoxScope_define_local(&scope, prelude[i].name, Box_wrap_fn(&prelude[i]));
    }
}


#pragma endregion
//#endregion


/// @file interpreter.c

//#region Interpreter_ErrorImpl 
#pragma region InterpErrorImpl 

#define interp_return_if_error(value) \
    if (Box_is_error(value)) {\
        return Box_halt();\
    }

#pragma endregion
//#endregion

//#region Interpreter_BoxedPrecacheImpl
#pragma region BoxedPrecacheImpl

void interp_init() {

    #ifdef INTERP_PRECACHE_INTERNED_STRINGS
        /// @todo consider this interning approach, and this size
        /// ... empirically how many vars / prog? how useful is it to intern?
        /// @todo presumably this should be a vector of strings, not a BoxDict
        BoxDict_elements_anew(&interp_precache().interned_strings, ARRAY_SIZE_MEDIUM);
    #endif
}

/// @todo implement this

/// @brief  
/// @param str 
/// @param out 
/// @return 
bool interp_precache_getstr(Str str, BoxKvPair *out) {
    return true;
}

void interp_precache_setstr(Str str) {
    return;   
}

void interp_precache_test_main() {
    #ifndef INTERP_PRECACHE_INTERNED_STRINGS
        log_message(LL_INFO, sMSG("Skipping interned strings test"));
        return;
    #endif

    // Initialize the interpreter pre-cache
    interp_init();
    log_assert(interp_precache().interned_strings.meta.size == 0, sMSG("Interned strings should be empty"));

    // Test setting and getting a string
    Str test_str = s("test_string");
    interp_precache_setstr(test_str);

    BoxKvPair entry;
    bool found = interp_precache_getstr(test_str, &entry);
    log_assert(found, sMSG("Failed to find interned string"));
    log_assert(Box_is_null(entry.value), sMSG("Interned string value should be null"));

    // Test retrieving a non-existent string
    Str non_existent = s("non_existent");
    found = interp_precache_getstr(non_existent, &entry);
    log_assert(!found, sMSG("Non-existent string should not be found"));
}

#pragma endregion
//#endregion

//#region Interpreter_EvalImpl
#pragma region InterpEvalImpl

#define interp_trace() \
    ctx_trace_me(Str_firstN(Ast_to_str(node), 20));

#define interp_print_traces() Tracer_print_stack(ctx().callstack, 16);
    

 ///@todo
#define interp_require_type(node_type)\
    if (node->type != node_type) {\
        assert(false);\
        return Box_halt();\
    }


static inline __attribute__((always_inline)) 
Box interp_eval_Ast_node(Ast *node, BoxScope *scope) {
    switch(node->type) {
        case AST_DISCARD:
            return Box_null();
        case AST_IF:
            return interp_eval_if(node, scope);
        case AST_DICT:
            return interp_eval_dict(node, scope);
        case AST_VEC:
            return interp_eval_vec(node, scope);
        case AST_FN_DEF_CALL:
            return interp_eval_call_fn(node, scope);
        case AST_METHOD_CALL:
            return interp_eval_method_call(node, scope);
        case AST_MEMBER_ACCESS:
            return interp_eval_member_access(node, scope);
        case AST_TAG: case AST_INT: case AST_FLOAT: case AST_DOUBLE: case AST_STR:
            return interp_eval_literal(node, scope);
        case AST_ID: 
            return interp_eval_identifier(node, scope);
        case AST_MOD:
            return interp_eval_module(node, scope);
        case AST_BLOCK:
            return interp_eval_block(node, scope);
        case AST_FN_DEF:
            return interp_eval_fn_def(node, scope);
        case AST_LOOP:
            return interp_eval_loop(node, scope);
        case AST_FOR:
            return interp_eval_for(node, scope);
        case AST_EXPRESSION:
            return interp_eval_expression(node, scope);
        case AST_FN_DEF_ANON:
            return interp_eval_fn_anon(node, scope);
        case AST_CONST_DEF:
            return interp_eval_const_def(node, scope);
        case AST_LEF_DEF:
            return interp_eval_let_def(node, scope);
        case AST_BOP:
            return interp_eval_binary_op(node, scope);
        case AST_UOP:
            return interp_eval_unary_op(node, scope);
        case AST_USE:
            return interp_eval_use(node, scope);
        case AST_STRUCT_DEF:
            return interp_eval_struct_def(node, scope);
        case AST_OBJECT_LITERAL:
            return interp_eval_object_literal(node, scope);
        case AST_MATCH:
            return interp_eval_match(node, scope);
        case AST_RETURN:
            return interp_eval_return(node, scope);
        case AST_MUTATION:
            return interp_eval_mutation(node, scope);
        default: 
            if((node->type > 0) && (node->type < AST_ENUM_MAX)) {
                interp_error(sMSG("Unknown AST node type (%.*s)"), fmt(Ast_nameof(node->type)));
            } else {
                interp_error(sMSG("Unknown AST node type (%d)"), node->type);
            }
            return Box_halt();
    }
}


Box interp_eval_ast(Ast *node, BoxScope *scope) {
    require_not_null(node); require_not_null(scope); 
    require_positive(node->type);
    Box result = interp_eval_Ast_node(node, scope);

    //|| Box_is_halt(result)
    if(Box_is_error(result)) {
        interp_graceful_exit();
    }

    return result;
}


void interp_graceful_exit() {
    /// @note print the stack trace
    log_message(LL_RECOVERABLE, sMSG("Interpreter Error"));
    interp_print_traces();
    error_print_all();

    /// @todo -- this should not be an abort
    /// ... maybe a longjmp or something else
    abort(); 
    // exit(1);
}



// Implementation of interp_run_from_source
Box interp_run_from_source(ParseContext *p, Str source, int argc, char **argv) {
    lex_source(p, source, s("    "));
    Str_puts(elements_to_ansi_anew(p->lexer));

    #ifdef INTERP_SHOW_PARSING
        lex_print_all(p->lexer);
    #endif 

    parse(p);
    require_not_null(p->parser.elements);
    
    #ifdef INTERP_SHOW_PARSING
        Ast_print(p->parser.elements);
    #endif

    log_assert(p->parser.elements->type == AST_BLOCK, sMSG("Top-level node must be a block"));

    /// @todo revise the BoxScope new'ing to ensure locals are initialized
    BoxScope globals = BoxScope_empty(s("Global scope"));
    BoxScope_locals_anew(&globals, NULL);
    native_add_prelude(globals);

    Box result = interp_eval_ast(p->parser.elements, &globals);
    interp_return_if_error(result);

    /// @note we find and run the main function 
    // BoxArray *main_args = cliargs_as_BoxArray(argc, argv);
    BoxArray main_args = {0}; //cliargs_as_BoxArray(argc, argv);
    Box main_fn;
    if(BoxScope_lookup(&globals, s("main"), &main_fn)) {
        return BoxFn_call(Box_unwrap_fn(main_fn), globals, main_args);
    } else {
        BoxScope_debug_print(globals);
        interp_error(sMSG("No main function found"));
        return Box_halt();
    }
}

Box BoxFn_interp_user_fn(BoxFn *fn, BoxScope function_scope, BoxArray args) {
    require_not_null(fn);
    log_assert(fn->type == FN_USER, sMSG("Function must be user-defined!"));

    Ast *body = (Ast *) fn->code;
    require_not_null(body);
    
    size_t i = 0;
    BoxDict_iter_items(fn->signature, arg) {
        Box value = at(args, i++);
        if(Box_is_null(value)) {
            value = arg->value;
        }
        BoxScope_define_local(&function_scope, arg->key, value);
    }

    return interp_eval_ast(body, &function_scope);
}



/// @brief
/// interp_eval_call_generator:
///     Creates a new BoxIterator to keep track of the generator's state.
///     Initializes the iterator with the original function and other necessary information.
///     Creates a new generator function that wraps the original function and the iterator.
///     Appends the iterator to the arguments.
///     Returns the new generator function.
/// BoxFn_interp_generator_fn:
///     Extracts the iterator from the last argument.
///     Calls the original function.
///     Checks the result:
///     If it's a yield, updates the iterator state and returns the yielded value.
///     If it's done, returns a "done" signal.
///     If it's unexpected, returns an error.
/// @example
///      loop fn example_generator() :=
///         for(i <- range(3)) yield i 
/// @param fn 
/// @param function_scope 
/// @param args 
/// @return 
Box interp_eval_call_generator(BoxFn *fn, BoxScope function_scope, BoxArray args) {
    require_not_null(fn);
    require_not_null(fn->fnptr);
    require(fn->type == FN_LOOP, sMSG("Function must be a generator!"));

    BoxIterator *iter = BoxIterator_anew(fn, args);
    BoxFn *gen_fn = BoxFn_anew(FN_GENERATOR, fn->name, fn->signature, function_scope, 
        (void *) BoxFn_interp_generator_fn, (void *) iter
    );

    BoxArray_append(&args, Box_wrap_BoxIterator(iter));
    return Box_wrap_fn(gen_fn);
}

Box BoxFn_interp_generator_fn(BoxFn *fn, BoxScope function_scope, BoxArray args) {
    require_not_null(fn);
    require_not_null(fn->fnptr);
    require(fn->type == FN_GENERATOR, sMSG("Function must be a generator!"));

    BoxIterator *iter = Box_unwrap_BoxIterator(last(args));
    Box result = BoxFn_call(fn, function_scope, args);

    if(Box_is_state_end(result)) {
        return Box_null();
    } else if(Box_is_state_yield(result)) {
        BoxIterator_update(iter, result);
        return BoxIterator_get(iter);
    } else {
        interp_error(sMSG("Unexpected generator state"));
        return Box_halt();
    }
}

Box interp_eval_if(Ast *node, BoxScope *scope) {
    require_not_null(node); require_not_null(scope); interp_trace();
    interp_require_type(AST_IF);

    Ast *condition = node->if_stmt.condition;
    Box cond_val = interp_eval_ast(condition, scope);
    
    // Assuming Box has a method to evaluate its truthiness
    bool cond_truth = Box_is_truthy(cond_val);
    
    if (cond_truth) {
        return interp_eval_ast(node->if_stmt.body, scope);
    } else if (node->if_stmt.else_body) {
        return interp_eval_ast(node->if_stmt.else_body, scope);
    } else {
        return Box_null();
    }
}

Box interp_eval_dict(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_DICT);

    DynDict *dict = DynDict_gcnew(node->dict.size);
    for (size_t i = 0; i < node->dict.size; i++) {
        struct AstDictEntry *entry = &node->dict.elements[i];

        Box key = interp_eval_ast(entry->key, scope);
        Box val = interp_eval_ast(entry->value, scope);
        DynDict_set(dict, key, val);
    }

    return Box_wrap_dict(dict);
}


Box interp_eval_block(Ast *node, BoxScope *scope) {

    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_BLOCK);

    BoxScope *block_scope;
    BoxScope block_local = BoxScope_empty(sMSG("$Scope"));
    
    /// @note if the block is transparent, we use the parent scope
    /// ... otherwise we create a new scope with the parent 
    /// ... transparent blocks are used for the global program scope
    /// ... (and maybe other things, eg., modules, some loops, etc.)
    /// ... non-transparent blocks are used for functions and other scopes

    if(node->block.transparent) {
        block_scope = scope;
    } else {
        BoxScope_locals_anew(&block_local, scope);
        block_scope = &block_local;
    }
    

    Box result = Box_null();
    Box lAst_result = Box_null();

    for (size_t i = 0; i < node->block.num_statements; i++) {
        Ast *stmt = node->block.statements[i];
        result = interp_eval_ast(stmt, block_scope);
        interp_log_debug(sMSG("Block statement result: %.*s"), fmt(Box_to_str(result)));

        if(Box_is_state_end(result)) {
            return lAst_result;
        }
        lAst_result = result;
    }

    return result;
}


Box interp_eval_fn_def(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_FN_DEF);

    BoxDict args;
    BoxDict_elements_anew(&args, ARRAY_SIZE_SMALL);

    for (size_t i = 0; i < node->fn.num_params; i++) {
        Box value = (node->fn.params->default_value != NULL) ?
            interp_eval_ast(node->fn.params->default_value, scope) :
            Box_null();

        BoxDict_set(&args, node->fn.params[i].name, value);
    }

    BoxFnType fnt = node->fn.is_generator ? FN_GENERATOR : FN_USER;

    BoxFn *fn = BoxFn_anew(fnt, node->fn.name, args, *scope, 
        (void *) BoxFn_interp_user_fn, (void*) node->fn.body
    );

    BoxScope_define_local(scope, node->fn.name, Box_wrap_fn(fn));
    require_scope_has(scope, node->fn.name);

    interp_log_debug(sMSG("Defined function '%.*s'"), fmt(node->fn.name));

    /// @todo is this correct?
    return Box_wrap_fn(fn);
}


Box interp_eval_fn_anon(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_FN_DEF_ANON);

    BoxDict args;
    BoxDict_elements_anew(&args, ARRAY_SIZE_SMALL);

    for (size_t i = 0; i < node->fn_anon.num_params; i++) {
        Box value = (node->fn_anon.params->default_value != NULL) ?
            interp_eval_ast(node->fn_anon.params->default_value, scope) :
            Box_null();

        BoxDict_set(&args, node->fn_anon.params[i].name, value);
    }

    BoxFn *fn = BoxFn_anew(FN_USER, s("$anon"), args, *scope, (void *) BoxFn_interp_user_fn, (void*) node->fn_anon.body);
    return Box_wrap_fn(fn);
}


Box interp_eval_member_access(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_MEMBER_ACCESS);

    Box object = interp_eval_ast(node->member_access.target, scope);
    interp_return_if_error(object);

    if (object.type != BT_DYN_OBJECT) {
        interp_error(sMSG("Attempted to access member on non-object type: %s"), bt_nameof(object.type));
        return Box_halt();
    }

    Str member_name = node->member_access.property;

    Box member = DynObject_get_attribute(Box_unwrap_object(object), member_name);
    return member;
}

Box interp_eval_method_call(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_METHOD_CALL);

    Box object = interp_eval_ast(node->method_call.target, scope);
    // Box_print(object);

    interp_return_if_error(object);

    Str method_name = node->method_call.method;
    Box method = DynObject_resolve_method(object, scope, method_name);
    interp_return_if_error(method);

    size_t total_args = node->method_call.num_args + 1;
    BoxArray *args = BoxArray_anew(total_args);

    BoxArray_append(args, object);
    for (size_t i = 0; i < node->method_call.num_args; i++) {
        Box arg = interp_eval_ast(node->method_call.args[i], scope);
        interp_return_if_error(arg);
        BoxArray_append(args, arg);
    }
    
    return BoxFn_call(Box_unwrap_fn(method), Box_unwrap_fn(method)->enclosure, *args);
}




Box interp_eval_module(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_MOD);
    Box_return_noimpl();
}

Box interp_eval_literal(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();

    switch(node->type) {
        case AST_INT: 
            /// @todo this will need atoi or similar
            return Box_wrap_int(node->integer.value);
        case AST_FLOAT:
        case AST_DOUBLE: // @todo proper double handling -- AST_FLOAT, TT_FLOAT aren't impl
            return Box_wrap_float((float) node->dble.value);
        case AST_STR:
            return Box_wrap_str(&node->str.value);
        case AST_TAG:
            return Box_wrap_tag(node->tag.name);
        default:
            interp_error(sMSG("interp_eval_literal called with invalid node type: %d"), node->type);
            return Box_halt();
    }
}


/// @brief `loop` repeats a block of code without a result 
///         `loop if` is equivalent to `while`
///         `loop` without a condition iterates over streams
///             The `for` keyword yields a value. See `for` for more information.
/// @param node 
/// @param scope 
/// @return
/// @example
///     loop if x < 10 in
///         x = x + 1
///         log(x)
///
///     Short form:
///         loop(x <- range(1, 10), y <- range(3), z = x*y) log(z)
///
///     loop 
///         x <- range(1, 10)
///         y <- range(3)
///         z = x*y
///     in
///         log(z)
Box interp_eval_loop(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_LOOP);

    BoxScope loop_scope = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&loop_scope, scope);

    if(node->loop.condition) {
        return interp_eval_loop_with_condition(node, &loop_scope);
    } else if(node->loop.bindings) {
        return interp_eval_loop_with_streams(node, &loop_scope);
    } else {
        return interp_eval_loop_with_infinite(node, &loop_scope);
    }
}

Box interp_eval_loop_with_condition(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_LOOP);

    Ast *condition = node->loop.condition;
    Ast *body = node->loop.body;

    Box result = Box_null();
    while(true) {
        Box cond_val = interp_eval_ast(condition, scope);
        interp_return_if_error(cond_val);

        if(!Box_is_truthy(cond_val)) {
            break;
        }

        result = interp_eval_ast(body, scope);
        if(Box_is_state_end(result)) {
            return result;
        }
    }

    return result;
}
Box interp_eval_loop_with_streams(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_LOOP);

    Ast **bindings = node->loop.bindings;
    size_t num_bindings = node->loop.num_bindings;
    Ast *body = node->loop.body;

    // Create a new iterator scope
    BoxScope iter_scope = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&iter_scope, scope);

    // Iterate over bindings
    for(size_t i = 0; i < num_bindings; i++) {
        Ast *binding = bindings[i];
        // Assuming binding is an AST_BINDING node
        if(binding->type != AST_BINDING) {
            interp_error(sMSG("Expected binding in loop statement."));
            return Box_halt();
        }

        Str var_name = binding->binding.identifier;
        Box expr_val = interp_eval_ast(binding->binding.expression, scope);
        interp_return_if_error(expr_val);
        BoxScope_define_local(&iter_scope, var_name, expr_val);
    }

    // Execute the loop body
    Box result = interp_eval_ast(body, &iter_scope);

    return result;
    // Cleanup
}

Box interp_eval_loop_with_infinite(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_LOOP);

    Ast *body = node->loop.body;

    Box result = Box_null();
    while(true) {
        result = interp_eval_ast(body, scope);
        if(Box_is_state_end(result)) {
            return result;
        }
        
    }

    return result;
}



/// @brief 
/// @param node 
/// @param scope 
/// @example
///    for(x <- range(1, 10)) x
///    for(x <- range(1, 10)) yield x
///    for(x <- range(1, 10), y <- range(3)) x * y
///    for(x <- range(1, 10), y <- range(3), z = x*y) z
///    for 
///         x <- range(1, 10)
///         y <- range(3)
///         z = x*y
///     in 
///         z
///    for 
///         x <- range(1, 10)
///         y <- range(3)
///         z = x*y
///     in yield z
/// @return 

Box interp_eval_for(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_FOR);

    // Extract bindings and body
    Ast **bindings = node->loop.bindings;
    size_t num_bindings = node->loop.num_bindings;
    Ast *body = node->loop.body;

    // Create a new iterator scope
    BoxScope iter_scope = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&iter_scope, scope);

    // Iterate over bindings
    for(size_t i = 0; i < num_bindings; i++) {
        Ast *binding = bindings[i];
        // Assuming binding is an AST_BINDING node
        if(binding->type != AST_BINDING) {
            interp_error(sMSG("Expected binding in for-loop."));
            return Box_halt();
        }

        Str var_name = binding->binding.identifier;
        Box expr_val = interp_eval_ast(binding->binding.expression, scope);
        interp_return_if_error(expr_val);
        BoxScope_define_local(&iter_scope, var_name, expr_val);
    }

    // Execute the loop body
    Box result = interp_eval_ast(body, &iter_scope);

    // Cleanup
    // TODO: Implement scope cleanup if necessary

    return result;
}


Box interp_eval_expression(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_EXPRESSION);

    return interp_eval_ast(node->exp_stmt.expression, scope);
}

Box interp_eval_const_def(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_CONST_DEF);

    Str var_name = node->const_stmt.name;
    Ast *expr = node->const_stmt.value;

    Box value = interp_eval_ast(expr, scope);

    interp_return_if_error(value);

    BoxScope_define_local(scope, var_name, value);
    
    interp_log_debug(sMSG("Defined constant '%.*s' with value '%.*s'"), fmt(var_name), fmt(Box_to_str(value)));

    return Box_done();
}


Box interp_eval_vec(Ast *node, BoxScope *scope) {
    require_not_null(node); require_not_null(scope); interp_trace();
    interp_require_type(AST_VEC);

    Ast **elements = node->vec.elements;
    size_t size = node->vec.size;

    DynArray *array = DynArray_gcnew(size);
    for(size_t i = 0; i < size; i++) {
        Box element = interp_eval_ast(elements[i], scope);
        interp_return_if_error(element);
        DynArray_append(array, element);
    }

    return Box_wrap_DynCollection(array);
}


Box interp_eval_let_def(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_LEF_DEF);

    Ast **bindings = node->let_stmt.bindings;
    size_t num_bindings = node->let_stmt.num_bindings;
    Ast *body = node->let_stmt.body;

    // Evaluate and define each binding
    for(size_t i = 0; i < num_bindings; i++) {
        Ast *binding = bindings[i];
        if(binding->type != AST_BINDING) {
            interp_error(sMSG("Expected binding in let statement."));
            return Box_halt();
        }

        Str var_name = binding->binding.identifier;
        Box expr_val = interp_eval_ast(binding->binding.expression, scope);
        interp_return_if_error(expr_val);
        BoxScope_define_local(scope, var_name, expr_val);
    }

    // Evaluate the body
    return interp_eval_ast(body, scope);
}

Box interp_eval_binary_op(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_BOP);

    Box left = interp_eval_ast(node->bop.left, scope);
    interp_return_if_error(left);
    Box right = interp_eval_ast(node->bop.right, scope);
    interp_return_if_error(right);

    return interp_eval_bop(node->bop.op, left, right);
}

Box interp_eval_unary_op(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_UOP);

    Box operand = interp_eval_ast(node->uop.operand, scope);
    interp_return_if_error(operand);

    return interp_eval_uop(node->uop.op, operand);
}

Box interp_eval_match(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_MATCH);

    Box expr_val = interp_eval_ast(node->match.expression, scope);
    interp_return_if_error(expr_val);

    for(size_t i = 0; i < node->match.num_cases; i++) {
        Ast *case_node = node->match.cases[i].condition;

        if(case_node) {
            Box test_val = interp_eval_ast(case_node, scope);
            interp_return_if_error(test_val);

            if(Box_eq(expr_val, test_val)) {
                return interp_eval_ast(node->match.cases[i].expression, scope);
            }
        } else {
            /// @todo -- the else branch
            return Box_null();
        }
    }

    interp_error(sMSG("No matching case found in match statement."));
    return Box_halt();
}


/// @brief handles the `return` keyword which can be placed anywhere in a block
///             to short-circuit the block and return a value
/// @example 
///    let(x = 10) in return x
///    let
///        x = 10  
///        y = 20
///    in
///        return x + y
///        x * y // allowed but unreachable
/// @param node 
/// @param scope 
/// @return 
Box interp_eval_return(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_RETURN);

    Box return_val = interp_eval_ast(node->return_stmt.value, scope);
    interp_return_if_error(return_val);


    /// @todo check if this implementation is correct
    return return_val;
}

Box interp_eval_mutation(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_MUTATION);

    // Evaluate target and value
    Box target = interp_eval_ast(node->mutation.target, scope);
    interp_return_if_error(target);
    Box value = interp_eval_ast(node->mutation.value, scope);
    interp_return_if_error(value);

    /// @todo perform the mutation
    ///     nb. we have broadcast/vector'd mutations and ordinary updates

    // if(target.type != BT_BOX_PTR) {
    //     interp_error(sMSG("Mutation target is not a mutable reference."));
    //     return Box_halt();
    // }

    Box *target_ptr = Box_unwrap_ptr(Box, target);
    *target_ptr = value;

    return value;
}



Box interp_eval_identifier(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_ID);

    Box out; 
    if(BoxScope_lookup(scope, node->id.name, &out)) {
        return out;
    } else {
        interp_error(sMSG("Undefined identifier: %.*s"), fmt(node->id.name));
        return Box_halt();
    }
} 


Box interp_eval_call_fn(Ast *node, BoxScope *scope) {
    require_not_null(node); require_not_null(scope); interp_trace();
    interp_require_type(AST_FN_DEF_CALL);

    Box callee_fn = interp_eval_ast(node->call.callee, scope);
    interp_return_if_error(callee_fn);

    if(callee_fn.type != BT_BOX_FN) {
        interp_error(sMSG("Not a function, type is: %.*s"), fmt(bt_nameof(callee_fn.type)));
        return Box_halt();  
    }

    BoxFn *fn = Box_unwrap_fn(callee_fn);
    
    BoxArray *args = BoxArray_anew(node->call.num_args);
    for (size_t i = 0; i < node->call.num_args; i++) {
        Box arg = interp_eval_ast(node->call.args[i], scope);
        BoxArray_append(args, arg);
    }

    if(len_ref(args) < fn->signature.meta.size) {
        interp_error(sMSG("Function `%.*s` expects at least %zu arguments, but %zu were provided."),
                    fmt(fn->name), fn->signature.meta.size, len_ref(args));
        // BoxArray_adel(args);
        return Box_halt();
    }

    BoxScope fn_scope = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&fn_scope, scope);

    return BoxFn_call(fn, fn_scope, *args);
}



Box interp_eval_use(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_USE);

    Box _module = interp_eval_load_module(node);

    if(Box_is_error(_module)) {
        interp_error(sMSG("Failed to load module: %.*s"), node->use_stmt.module_path);
        return Box_halt();
    }
    
    DynObject *module = Box_unwrap_object(_module);
    BoxScope_merge_local(scope, &module->data->namespace);

    return Box_null();
}

Box interp_eval_load_module(Ast *node) {
    /// @todo implement module loading
    return Box_halt();
}

Box interp_eval_struct_def(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_STRUCT_DEF);

    BoxStruct *struct_def = BoxStruct_anew(node->struct_def.name, node->struct_def.num_fields);
    for(size_t i = 0; i < node->struct_def.num_fields; i++) {
        BoxDict_set(&struct_def->fields, node->struct_def.fields[i].name, Box_null());
    }

    BoxScope_define_local(scope, node->struct_def.name, Box_wrap_struct(struct_def));

    return Box_done();
}


/// @brief 
/// @param node 
/// @param scope 
/// @return
/// @example
///     const eg = Point { .x = 10, .y = 20 }
///
Box interp_eval_object_literal(Ast *node, BoxScope *scope) {
    require_not_null(node);
    require_not_null(scope);
    interp_trace();
    interp_require_type(AST_OBJECT_LITERAL);

    Box struct_obj;
    
    if(!BoxScope_lookup(scope, node->object_literal.struct_name, &struct_obj)) {
        interp_error(sMSG("Undefined struct: %.*s"), node->object_literal.struct_name);
        return Box_halt();
    }
    
    

    BoxStruct *struct_def = Box_unwrap_struct(struct_obj);
    DynObject *object = DynObject_gcnew(struct_def);
    BoxDict *fields = &object->data->namespace.locals;
    
    for(size_t i = 0; i < node->object_literal.num_fields; i++) {
        Str field_name = node->object_literal.fields[i].name;
        Ast *field_value = node->object_literal.fields[i].value;

        Box value = interp_eval_ast(field_value, scope);
        interp_return_if_error(value);

        BoxDict_set(fields, field_name, value);
    }

    return Box_wrap_object(object);
}

int interp_eval_test_main() {
    // Create a simple AST representing the expression: 1 + 2
    Ast left = { .type = AST_INT, .integer.value = 1 };
    Ast right = { .type = AST_INT, .integer.value = 2 };
    Ast bop = { .type = AST_BOP, .bop.left = &left, .bop.right = &right, .bop.op = s("+") };

    // Initialize global scope
    BoxScope global_scope = BoxScope_empty(sMSG("$Scope"));
    BoxScope_locals_anew(&global_scope, NULL);
    log_assert(global_scope.locals.meta.capacity > 0, sMSG("Failed to create global scope"));

    // Evaluate the AST
    Box result = interp_eval_ast(&bop, &global_scope);

    // Assert the result is correct
    log_assert(result.type == BT_INT, sMSG("interp_eval_ast did not return BT_INT"));
    log_assert(Box_unwrap_int(result) == 3, sMSG("interp_eval_ast did not compute 1 + 2 correctly"));

    // Cleanup
    // BoxScope_adel(global_scope);

    return 0;
}


#pragma endregion
//#endregion

//#region Interpreter_EvaluatorImpl
#pragma region InterpreterEvaluatorImpl

// Evaluate a function call
Box interp_eval_fn(Box fn_obj, int num_args, Box *args) {
    if(fn_obj.type != BT_BOX_FN) {
        interp_error(sMSG("Attempted to call a non-function."));
        return Box_halt();
    }

    BoxFn *fn = Box_unwrap_fn(fn_obj);
    BoxArray *args_array = BoxArray_anew(num_args);
    for(int i = 0; i < num_args; i++) {
        args_array->elements[i] = args[i];
    }

    return BoxFn_call(fn, fn->enclosure, *args_array);
}

// Evaluate a unary operation
Box interp_eval_uop(Str op, Box operand) {
    if(Str_eq_chr(op, '-')) {
        if(operand.type == BT_INT) {
            return Box_wrap_int(-Box_unwrap_int(operand));
        } else if(operand.type == BT_FLOAT) {
            float val = Box_unwrap_float(operand);
            return Box_wrap_float(-val);
        } else {
            interp_error(sMSG("Unsupported operand type for unary '-'."));
            return Box_halt();
        }
    }
    // Add more unary operators as needed
    interp_error(sMSG("Unsupported unary operator: %s"), op);
    return Box_halt();
}

// Evaluate a binary operation
/// @todo make the type jugglery more robust
Box interp_eval_bop(Str op, Box left, Box right) {
    if(Str_eq_chr(op, '+')) {
        if(left.type == BT_INT && right.type == BT_INT) {
            return Box_wrap_int(Box_unwrap_int(left) + Box_unwrap_int(right));
        } 
        
        else if(left.type == BT_FLOAT && right.type == BT_FLOAT) {
            return Box_wrap_float(Box_unwrap_float(left) + Box_unwrap_float(right));
        } 
        /// @note if either is a float, we'll cast both to float
        else if(left.type == BT_INT && right.type == BT_FLOAT) {
            return Box_wrap_float((float) Box_unwrap_int(left) + Box_unwrap_float(right));
        } 
        
        else if(left.type == BT_FLOAT && right.type == BT_INT) {
            return Box_wrap_float(Box_unwrap_float(left) + (float) Box_unwrap_int(right));
        }

        else if(left.type == BT_BOX_STRING && right.type == BT_BOX_STRING) {
            // Concatenate strings
            /// @todo: this can use a fix string since we know the sizes of both operands
        } 
        
        else {
            interp_error(sMSG("Unsupported operand types for '%c': %.*s, %.*s"), 
                Str_chr_at(op, 0), fmt(bt_nameof(left.type)), fmt(bt_nameof(right.type)));
            return Box_halt();
        }
    }
    // Implement other binary operators (e.g., -, *, /, etc.)
    if(Str_eq_chr(op, '-')) {
        if(left.type == BT_INT && right.type == BT_INT) {
            return Box_wrap_int(Box_unwrap_int(left) - Box_unwrap_int(right));
        } else if(left.type == BT_FLOAT && right.type == BT_FLOAT) {
            return Box_wrap_float(Box_unwrap_float(left) - Box_unwrap_float(right));
        } else {
            interp_error(sMSG("Unsupported operand types for '%c': %.*s, %.*s"), 
                Str_chr_at(op, 0), fmt(bt_nameof(left.type)), fmt(bt_nameof(right.type)));
            return Box_halt();
        }
    }
    if(Str_eq_chr(op, '*')) {
        if(left.type == BT_INT && right.type == BT_INT) {
            return Box_wrap_int(Box_unwrap_int(left) * Box_unwrap_int(right));
        } else if(left.type == BT_FLOAT && right.type == BT_FLOAT) {
            return Box_wrap_float(Box_unwrap_float(left) * Box_unwrap_float(right));
        } else {
            interp_error(sMSG("Unsupported operand types for '%c': %.*s, %.*s"), 
                Str_chr_at(op, 0), fmt(bt_nameof(left.type)), fmt(bt_nameof(right.type)));
            return Box_halt();
        }
    }
    if(Str_eq_chr(op, '/')) {
        if(left.type == BT_INT && right.type == BT_INT) {
            if(Box_unwrap_int(right) == 0) {
                interp_error(sMSG("Division by zero."));
                return Box_halt();
            }
            return Box_wrap_int(Box_unwrap_int(left) / Box_unwrap_int(right));
        } else if(left.type == BT_FLOAT && right.type == BT_FLOAT) {
            if(Box_unwrap_float(right) == 0.0f) {
                interp_error(sMSG("Division by zero."));
                return Box_halt();
            }
            return Box_wrap_float(Box_unwrap_float(left) / Box_unwrap_float(right));
        } else {
            interp_error(sMSG("Unsupported operand types for '%c': %.*s, %.*s"), 
                Str_chr_at(op, 0), fmt(bt_nameof(left.type)), fmt(bt_nameof(right.type)));
            return Box_halt();
        }
    }
    // Add more operators as needed

    interp_error(sMSG("Unsupported binary operator: %s"), op);
    return Box_halt();
}


int interp_error_test_main() {

    return 0;
}
#pragma endregion
//#endregion

//#region Interpreter_TestMain
#pragma region InterpreterTestMain

int interpreter_member_access_test() {
    // Define a struct Point
    BoxScope global_scope = BoxScope_empty(s("Global scope"));
    BoxScope_locals_anew(&global_scope, NULL);

    // Define struct Point
    Ast point_struct = {
        .type = AST_STRUCT_DEF,
        .struct_def = {
            .name = s("Point"),
            .num_fields = 2,
            .fields = (struct StructField[]) {
                { .name = s("x"), .type = NULL, .default_value = NULL },
                { .name = s("y"), .type = NULL, .default_value = NULL }
            }
        }
    };
    Box result = interp_eval_struct_def(&point_struct, &global_scope);
    log_assert(result.type == BT_STATE && result.payload == BS_DONE, sMSG("Struct def did not return box_done"));

    // Create a Point instance
    Ast point_instance = {
        .type = AST_OBJECT_LITERAL,
        .object_literal = {
            .struct_name = s("Point"),
            .num_fields = 2,
            .fields = (struct AstObjectField[]) {
                { .name = s("x"), .value = &(Ast){ .type = AST_INT, .integer.value = 10 } },
                { .name = s("y"), .value = &(Ast){ .type = AST_INT, .integer.value = 20 } }
            }
        }
    };
    Box p = interp_eval_object_literal(&point_instance, &global_scope);
    BoxScope_define_local(&global_scope, s("p"), p);

    // Access p.x
    Ast access_x = {
        .type = AST_MEMBER_ACCESS,
        .member_access = {
            .target = &(Ast){ .type = AST_ID, .id = { .name = s("p") } },
            .property = s("x")
        }
    };
    Box x_val = interp_eval_ast(&access_x, &global_scope);
    log_assert(x_val.type == BT_INT && Box_unwrap_int(x_val) == 10, sMSG("Member access p.x failed"));

    // Access p.y
    Ast access_y = {
        .type = AST_MEMBER_ACCESS,
        .member_access = {
            .target = &(Ast){ .type = AST_ID, .id = { .name = s("p") } },
            .property = s("y")
        }
    };
    Box y_val = interp_eval_ast(&access_y, &global_scope);
    log_assert(y_val.type == BT_INT && Box_unwrap_int(y_val) == 20, sMSG("Member access p.y failed"));

    return 0;
}

int interpreter_method_call_test() {
    return 0;
}

int interpreter_scope_tests() {
    printf("Running interpreter tests...\n");
    // Test global constant def
    {
        BoxScope global_scope = BoxScope_empty(sMSG("Global scope"));
        BoxScope_locals_anew(&global_scope, NULL);

        Ast const_node = {
            .type = AST_CONST_DEF,
            .const_stmt = {
                .name = s("test"),
                .value = &(Ast){ .type = AST_INT, .integer.value = 1234 }
            }
        };

        Box result = interp_eval_const_def(&const_node, &global_scope);
        log_assert(result.type == BT_STATE && result.payload == BS_DONE, sMSG("Const def did not return box_done"));

        Box retrieved;
        bool found = BoxScope_lookup(&global_scope, s("test"), &retrieved);
        log_assert(found, sMSG("Global constant 'test' was not found"));
        log_assert(retrieved.type == BT_INT && Box_unwrap_int(retrieved) == 1234, sMSG("Global constant 'test' has incorrect value"));

        log_message(LL_INFO, sMSG("Global constant 'test' has been defined"));
        log_message(LL_INFO, sMSG("Global constant 'test' has value %lld\n"), Box_unwrap_int(retrieved));
    }

    // Test global function def
    {
        BoxScope global_scope = BoxScope_empty(sMSG("Global scope"));
        BoxScope_locals_anew(&global_scope, NULL);

        Ast fn_node = {
            .type = AST_FN_DEF,
            .fn = {
                .name = s("main"),
                .num_params = 0,
                .params = NULL,
                .body = &(Ast){ 
                    .type = AST_FN_DEF_CALL,
                    .call = {
                        .callee = &(Ast){ 
                            .type = AST_ID, 
                            .id = { .name = s("log") } 
                        },
                        .num_args = 1,
                        .args = anew(sizeof(Ast*) * 1)
                    }
                }
            }
        };

        // fn_node.fn.params.args[0] = &(Ast){ 
        //     .type = AST_ID, 
        //     .id = { .name = s("test") } 
        // };

        Box result = interp_eval_fn_def(&fn_node, &global_scope);
        log_assert(result.type == BT_BOX_FN, sMSG("Function def did not return BT_BOX_FN"));

        Box retrieved;
        bool found = BoxScope_lookup(&global_scope, s("main"), &retrieved);
        log_assert(found, sMSG("Global function 'main' was not found"));
        log_assert(retrieved.type == BT_BOX_FN, sMSG("Global function 'main' has incorrect type"));

        log_message(LL_INFO, sMSG("Global function 'main' has been defined\n"));
    }

    return 0;
}


void interpreter_test_main() {
    Box_test_main();

    // Run tests for InterpErrorImpl
    interp_error_test_main();

    // Run tests for BoxedPrecacheImpl
    interp_precache_test_main();

    // Run tests for BoxedDataTypesImpl
    BoxArray_test_main();
    BoxDict_test_main();
    DynArray_test_main();
    DynDict_test_main();
    BoxScope_test_main();
    BoxFn_test_main();

    interpreter_scope_tests();
    interpreter_member_access_test();
    interpreter_method_call_test();
    // Run tests for InterpEvalImpl
    interp_eval_test_main();
}

#pragma endregion
//#endregion

//#region Interpreter_Setup 
#pragma region InterpreterSetup 


#define code_example(n)\
    (Str) {.cstr = __glo_example##n, .size = sizeof(__glo_example##n) - 1}


void interpreter_examples_main(int argc, char **argv) {
    interp_run_from_source(pctx(), code_example(1), argc, argv);
}


void interpreter_main(int argc, char **argv) {
    enum {CLI_POSITIONAL = 0, CLI_SOURCE, CLI_HELP, CLI_INDENT, CLI_ENUM_MAX};

    CliOption options[] = {
        [CLI_POSITIONAL] = cli_opt_default(s("--source"), s("main.doubt")),
        [CLI_HELP]   = cli_opt_flag(s("--help")),
        [CLI_INDENT] = cli_opt_default(s("--indent"), s("    ")),
    };

    cli_print_opts(options, CLI_ENUM_MAX);
    /// @todo file reading
    // Str source = Str_read_file_anew(cli_opt_get(options, CLI_POSITIONAL).cstr);
    //

    interp_run_from_source(pctx(), code_example(1), argc, argv);
}


void setup_memory() {
    Arena_init(mem().arena_static, 32 * ARENA_1MB);
    Arena_init(mem().ar_interp_local, 2 * ARENA_1MB);
    Arena_init(mem().ar_interp_global, 2 * ARENA_1MB);

    ctx_setup(&ctx(), mem().Heap_interp_gc, mem().arena_static);
    parser_ctx_anew(pctx(), mem().arena_static, mem().arena_static);
}


#pragma endregion
//#endregion

//#region Interpreter_Main
#pragma region InterpreterMain

int main(int argc, char **argv) {
    setup_memory();


    #if defined(EXAMPLES) 
        interpreter_examples_main(argc, argv);
    #endif
    
    #if defined(TEST)
        // parsing_test_main();
        // internal_test_main();
        // interpreter_test_main();
    #else 
        interpreter_main(argc, argv);
    #endif

    cfree_log_leaks();  

    return 0;
}

#pragma endregion
//#endregion

/// @note end of file
