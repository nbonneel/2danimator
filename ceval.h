#ifndef CEVAL
#define CEVAL
// functions accessible from main() 
// - double ceval_result(char * inp) returns the result of valid math expression stored as a char array `inp`
// - void ceval_tree(char * inp) prints the parse tree for the input expression `inp`
// - define CEVAL_EPSILON (default value : 1e-2), CEVAL_DELTA (default value : 1e-6) and CEVAL_MAX_DIGITS (default value : 15) manually before the include directive
// - define CEVAL_STOICAL before the #include directive to use the parser/evaluator in stoical (non-complaining) mode. It suppresses all the error messages from [ceval]. 

#include<stdio.h>
#include<string.h>
#include<math.h>
#include<ctype.h>
#include<stdarg.h>
/****************************************** TOKENS ***********************************************/
typedef enum ceval_node_id {
    CEVAL_WHITESPACE, CEVAL_OPENPAR, CEVAL_CLOSEPAR, CEVAL_COMMA, 
    CEVAL_OR, CEVAL_AND, CEVAL_BIT_OR, CEVAL_BIT_XOR,
    CEVAL_BIT_AND, CEVAL_EQUAL, CEVAL_NOTEQUAL,CEVAL_LESSER,
    CEVAL_GREATER, CEVAL_LESSER_S, CEVAL_GREATER_S, CEVAL_BIT_LSHIFT, 
    CEVAL_BIT_RSHIFT, CEVAL_PLUS, CEVAL_MINUS, CEVAL_TIMES, 
    CEVAL_DIVIDE, CEVAL_MODULO, CEVAL_QUOTIENT, CEVAL_POW,
    CEVAL_GCD, CEVAL_HCF, CEVAL_LCM, CEVAL_LOG,
    CEVAL_ATAN2, CEVAL_POWFUN, CEVAL_CURVEX, CEVAL_CURVEY, CEVAL_CURVEA,

    CEVAL_ABS, CEVAL_EXP, CEVAL_SQRT,CEVAL_CBRT, 
    CEVAL_LN, CEVAL_LOG10, CEVAL_CEIL, CEVAL_FLOOR, 
    CEVAL_SIGNUM, CEVAL_FACTORIAL, CEVAL_INT, CEVAL_FRAC, 
    CEVAL_DEG2RAD, CEVAL_RAD2DEG, CEVAL_SIN, CEVAL_COS, 
    CEVAL_TAN, CEVAL_ASIN, CEVAL_ACOS, CEVAL_ATAN, 
    CEVAL_SINH, CEVAL_COSH, CEVAL_TANH,CEVAL_NOT, 
    CEVAL_BIT_NOT,CEVAL_POSSIGN, CEVAL_NEGSIGN, 
    
    CEVAL_NUMBER, CEVAL_CONST_PI, CEVAL_CONST_E
} ceval_node_id;
typedef enum ceval_token_prec_specifiers {
// precedences :: <https://en.cppreference.com/w/cpp/language/operator_precedence>
// these precision specifiers are ordered in the ascending order of their precedences
// here, the higher precedence operators are evaluated first and end up at the bottom of the parse trees
    CEVAL_PREC_IGNORE, 
    // {' ', '\t', '\n', '\b', '\r'}
    CEVAL_PREC_PARANTHESES,
    // {'(', ')'}
    CEVAL_PREC_COMMA_OPR,
    // {','}
    CEVAL_PREC_LOGICAL_OR_OPR,
    // {'||'}
    CEVAL_PREC_LOGICAL_AND_OPR,
    // {'&&'}
    CEVAL_PREC_BIT_OR_OPR,
    // {'|'}
    CEVAL_PREC_BIT_XOR_OPR,
    // {'^'}
    CEVAL_PREC_BIT_AND_OPR,
    // {'&'}
    CEVAL_PREC_EQUALITY_OPRS,
    // {'==', '!='}
    CEVAL_PREC_RELATIONAL_OPRS,
    // {'<', '>', '<=', '>='}
    CEVAL_PREC_BIT_SHIFT_OPRS,
    // {'<<', '>>'}
    CEVAL_PREC_ADDITIVE_OPRS,
    // {'+', '-'}
    CEVAL_PREC_SIGN_OPRS,
    // {'+', '-'}
    CEVAL_PREC_MULTIPLICATIVE_OPRS,
    // {'*', '/', '%', '//'}
    CEVAL_PREC_EXPONENTIATION_OPR,
    // {'**'}
    CEVAL_PREC_FUNCTIONS,
    // {
    //     'exp()', 'sqrt()', 'cbrt()', 'sin()',
    //     'cos()', 'tan()', 'asin()', 'acos()', 
    //     'atan()', 'sinh()', 'cosh()', 'tanh()', 
    //     'abs()', 'ceil()', 'floor()', 'log10()', 
    //     'ln()', 'deg2rad()', 'rad2deg()', 'signum()',
    //     'int()', 'frac()', 'fact()', `pow()`, 
    //     `atan2()`, `gcd()`, `hcf()`, `lcm()`,
    //     `log()`
    // }
    CEVAL_PREC_NOT_OPRS,
    // {'!', '~'}}
    CEVAL_PREC_NUMERIC
    // {'pi', 'pi', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'}
} ceval_token_prec_specifiers;
typedef enum ceval_token_type {
    CEVAL_UNARY_OPERATOR,
    CEVAL_BINARY_OPERATOR,
    CEVAL_UNARY_FUNCTION,
    CEVAL_BINARY_FUNCTION,
    CEVAL_OTHER
} ceval_token_type;
typedef struct ceval_token_info_ {
    ceval_node_id id;
    const char * symbol; 
    double prec;
    ceval_token_type token_type;
} ceval_token_info_; 
static ceval_token_info_ ceval_token_info[] = {
    { CEVAL_WHITESPACE, " ", CEVAL_PREC_IGNORE, CEVAL_OTHER },
    { CEVAL_WHITESPACE, "\n", CEVAL_PREC_IGNORE, CEVAL_OTHER },
    { CEVAL_WHITESPACE, "\t", CEVAL_PREC_IGNORE, CEVAL_OTHER },
    { CEVAL_WHITESPACE, "\r", CEVAL_PREC_IGNORE, CEVAL_OTHER },
    { CEVAL_WHITESPACE, "\b", CEVAL_PREC_IGNORE, CEVAL_OTHER },

    { CEVAL_DEG2RAD, "deg2rad", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_RAD2DEG, "rad2deg", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },

    { CEVAL_SIGNUM, "signum", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },

    { CEVAL_ATAN2, "atan2", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
	{ CEVAL_CURVEX, "curvex", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION },
	{ CEVAL_CURVEY, "curvey", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION },
	{ CEVAL_CURVEA, "curvea", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION },
    { CEVAL_LOG10, "log10", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_FLOOR, "floor", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },

    { CEVAL_SQRT, "sqrt", CEVAL_PREC_FUNCTIONS , CEVAL_UNARY_FUNCTION },
    { CEVAL_CBRT, "cbrt", CEVAL_PREC_FUNCTIONS , CEVAL_UNARY_FUNCTION },
    { CEVAL_CEIL, "ceil", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_FRAC, "frac", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_FACTORIAL, "fact", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION }, 
    { CEVAL_SINH, "sinh", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION }, 
    { CEVAL_COSH, "cosh", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_TANH, "tanh", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_ASIN, "asin", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION }, 
    { CEVAL_ACOS, "acos", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_ATAN, "atan", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    
    { CEVAL_POWFUN, "pow", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
    { CEVAL_GCD, "gcd", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
    { CEVAL_HCF, "hcf", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
    { CEVAL_LCM, "lcm", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
    { CEVAL_LOG, "log", CEVAL_PREC_FUNCTIONS, CEVAL_BINARY_FUNCTION }, 
    { CEVAL_INT, "int", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_SIN, "sin", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_COS, "cos", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_TAN, "tan", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_ABS, "abs", CEVAL_PREC_FUNCTIONS , CEVAL_UNARY_FUNCTION },
    { CEVAL_EXP, "exp", CEVAL_PREC_FUNCTIONS , CEVAL_UNARY_FUNCTION },

    { CEVAL_CONST_PI, "pi", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_LN, "ln", CEVAL_PREC_FUNCTIONS, CEVAL_UNARY_FUNCTION },
    { CEVAL_OR, "||", CEVAL_PREC_LOGICAL_OR_OPR, CEVAL_BINARY_OPERATOR },
    { CEVAL_AND, "&&", CEVAL_PREC_LOGICAL_AND_OPR, CEVAL_BINARY_OPERATOR },
    { CEVAL_EQUAL, "==", CEVAL_PREC_EQUALITY_OPRS, CEVAL_BINARY_OPERATOR },
    { CEVAL_NOTEQUAL, "!=", CEVAL_PREC_EQUALITY_OPRS, CEVAL_BINARY_OPERATOR },
    { CEVAL_LESSER, "<=", CEVAL_PREC_RELATIONAL_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_GREATER, ">=", CEVAL_PREC_RELATIONAL_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_BIT_LSHIFT, "<<", CEVAL_PREC_BIT_SHIFT_OPRS, CEVAL_BINARY_OPERATOR},
    { CEVAL_BIT_RSHIFT, ">>", CEVAL_PREC_BIT_SHIFT_OPRS, CEVAL_BINARY_OPERATOR},
    { CEVAL_QUOTIENT, "//", CEVAL_PREC_MULTIPLICATIVE_OPRS , CEVAL_BINARY_OPERATOR }, 
    { CEVAL_POW, "**", CEVAL_PREC_EXPONENTIATION_OPR , CEVAL_BINARY_OPERATOR },

    { CEVAL_CONST_E, "e", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_OPENPAR, "(", CEVAL_PREC_PARANTHESES, CEVAL_OTHER },
    { CEVAL_CLOSEPAR, ")", CEVAL_PREC_PARANTHESES, CEVAL_OTHER },
    { CEVAL_COMMA, ",", CEVAL_PREC_COMMA_OPR , CEVAL_BINARY_OPERATOR },
    { CEVAL_BIT_OR, "|", CEVAL_PREC_BIT_OR_OPR, CEVAL_BINARY_OPERATOR},
    { CEVAL_BIT_XOR, "^", CEVAL_PREC_BIT_XOR_OPR, CEVAL_BINARY_OPERATOR},
    { CEVAL_BIT_AND, "&", CEVAL_PREC_BIT_AND_OPR, CEVAL_BINARY_OPERATOR},
    { CEVAL_LESSER_S, "<", CEVAL_PREC_RELATIONAL_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_GREATER_S, ">", CEVAL_PREC_RELATIONAL_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_PLUS, "+", CEVAL_PREC_ADDITIVE_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_MINUS, "-", CEVAL_PREC_ADDITIVE_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_POSSIGN, "+", CEVAL_PREC_SIGN_OPRS, CEVAL_UNARY_OPERATOR }, 
    { CEVAL_NEGSIGN, "-", CEVAL_PREC_SIGN_OPRS, CEVAL_UNARY_OPERATOR }, 
    { CEVAL_TIMES, "*", CEVAL_PREC_MULTIPLICATIVE_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_DIVIDE, "/", CEVAL_PREC_MULTIPLICATIVE_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_MODULO, "%", CEVAL_PREC_MULTIPLICATIVE_OPRS , CEVAL_BINARY_OPERATOR },
    { CEVAL_NOT, "!", CEVAL_PREC_NOT_OPRS, CEVAL_UNARY_FUNCTION},
    { CEVAL_BIT_NOT, "~", CEVAL_PREC_NOT_OPRS, CEVAL_UNARY_OPERATOR},

    { CEVAL_NUMBER, "0", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "1", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "2", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "3", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "4", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "5", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "6", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "7", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "8", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
    { CEVAL_NUMBER, "9", CEVAL_PREC_NUMERIC, CEVAL_OTHER },
}; 
#ifndef CEVAL_TOKEN_TABLE_SIZE
#define CEVAL_TOKEN_TABLE_SIZE sizeof(ceval_token_info) / sizeof(ceval_token_info[0])
#endif
// function prototypes for mapping the attributes of various tokens
int ceval_is_binary_opr(ceval_node_id);
int ceval_is_binary_fun(ceval_node_id);
const char * ceval_token_symbol(ceval_node_id);
ceval_node_id ceval_token_id(char * symbol);
double ceval_token_prec(ceval_node_id);

int ceval_is_binary_opr(ceval_node_id id);
int ceval_is_binary_fun(ceval_node_id id);
const char * ceval_token_symbol(ceval_node_id id);
ceval_node_id ceval_token_id(char * symbol);
double ceval_token_prec(ceval_node_id id);

typedef struct ceval_node {
    enum ceval_node_id id;
    double pre;
    double number;
    struct ceval_node * left, * right, * parent;
}
ceval_node;
#ifdef __cplusplus
  #define CEVAL_CXX
  #include<iostream>
  #include<string>
#endif
/***************************************** !TOKENS *******************************************/

/****************************************** FUNCTIONS ******************************************/
//constant definitions
#ifdef M_PI
#define CEVAL_PI M_PI
#else
#define CEVAL_PI 3.14159265358979323846
#endif
#ifdef M_E
#define CEVAL_E M_E
#else
#define CEVAL_E 2.71828182845904523536
#endif

#ifndef CEVAL_EPSILON
#define CEVAL_EPSILON 1e-2
#endif
#ifndef CEVAL_DELTA
#define CEVAL_DELTA 1e-6
#endif
#ifndef CEVAL_MAX_DIGITS
#define CEVAL_MAX_DIGITS 15
#endif
//these can be defined by the user before the include directive depending the desired level of precision

//helper function prototypes
void ceval_error(const char * , ...);
double ceval_gcd_binary(int, int);
char * ceval_shrink(char * );

//single argument funtion prototypes
double ceval_signum(double);
double ceval_asin(double);
double ceval_acos(double);
double ceval_atan(double);
double ceval_sin(double);
double ceval_cos(double);
double ceval_tan(double);
double ceval_sinh(double);
double ceval_cosh(double);
double ceval_tanh(double);
double ceval_rad2deg(double);
double ceval_deg2rad(double);
double ceval_int_part(double);
double ceval_frac_part(double);
double ceval_log10(double);
double ceval_ln(double);
double ceval_exp(double);
double ceval_factorial(double);
double ceval_positive_sign(double);
double ceval_negative_sign(double);
double ceval_abs(double);
double ceval_sqrt(double);
double ceval_sqrt(double);
double ceval_cbrt(double);
double ceval_ceil(double);
double ceval_floor(double);
double ceval_not(double);
double ceval_bit_not(double);

//single argument function definitions
static double(*single_arg_fun[])(double) = {
	// double_arg_fun (first three tokens are whitespace and parantheses)
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	// single_arg_fun
	ceval_abs, ceval_exp, ceval_sqrt, ceval_cbrt,
	ceval_ln, ceval_log10, ceval_ceil, ceval_floor,
	ceval_signum, ceval_factorial, ceval_int_part, ceval_frac_part,
	ceval_deg2rad, ceval_rad2deg, ceval_sin, ceval_cos,
	ceval_tan, ceval_asin, ceval_acos, ceval_atan,
	ceval_sinh, ceval_cosh, ceval_tanh, ceval_not,
	ceval_bit_not, ceval_positive_sign, ceval_negative_sign,
	// number and constant tokens
	NULL, NULL, NULL
};

//double argument function prototypes
double ceval_sum(double, double, int);
double ceval_diff(double, double, int);
double ceval_prod(double, double, int);
double ceval_div(double, double, int);
double ceval_quotient(double, double, int);
double ceval_modulus(double, double, int);
double ceval_gcd(double, double, int);
double ceval_hcf(double, double, int);
double ceval_lcm(double, double, int);
double ceval_log(double, double, int);
double ceval_are_equal(double, double, int);
double ceval_not_equal(double, double, int);
double ceval_lesser(double, double, int);
double ceval_greater(double, double, int);
double ceval_lesser_s(double, double, int);
double ceval_greater_s(double, double, int);
double ceval_comma(double, double, int);
double ceval_power(double, double, int);
double ceval_atan2(double, double, int);
double ceval_curveX(double, double, int);
double ceval_curveY(double, double, int);
double ceval_curveA(double, double, int);
double ceval_and(double, double, int);
double ceval_or(double, double, int);
double ceval_bit_and(double, double, int);
double ceval_bit_xor(double, double, int);
double ceval_bit_or(double, double, int);
double ceval_bit_lshift(double, double, int);
double ceval_bit_rshift(double, double, int);


//double argument function definitions
static double( * double_arg_fun[])(double, double, int) = {
    // double_arg_fun (first three tokens are whitespace and parantheses)
    NULL, NULL, NULL, ceval_comma,
    ceval_or, ceval_and, ceval_bit_or, ceval_bit_xor,
    ceval_bit_and, ceval_are_equal, ceval_not_equal, ceval_lesser,
    ceval_greater, ceval_lesser_s, ceval_greater_s, ceval_bit_lshift,
    ceval_bit_rshift, ceval_sum, ceval_diff, ceval_prod,
    ceval_div, ceval_modulus, ceval_quotient, ceval_power, 
    ceval_gcd, ceval_hcf, ceval_lcm, ceval_log,
    ceval_atan2, ceval_power, ceval_curveX,  ceval_curveY,  ceval_curveA,
    // single_arg_fun
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL,
    // number and constant tokens
    NULL, NULL, NULL
};

/**************************************** !FUNCTIONS ********************************************/

/***************************************** PARSE_TREE_CONSTRUCTION *******************************************/
void * ceval_make_tree(char * );
ceval_node * ceval_insert_node(ceval_node * , ceval_node, int);
void ceval_print_tree(const void * );
void ceval_print_node(const ceval_node * , int);
void ceval_delete_node(ceval_node * );
void ceval_delete_tree(void * );


/***************************************** !PARSE_TREE_CONSTRUCTION *******************************************/

/***************************************** EVALUATION *******************************************/
double ceval_evaluate_tree_(const ceval_node * );
double ceval_evaluate_tree(const void * );


/***************************************** !EVALUATION *******************************************/

/***************************************** MAIN FUNCTIONS *******************************************/
// prototypes 
double ceval_result(char *);
void ceval_tree(char *);



#ifdef CEVAL_CXX
    // prototypes
    double ceval_result2(std::string);
    void ceval_tree2(std::string);


#endif
/***************************************** !MAIN FUNCTIONS *******************************************/
#endif
