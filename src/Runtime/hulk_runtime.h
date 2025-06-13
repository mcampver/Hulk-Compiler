#ifndef HULK_RUNTIME_H
#define HULK_RUNTIME_H

#ifdef __cplusplus
extern "C" {
#endif

// String operations
char* hulk_string_concat(const char* a, const char* b);
char* hulk_string_triple_concat(const char* a, const char* b, const char* c);
char* hulk_string_repeat(const char* str, int times);
int hulk_string_equal(const char* a, const char* b);

// Enhanced arithmetic operations
double hulk_integer_div(double a, double b);
double hulk_enhanced_mod(double a, double b);
char* hulk_triple_add(const char* a, const char* b, const char* c);

// Logical operations
int hulk_logical_and(int a, int b);
int hulk_logical_or(int a, int b);
int hulk_logical_not(int a);

// Built-in functions
void hulk_debug(const char* format, ...);
char* hulk_type_of(const char* type_name);
void hulk_assert(int condition, const char* message);

// Standard math functions
double hulk_sin(double x);
double hulk_cos(double x);
double hulk_sqrt(double x);
double hulk_log(double x);
double hulk_exp(double x);
double hulk_pow(double x, double y);

// Memory management
void hulk_free_string(char* str);

// Print functions
void hulk_print_number(double value);
void hulk_print_string(const char* str);
void hulk_print_boolean(int value);
void hulk_println();

#ifdef __cplusplus
}
#endif

#endif // HULK_RUNTIME_H
