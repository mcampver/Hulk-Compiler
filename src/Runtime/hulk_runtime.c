#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <assert.h>

// Forward declarations
char* hulk_string_concat(const char* a, const char* b);
char* hulk_string_triple_concat(const char* a, const char* b, const char* c);
char* hulk_string_repeat(const char* str, int times);
int hulk_string_equal(const char* a, const char* b);
double hulk_integer_div(double a, double b);
double hulk_enhanced_mod(double a, double b);
char* hulk_triple_add(const char* a, const char* b, const char* c);
int hulk_logical_and(int a, int b);
int hulk_logical_or(int a, int b);
int hulk_logical_not(int a);
void hulk_debug(const char* format, ...);
char* hulk_type_of(const char* type_name);
void hulk_assert(int condition, const char* message);

// String operations
char* hulk_string_concat(const char* a, const char* b) {
    if (!a) a = "";
    if (!b) b = "";
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    char* result = malloc(len_a + len_b + 1);
    
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in string concatenation\n");
        exit(1);
    }
    
    strcpy(result, a);
    strcat(result, b);
    return result;
}

char* hulk_string_triple_concat(const char* a, const char* b, const char* c) {
    if (!a) a = "";
    if (!b) b = "";
    if (!c) c = "";
    
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    size_t len_c = strlen(c);
    char* result = malloc(len_a + len_b + len_c + 1);
    
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in triple concatenation\n");
        exit(1);
    }
    
    strcpy(result, a);
    strcat(result, b);
    strcat(result, c);
    return result;
}

char* hulk_string_repeat(const char* str, int times) {
    if (!str || times <= 0) {
        char* empty = malloc(1);
        empty[0] = '\0';
        return empty;
    }
    
    size_t str_len = strlen(str);
    size_t total_len = str_len * times;
    char* result = malloc(total_len + 1);
    
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in string repeat\n");
        exit(1);
    }
    
    result[0] = '\0';
    for (int i = 0; i < times; i++) {
        strcat(result, str);
    }
    
    return result;
}

int hulk_string_equal(const char* a, const char* b) {
    if (!a && !b) return 1;
    if (!a || !b) return 0;
    return strcmp(a, b) == 0 ? 1 : 0;
}

// Enhanced arithmetic operations
double hulk_integer_div(double a, double b) {
    if (b == 0) {
        fprintf(stderr, "Error: Division by zero in integer division (//)\n");
        exit(1);
    }
    return floor(a / b);
}

double hulk_enhanced_mod(double a, double b) {
    if (b == 0) {
        fprintf(stderr, "Error: Modulo by zero in enhanced modulo (%%)\n");
        exit(1);
    }
    // Enhanced modulo: handles negative numbers differently
    double result = fmod(a, b);
    if (result < 0 && b > 0) {
        result += b;
    } else if (result > 0 && b < 0) {
        result += b;
    }
    return result;
}

// Triple operations
char* hulk_triple_add(const char* a, const char* b, const char* c) {
    // For numeric triple add, we convert to string representation
    return hulk_string_triple_concat(a, b, c);
}

// Logical operations
int hulk_logical_and(int a, int b) {
    return (a != 0) && (b != 0) ? 1 : 0;
}

int hulk_logical_or(int a, int b) {
    return (a != 0) || (b != 0) ? 1 : 0;
}

int hulk_logical_not(int a) {
    return (a == 0) ? 1 : 0;
}

// Built-in functions
void hulk_debug(const char* format, ...) {
    printf("[DEBUG] ");
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

char* hulk_type_of(const char* type_name) {
    size_t len = strlen(type_name);
    char* result = malloc(len + 1);
    if (!result) {
        fprintf(stderr, "Error: Memory allocation failed in type_of\n");
        exit(1);
    }
    strcpy(result, type_name);
    return result;
}

void hulk_assert(int condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Assertion failed: %s\n", message ? message : "No message provided");
        exit(1);
    }
}

// Standard math functions wrappers
double hulk_sin(double x) { return sin(x); }
double hulk_cos(double x) { return cos(x); }
double hulk_sqrt(double x) { 
    if (x < 0) {
        fprintf(stderr, "Error: Square root of negative number\n");
        exit(1);
    }
    return sqrt(x); 
}
double hulk_log(double x) { 
    if (x <= 0) {
        fprintf(stderr, "Error: Logarithm of non-positive number\n");
        exit(1);
    }
    return log(x); 
}
double hulk_exp(double x) { return exp(x); }
double hulk_pow(double x, double y) { return pow(x, y); }

// Memory management for strings
void hulk_free_string(char* str) {
    if (str) {
        free(str);
    }
}

// Print functions
void hulk_print_number(double value) {
    if (floor(value) == value) {
        printf("%.0f", value);
    } else {
        printf("%g", value);
    }
}

void hulk_print_string(const char* str) {
    if (str) {
        printf("%s", str);
    }
}

void hulk_print_boolean(int value) {
    printf("%s", value ? "true" : "false");
}

void hulk_println() {
    printf("\n");
}
