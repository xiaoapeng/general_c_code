/**
 * @file debug.c
 * @brief debug模块
 * @author simon.xiaoapeng (simon.xiaoapeng@gmail.com)
 * @version 1.0
 * @date 2022-09-03
 * 
 * @copyright Copyright (c) 2022  simon.xiaoapeng@gmail.com
 * 
 * @par 修改日志:
**/



#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include <time.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "debug.h"


#define eh_malloc(size) malloc(size)
#define eh_free(ptr) free(ptr)


#define FORMAT_FLOAT_F_RANGE_MAX        (1.e+18)
#define FORMAT_FLOAT_F_RANGE_MIN        (-(1.e+18))
#define FORMAT_FLOAT_POWERS_TAB_SIZE    19
#define FORMAT_STACK_CACHE_SIZE         (16)
#define FORMAT_LOG10_TAYLOR_TERMS       (4)
#define FORMAT_DBL_EXP_OFFSET           (1023)
#define FORMAT_DBL_EXP_OFFSET           (1023)
#define FORMAT_DBL_MIN_POW10            (1.e-308)

#define FORMAT_LEFT         0x00000001          /* 输出结果左对齐 */
#define FORMAT_PLUS         0x00000002          /* 输出结果若为正数则添加正号 */
#define FORMAT_SPACE        0x00000004          /* 输出结果若为正数则添加空格 */
#define FORMAT_SPECIAL      0x00000008          /* 特殊标志位，用于添加一些特定符号，比如%#x自动添加0x */
#define FORMAT_ZEROPAD      0x00000010          /* 当宽度大于输出结果长度时，则用0填充 */
#define FORMAT_LARGE        0x00000020          /* 使用大写输出 */
#define FORMAT_SIGNED       0x00000040          /* 有符号输出 */
#define FORMAT_FLOAT_E      0x00000080          /* 浮点数输出  E格式*/
#define FORMAT_FLOAT_F      0x00000100          /* 浮点数输出  F格式*/
#define FORMAT_FLOAT_G      0x00000200          /* 浮点数输出  G格式*/

enum stream_type{
    STREAM_TYPE_FUNCTION,
    STREAM_TYPE_MEMORY,
};

enum format_qualifier{
    FORMAT_QUALIFIER_NONE,
    FORMAT_QUALIFIER_LONG,
    FORMAT_QUALIFIER_LONG_LONG,
    FORMAT_QUALIFIER_SHORT,
    FORMAT_QUALIFIER_CHAR,
    FORMAT_QUALIFIER_SIZE_T,
};

enum base_type{
    BASE_TYPE_BIN = 2, 
    BASE_TYPE_OCT = 8, 
    BASE_TYPE_DEC = 10, 
    BASE_TYPE_HEX = 16
};

struct stream_out{
    enum stream_type type;
    union{
        struct{
            void (*write)(void *stream, const uint8_t *buf, size_t size);
            uint8_t *cache;
            uint8_t *pos;
            uint8_t *end;
        }f;
        struct{
            uint8_t *buf;
            uint8_t *pos;
            uint8_t *end;
        }m;
    };
};


static const char small_digits[] = "0123456789abcdef";
static const char large_digits[] = "0123456789ABCDEF";
static const double powers_of_10[FORMAT_FLOAT_POWERS_TAB_SIZE] = {
  1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08,
  1e09, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18
};
static uint8_t _stdout_cache[DEBUG_CONFIG_STDOUT_MEM_CACHE_SIZE];

__attribute__((weak)) void stdout_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    (void)buf;
    (void)size;
}

static struct stream_out _stdout = {
    .type = STREAM_TYPE_FUNCTION,
    .f = {
        .write = stdout_write,
        .cache = _stdout_cache,
        .pos = _stdout_cache,
        .end = _stdout_cache + DEBUG_CONFIG_STDOUT_MEM_CACHE_SIZE,
    },
};
struct double_components {
  uint64_t              integral;
  uint64_t              fractional;
  bool                  is_negative;
};

union double_union{
    double d;
    struct{
        uint64_t mantissa:52;
        uint64_t exponent:11;
        uint64_t sign:1;
    };
    struct{
        uint64_t v64;
    };
};

struct scaling_factor {
    double raw_factor;
    bool multiply; // if true, need to multiply by raw_factor; otherwise need to divide by it
};

static int bastardized_floor(double x)
{
    int n;
    if (x >= 0) { return (int) x; }
    n = (int) x;
    return ( ((double) n) == x ) ? n : n-1;
}

static double pow10_of_int(int floored_exp10)
{
    union double_union du = {.v64 = 0};
    int exp2;
    if (floored_exp10 == -DBL_MAX_10_EXP ) {
        return FORMAT_DBL_MIN_POW10;
    }
    exp2 = bastardized_floor(floored_exp10 * 3.321928094887362 + 0.5);
    const double z  = floored_exp10 * 2.302585092994046 - exp2 * 0.6931471805599453;
    const double z2 = z * z;
    du.exponent = (uint16_t)(((exp2 ) + FORMAT_DBL_EXP_OFFSET) & 0x7FF);
    du.d *= 1 + 2 * z / (2 - z + (z2 / (6 + (z2 / (10 + z2 / 14)))));
    return du.d;
}



static double log10_of_positive(double positive_number){
    union double_union du = {.d = positive_number};
    int exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;
    double z;
    du.exponent = FORMAT_DBL_EXP_OFFSET;
    z = du.d - 1.5;
    return (
    // Taylor expansion around 1.5:
    0.1760912590556812420           // Expansion term 0: ln(1.5)            / ln(10)
    + z     * 0.2895296546021678851 // Expansion term 1: (M - 1.5)   * 2/3  / ln(10)
#if FORMAT_LOG10_TAYLOR_TERMS > 2
    - z*z   * 0.0965098848673892950 // Expansion term 2: (M - 1.5)^2 * 2/9  / ln(10)
#if FORMAT_LOG10_TAYLOR_TERMS > 3
    + z*z*z * 0.0428932821632841311 // Expansion term 2: (M - 1.5)^3 * 8/81 / ln(10)
#endif
#endif
    // exact log_2 of the exponent x, with logarithm base change
    + exp2 * 0.30102999566398119521 // = exp2 * log_10(2) = exp2 * ln(2)/ln(10)
  );
}

static int num_rsh(unsigned long long *number, int base){
    int res;
    res = (int)((*number) % (unsigned long long)base);
    *number = (*number) / (unsigned long long)base;
    return res;
}

/* 获取数字在任意进制下的位数 */
static int num_bit_count(unsigned long long number, int base){
    int res = 0;
    do{
        number = number / (unsigned long long)base;
        res++;
    }while(number);
    return res;
}

static inline void streamout_in_byte(struct stream_out *stream, char ch){
    bool out = false;
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:
            if(stream->f.pos < stream->f.end){
                *stream->f.pos = (uint8_t)ch;
                stream->f.pos++;
                if(ch == '\n') out = true;
            }
            if( stream->f.pos == stream->f.end || out ){
                stream->f.write(&stream, stream->f.cache,  (size_t)(stream->f.pos - stream->f.cache));
                stream->f.pos = stream->f.cache;
            }
            break;
        case STREAM_TYPE_MEMORY:
            if(stream->m.pos < stream->m.end){
                *stream->m.pos = (uint8_t)ch;
                stream->m.pos++;
            }
            break;
    }
}

static inline void streamout_finish(struct stream_out *stream){
    switch(stream->type){
        case STREAM_TYPE_FUNCTION:
            break;
        case STREAM_TYPE_MEMORY:
            if(stream->m.pos < stream->m.end){
                *stream->m.pos = '\0';
            }else{
                *(stream->m.end - 1) = '\0';
            }
    }
}

static inline int skip_atoi(const char **s)
{
    int i = 0;
    while (isdigit((int)(**s)))
        i = i * 10 + (*((*s)++) - '0');
    return i;
}

static inline int vprintf_char(struct stream_out *stream, char ch, int field_width, int flags){
    int n = 0;
    n = field_width <= 1 ? 1 : field_width;
    if(flags & FORMAT_LEFT)
        streamout_in_byte(stream, ch);
    while(field_width-- > 1)
        streamout_in_byte(stream, ' ');
    if(!(flags & FORMAT_LEFT))
        streamout_in_byte(stream, ch);
    return n;
}

static inline int vprintf_string(struct stream_out *stream, char *s, int field_width, int precision, int flags){
    int n = 0;
    int len;
    int diff;
    if( s == NULL )
        s= "(null)";
    if(field_width < 0){
        for(len = 0; (len != precision) && (s[len]); len++,n++)
            streamout_in_byte(stream, s[len]);
        return n;
    }

    for (len = 0; (len != precision) && (s[len]); len++) { }

    if(field_width <= len){
        for(int i=0; i<len; i++,n++){
            streamout_in_byte(stream, s[i]);
        }
        return n;
    }
    diff = field_width - len;
    if(!(flags & FORMAT_LEFT)){
        /* 右对齐 */
        for(int i=0; i<diff; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    for(int i=0; i<len; i++,n++){
        streamout_in_byte(stream, s[i]);
    }
    
    if(flags & FORMAT_LEFT){
        /* 左对齐 */
        for(int i=0; i<diff; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    return n;
}

static double apply_scaling(double num, struct scaling_factor normalization)
{
    return normalization.multiply ? num * normalization.raw_factor : num / normalization.raw_factor;
}

static double unapply_scaling(double normalized, struct scaling_factor normalization)
{
    return normalization.multiply ? normalized / normalization.raw_factor : normalized * normalization.raw_factor;
}

static struct scaling_factor update_normalization(struct scaling_factor sf, double extra_multiplicative_factor)
{
    struct scaling_factor result;
    if (sf.multiply) {
        result.multiply = true;
        result.raw_factor = sf.raw_factor * extra_multiplicative_factor;
    }
    else {
        union double_union du = {.d = sf.raw_factor};
        int factor_exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;
        du.d = extra_multiplicative_factor;
        int extra_factor_exp2 = du.exponent - FORMAT_DBL_EXP_OFFSET;;

        // Divide the larger-exponent raw raw_factor by the smaller
        if (abs(factor_exp2) > abs(extra_factor_exp2)) {
            result.multiply = false;
            result.raw_factor = sf.raw_factor / extra_multiplicative_factor;
        }else {
            result.multiply = true;
            result.raw_factor = extra_multiplicative_factor / sf.raw_factor;
        }
    }
    return result;
}


void float_decentralized(double num, struct double_components *components, int precision) 
{
    union double_union du = {.d = num};
    double abs_number;
    double remainder;
    components->is_negative = du.sign;
    abs_number = (components->is_negative) ? -num : num;
    components->integral = (uint64_t)abs_number;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE)
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;
    remainder = (abs_number - (double) components->integral) * powers_of_10[precision];
    components->fractional = (uint64_t)remainder;

    remainder -= (double) components->fractional;

    /* 银行家舍入法， 四舍六入五取偶 */
    if(remainder > 0.5){
        ++components->fractional;
        /* 检查是否溢出到整数位去了 */
        if ((double) components->fractional >= powers_of_10[precision]) {
            components->fractional = 0;
            ++components->integral;
        }
    }else if ((remainder == 0.5) && ((components->fractional == 0U) || (components->fractional & 1U))) {
        ++components->fractional;
    }

    if (precision == 0U) {
        remainder = abs_number - (double) components->integral;
        if ((!(remainder < 0.5) || (remainder > 0.5)) && (components->integral & 1)) {
            ++components->integral;
        }
    }
    return ;
}

static void float_normalized_decentralized(
   struct double_components *components,
   bool negative, int precision, double non_normalized, 
   struct scaling_factor normalization, int floored_exp10)
{
    double scaled = apply_scaling(non_normalized, normalization);
    bool close_to_representation_extremum = ( (-floored_exp10 + (int) precision) >= DBL_MAX_10_EXP - 1 );
    double remainder;
    double prec_power_of_10;
    struct scaling_factor account_for_precision;
    double scaled_remainder;
    double rounding_threshold;
    
    components->is_negative = negative;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE)
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;

    if (close_to_representation_extremum) {
        // We can't have a normalization factor which also accounts for the precision, i.e. moves
        // some decimal digits into the mantissa, since it's unrepresentable, or nearly unrepresentable.
        // So, we'll give up early on getting extra precision...
        float_decentralized(negative ? -scaled : scaled, components, precision);
        return ;
    }
    components->integral = (uint64_t) scaled;
    remainder = non_normalized - unapply_scaling((double) components->integral, normalization);
    prec_power_of_10 = powers_of_10[precision];
    account_for_precision = update_normalization(normalization, prec_power_of_10);
    scaled_remainder = apply_scaling(remainder, account_for_precision);
    rounding_threshold = 0.5;

    components->fractional = (uint64_t) scaled_remainder; // when precision == 0, the assigned value should be 0
    scaled_remainder -= (double) components->fractional; //when precision == 0, this will not change scaled_remainder

    components->fractional += (scaled_remainder >= rounding_threshold);
    if (scaled_remainder == rounding_threshold) {
        // banker's rounding: Round towards the even number (making the mean error 0)
        components->fractional &= ~((uint64_t) 0x1);
    }
    // handle rollover, e.g. the case of 0.99 with precision 1 becoming (0,100),
    // and must then be corrected into (1, 0).
    // Note: for precision = 0, this will "translate" the rounding effect from
    // the fractional part to the integral part where it should actually be
    // felt (as prec_power_of_10 is 1)
    if ((double) components->fractional >= prec_power_of_10) {
        components->fractional = 0;
        ++components->integral;
    }

}

static int vprintf_float_decimalism_or_normalized(struct stream_out *stream, struct double_components *components_num, 
    int field_width, int precision, int flags, int floored_exp10){
    char _number_buf[FORMAT_STACK_CACHE_SIZE];
    char *number_buf = _number_buf;
    char sign = 0;
    char dot = 0;
    char exponent_sign_e = 0;
    char exponent_sign = 0;
    int n = 0;
    int need_buf_min_size = 0;
    int integral_valid_len = 0;
    int fractional_pad_len = 0;
    int fractional_valid_len = 0;
    int fractional_precision_pad_len = 0;
    int exponent_valid_len = 0;
    int valid_len = 0;
    int space_or_zero_pad_len = 0;
    unsigned long long number = 0;
    const char *digits = small_digits;

    if(precision >= FORMAT_FLOAT_POWERS_TAB_SIZE){
        fractional_precision_pad_len = precision - (FORMAT_FLOAT_POWERS_TAB_SIZE - 1);
        precision = FORMAT_FLOAT_POWERS_TAB_SIZE - 1;
    }

    if(components_num->is_negative){
        sign = '-';
    }else if(flags & FORMAT_PLUS){
        sign = '+';
    }else if(flags & FORMAT_SPACE){
        sign = ' ';
    }
    if(precision > 0 || flags & FORMAT_SPECIAL){
        dot = '.';
    }

    if(flags & FORMAT_FLOAT_E){
        exponent_sign_e = flags & FORMAT_LARGE ? 'E' : 'e';
        exponent_sign = floored_exp10 >= 0 ? '+' : '-';
        floored_exp10 = abs(floored_exp10);
        exponent_valid_len = num_bit_count((unsigned long long)floored_exp10, BASE_TYPE_DEC);
    }


    integral_valid_len = num_bit_count(components_num->integral, BASE_TYPE_DEC);
    if(precision > 0){
        fractional_valid_len = num_bit_count(components_num->fractional, BASE_TYPE_DEC);
        if(precision > fractional_valid_len)
            fractional_pad_len = precision - fractional_valid_len;
    }

    need_buf_min_size = integral_valid_len > fractional_valid_len ? 
        integral_valid_len : fractional_valid_len;
    need_buf_min_size = need_buf_min_size > exponent_valid_len ?
        need_buf_min_size : exponent_valid_len;

    if(need_buf_min_size > FORMAT_STACK_CACHE_SIZE){
        number_buf = eh_malloc((size_t)need_buf_min_size);
    }

    if(flags & FORMAT_FLOAT_E){
        valid_len = (sign ? 1 : 0) + integral_valid_len + 
                    (dot ? 1 : 0) + fractional_pad_len + fractional_valid_len + fractional_precision_pad_len +
                    (exponent_sign_e ? 1 : 0) + (exponent_sign ? 1 : 0) + 
                    ((exponent_valid_len < 2) ? 2 : exponent_valid_len);
    }else{
        valid_len = (sign ? 1 : 0) + integral_valid_len + 
                    (dot ? 1 : 0) + fractional_pad_len + fractional_valid_len + fractional_precision_pad_len;
    }
    
    if(field_width > valid_len)
        space_or_zero_pad_len = field_width - valid_len;

    /* 右对齐填充 */
    if(!(flags & FORMAT_LEFT) && !(flags & FORMAT_ZEROPAD)){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, ' ');
    }

    /* 符号位 */
    if(sign){
        streamout_in_byte(stream, sign);
        n++;
    }
    
    /* 0填充 */
    if(flags & FORMAT_ZEROPAD){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 整数部分 */
    number = (unsigned long long)components_num->integral;
    for(int i=(integral_valid_len-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
    }

    for(int i=0; i<integral_valid_len; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }

    /* dot打印 */
    if(dot){
        streamout_in_byte(stream, dot);
        n++;
    }

    /* 小数部分0填充 */
    if(fractional_pad_len > 0){
        for(int i=0; i<fractional_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 小数部分 */
    number = (unsigned long long)components_num->fractional;
    for(int i=(fractional_valid_len-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
    }
    
    for(int i=0; i<fractional_valid_len; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }

    /* 精度不足部分补0 */
    if(fractional_precision_pad_len > 0){
        for(int i=0; i<fractional_precision_pad_len; i++,n++)
            streamout_in_byte(stream, '0');
    }

    /* 指数部分 */
    if(flags & FORMAT_FLOAT_E){       
        streamout_in_byte(stream, exponent_sign_e);
        streamout_in_byte(stream, exponent_sign);
        n += 2;

        number = (unsigned long long)floored_exp10;
        for(int i=(exponent_valid_len-1); i>=0; i--){
            number_buf[i] = digits[num_rsh(&number, (int)BASE_TYPE_DEC)];
        }
        if(exponent_valid_len < 2){
            streamout_in_byte(stream, '0');
            n++;
        }
        for(int i=0; i<exponent_valid_len; i++,n++){
            streamout_in_byte(stream, number_buf[i]);
        }
    }

    /* 左对齐时填充 */
    if(flags & FORMAT_LEFT && !(flags & FORMAT_ZEROPAD)){
        for(int i=0; i<space_or_zero_pad_len; i++,n++)
            streamout_in_byte(stream, ' ');
    }
    if(number_buf != _number_buf)
        eh_free(number_buf);

    return n;
}

static int vprintf_float_e(struct stream_out *stream, double num, int field_width, int precision, int flags){
    union double_union du = {.d = num};
    double abs_number =  du.sign ? -num : num;
    int floored_exp10;
    bool abs_exp10_covered_by_powers_table = false;
    struct double_components components;
    struct scaling_factor normalization = {0};
    
    if(precision < 0)
        precision = 6;

    if(abs_number == 0.0){
        floored_exp10 = 0;
    }else{
        double exp10 = log10_of_positive(abs_number);
        floored_exp10 = bastardized_floor(exp10);
        double p10 = pow10_of_int(floored_exp10);
        if (abs_number < p10) {
            floored_exp10--;
            p10 /= 10;
        }
        abs_exp10_covered_by_powers_table = abs(floored_exp10) < FORMAT_FLOAT_POWERS_TAB_SIZE;
        normalization.raw_factor = abs_exp10_covered_by_powers_table ? powers_of_10[abs(floored_exp10)] : p10;
    }
    /* 只有使用powers_of_10表才有可能用到乘法 */
    normalization.multiply = (floored_exp10 < 0 && abs_exp10_covered_by_powers_table);
    float_normalized_decentralized(&components, du.sign, precision, abs_number, normalization, floored_exp10);
    return vprintf_float_decimalism_or_normalized(stream, &components, field_width, precision, flags, floored_exp10);
    return 0;
}

static inline int vprintf_float_f_or_g(struct stream_out *stream, double num, int field_width, int precision, int flags){
    struct double_components components_num;
    if(num < FORMAT_FLOAT_F_RANGE_MIN || num > FORMAT_FLOAT_F_RANGE_MAX)
        return 0;
    if(precision < 0)
        precision = 6;
    float_decentralized(num, &components_num, precision);
    return vprintf_float_decimalism_or_normalized(stream, &components_num, field_width, precision, flags, 0);
}

static int vprintf_float(struct stream_out *stream, double num, int field_width, int precision, int flags){
    int n=0;
    if(isinf(num) || isnan(num)){
        char *out_str = NULL;
        if(isinf(num)){
            if(num < 0){
                out_str = flags & FORMAT_LARGE ? "-INF":"-inf";
            }else{
                if(flags & FORMAT_PLUS){
                    out_str = flags & FORMAT_LARGE ? "+INF":"+inf";
                }else if(flags & FORMAT_SPACE){
                    out_str = flags & FORMAT_LARGE ? " INF":" inf";
                }else{
                    out_str = flags & FORMAT_LARGE ? "INF":"inf";
                }
            }
        }else if(isnan(num)){
            out_str = flags & FORMAT_LARGE ? "NAN":"nan";
        }
        n += vprintf_string(stream, out_str, field_width, -1, flags);
        return n;
    }
    if(flags & FORMAT_FLOAT_F || flags & FORMAT_FLOAT_G){
        n += vprintf_float_f_or_g(stream, num, field_width, precision, flags);
        if(n > 0) return n;
    }
    
    flags &= (~(FORMAT_FLOAT_F|FORMAT_FLOAT_G));
    flags |= FORMAT_FLOAT_E;
    return vprintf_float_e(stream, num, field_width, precision, flags);
}

static int vprintf_array(struct stream_out *stream, const uint8_t *array, int field_width, 
    int precision, int flags, enum format_qualifier qualifier){
    const char *digits = small_digits;
    const uint8_t *item;
    char item_size;
    int valid_len;
    int array_len;
    int array_reality_len;
    int remainder;
    int space_pad_len = 0;
    int n = 0;

    if(precision < 0) return 0;

    if(flags & FORMAT_LARGE)
        digits = large_digits;
    switch(qualifier){
        case FORMAT_QUALIFIER_LONG:
            item_size = sizeof(unsigned long);
            break;
        case FORMAT_QUALIFIER_LONG_LONG:
            item_size = sizeof(unsigned long long);
            break;
        case FORMAT_QUALIFIER_SHORT:
            item_size = sizeof(unsigned short);
            break;
        case FORMAT_QUALIFIER_CHAR:
            item_size = sizeof(unsigned char);
            break;
        default:
            item_size = sizeof(unsigned int);
            break;
    }
    array_len = precision/item_size;
    remainder = precision % item_size;
    array_reality_len = array_len - (remainder ? 1 : 0);
    valid_len = (array_reality_len * (item_size * 2)) + (array_reality_len - 1);
    if(field_width > valid_len){
        space_pad_len = field_width - valid_len;
    }
    
    if(!(flags & FORMAT_LEFT)){
        for(int i=0; i<space_pad_len; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }

    item = array;
    for(int i=0; i<array_len; i++, item += item_size){
        if(i){
            streamout_in_byte(stream, ' ');
            n++;
        }
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        for(int j=item_size - 1; j>=0; j--, n+=2){
#else
        for(int j=0; j<item_size; j++, n+=2){
#endif
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
    }
    if(remainder){
        streamout_in_byte(stream, ' ');
        n++;
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        for(int i=0;i<(item_size-remainder);i++,n+=2){
            streamout_in_byte(stream, '?');
            streamout_in_byte(stream, '?');
        }
        for(int j=remainder - 1; j>=0; j--, n+=2){
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
#else
        for(int j=0; j<remainder; j++, n+=2){
            streamout_in_byte(stream, digits[(item[j] >> 4) & 0x0f]);
            streamout_in_byte(stream, digits[     (item[j]) & 0x0f]);
        }
        for(int i=0;i<(item_size-remainder);i++,n+=2){
            streamout_in_byte(stream, '?');
            streamout_in_byte(stream, '?');
        }
#endif
    }

    if(flags & FORMAT_LEFT){
        for(int i=0; i<space_pad_len; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    return n;
}
static inline int vprintf_number(struct stream_out *stream, unsigned long long num, int field_width, int precision, int flags, enum base_type base){
    char _number_buf[FORMAT_STACK_CACHE_SIZE];
    char *number_buf = _number_buf;
    char sign = 0;
    char *special = NULL;
    const char *digits = small_digits;
    int bit_count;
    int n = 0;
    int special_count = 0;
    int reality_count = 0;
    int zeropad_count = 0;
    int spacepad_count = 0;
    
    if(base > BASE_TYPE_HEX) return 0;

    if(flags & FORMAT_LARGE)
        digits = large_digits;
    /* 正数化 */
    if(flags & FORMAT_SIGNED){
        if((long long)num < 0){
            sign = '-';
            num = -num;
        }
    }
    if(sign != '-'){
        if(flags & FORMAT_PLUS){
            sign = '+';
        }else if(flags & FORMAT_SPACE){
            sign = ' ';
        }
    }
    
    bit_count = num_bit_count(num, (int)base);
    
    /* 不要轻易去malloc */
    if(bit_count > FORMAT_STACK_CACHE_SIZE){
        number_buf = eh_malloc((size_t)bit_count);
        if(number_buf == NULL)  return 0;
    }
    for(int i=(int)(bit_count-1); i>=0; i--){
        number_buf[i] = digits[num_rsh(&num, (int)base)];
    }

    if(flags&FORMAT_SPECIAL){
        if(base == BASE_TYPE_OCT){
            special = "0";
            special_count = 1;
        }else if(base == BASE_TYPE_HEX){
            special = flags & FORMAT_LARGE ? "0X":"0x";
            special_count = 2;
        }else if(base == BASE_TYPE_BIN){
            special = flags & FORMAT_LARGE ? "0B":"0b";
            special_count = 2;
        }
    }

    reality_count = ((sign) ? 1 : 0) + special_count + bit_count;

    if(precision >= 0){
        flags = (flags & ~FORMAT_ZEROPAD);
        if(precision > bit_count)
            zeropad_count = precision - bit_count;
        reality_count = reality_count + zeropad_count;
    }

    if(field_width > reality_count){
        if(flags & FORMAT_ZEROPAD){
            zeropad_count = field_width - reality_count;
        }else{
            spacepad_count = field_width - reality_count;
        }
    }

    /* 右对齐空格填充 */
    if(!(flags & FORMAT_LEFT)){
        for(int i=0; i<spacepad_count; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }

    /* 符号位 */
    if(sign){
        streamout_in_byte(stream, sign);
        n++;
    }
    /* 特殊字符 0x 0X 0 */
    for(int i=0; i<special_count; i++,n++){
        streamout_in_byte(stream, special[i]);
    }
    /* 中间0填充 */
    for(int i=0; i<zeropad_count; i++,n++){
        streamout_in_byte(stream, '0');
    }
    /* 数字有效位 */
    for(int i=0; i<bit_count; i++,n++){
        streamout_in_byte(stream, number_buf[i]);
    }
    
    /* 左对齐空格填充 */
    if(flags & FORMAT_LEFT){
        for(int i=0; i<spacepad_count; i++,n++){
            streamout_in_byte(stream, ' ');
        }
    }
    if(number_buf != _number_buf)
        eh_free(number_buf);
    return n;
}

static int eh_stream_vprintf(struct stream_out *stream, const char *fmt, va_list args){
    int n = 0;
    int flags;
    int field_width;
    int precision;
    enum base_type base;
    enum format_qualifier qualifier;
    unsigned long long num;
    double double_num;
    const char *fmt_start;
    for(; *fmt; fmt++){
        if(*fmt != '%'){
            streamout_in_byte(stream, *fmt);
            n++;
            continue;
        }
        fmt_start = fmt;
        flags = 0x00;
        while (1){
            ++fmt;
            if (*fmt == '-'){
                flags |= FORMAT_LEFT;
            }else if (*fmt == '+'){
                flags |= FORMAT_PLUS;
            }else if (*fmt == ' '){
                flags |= FORMAT_SPACE;
            }else if (*fmt == '#'){
                flags |= FORMAT_SPECIAL;
            }else if (*fmt == '0'){
                flags |= FORMAT_ZEROPAD;
            }else{
                break;
            }
        }

        field_width = -1;
        if (isdigit((int)(*fmt))){
            field_width = skip_atoi(&fmt);
        }else if (*fmt == '*'){
            ++fmt;
            field_width = va_arg(args, int);
            if (field_width < 0){
                field_width = -field_width;
                flags |= FORMAT_LEFT;
            }
        }

        precision = -1;
        if (*fmt == '.'){
            ++fmt;
            if (isdigit((int)(*fmt))){
                precision = skip_atoi(&fmt);
            }
            else if (*fmt == '*'){
                ++fmt;
                precision = va_arg(args, int);
            }
            if (precision < 0){
                precision = 0;
            }
        }

        qualifier = FORMAT_QUALIFIER_NONE;
        if(*fmt == 'h'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_SHORT;
            if(*fmt == 'h'){
                fmt++;
                qualifier = FORMAT_QUALIFIER_CHAR;
            }
        }else if(*fmt == 'l'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_LONG;
            if(*fmt == 'l'){
                fmt++;
                qualifier = FORMAT_QUALIFIER_LONG_LONG;
            }
        }else if(*fmt == 'L'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_LONG_LONG;
        }else if(*fmt == 'z'){
            fmt++;
            qualifier = FORMAT_QUALIFIER_SIZE_T;
        }

        base = BASE_TYPE_DEC;

        switch (*fmt){
            case 's':{
                n += vprintf_string(stream, va_arg(args, char *), field_width, precision, flags);
                continue;
            }
            case 'd':
                /*FALLTHROUGH*/
            case 'i':
                flags |= FORMAT_SIGNED;
            case 'u':
                goto _print_number;
            case 'X':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'x':
                base = BASE_TYPE_HEX;
                goto _print_number;
            case 'c':{
                n += vprintf_char(stream, (char)va_arg(args, int), field_width, flags);
                continue;
            }
            case '%':{
                streamout_in_byte(stream, '%');
                n++;
                continue;
            }
            case 'B':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'b':{
                base = BASE_TYPE_BIN;
                goto _print_number;
            }
            case 'o':{
                base = BASE_TYPE_OCT;
                goto _print_number;
            }
            case 'p':{
                if(field_width < 0){
                    field_width = (sizeof(void *) << 1) + 2;
                    flags |= FORMAT_ZEROPAD | FORMAT_SPECIAL;
                }
                n += vprintf_number(stream, (unsigned long)va_arg(args, void *), field_width, precision, flags, BASE_TYPE_HEX);
                continue;
            }
            case 'E':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'e':
                flags |= FORMAT_FLOAT_E;
                goto _print_folat;
            case 'G':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'g':
                flags |= FORMAT_FLOAT_G;
                goto _print_folat;
            case 'F':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'f':
                flags |= FORMAT_FLOAT_F;
                goto _print_folat;
            case 'Q':
                flags |= FORMAT_LARGE;
                /*FALLTHROUGH*/
            case 'q':
                n += vprintf_array(stream, va_arg(args, void *), field_width, precision, flags, qualifier);
                break;
            default:{
                streamout_in_byte(stream, '%');
                fmt = fmt_start;
                continue;
            }

        }
        continue;
    
    _print_number:
        {
            switch(qualifier){
                case FORMAT_QUALIFIER_LONG:{
                    /* long */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed long) : 
                        (unsigned long long)va_arg(args, unsigned long);
                    break;
                }
                case FORMAT_QUALIFIER_LONG_LONG:{
                    /* long long */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed long long) : 
                        (unsigned long long)va_arg(args, unsigned long long);
                    break;
                }
                case FORMAT_QUALIFIER_SHORT:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)((signed short)va_arg(args, int)) : 
                        (unsigned long long)((unsigned short)va_arg(args, int));
                    break;
                }
                case FORMAT_QUALIFIER_CHAR:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)((signed char)va_arg(args, int)) : 
                        (unsigned long long)((unsigned char)va_arg(args, int));
                    break;
                }
                case FORMAT_QUALIFIER_SIZE_T:{
                    /* short */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, ssize_t) : 
                        (unsigned long long)va_arg(args, size_t);
                    break;
                }
                default:{
                    /* int */
                    num = (flags & FORMAT_SIGNED) ? 
                        (unsigned long long)va_arg(args, signed int) : 
                        (unsigned long long)va_arg(args, unsigned int);
                    break;
                }
            }
            n += vprintf_number(stream, num, field_width, precision, flags, base);
            continue;
        }
    _print_folat:
        {   
            /* 目前不支持 long double */
            if(FORMAT_QUALIFIER_LONG_LONG == qualifier){
                continue;
            }
            //double_num = (qualifier == FORMAT_QUALIFIER_LONG_LONG)? va_arg(args, long double) : va_arg(args, double);
            double_num = va_arg(args, double);
            n += vprintf_float(stream, double_num, field_width, precision, flags);
            continue;
        }
    }
    return n;
}


static int eh_stream_printf(struct stream_out *stream, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_stream_vprintf(stream, fmt, args);
    va_end(args);
    return n;
}

static int eh_vsnprintf(char *buf, size_t size, const char *fmt, va_list args){
    int n;
    struct stream_out stream = {
        .type = STREAM_TYPE_MEMORY,
        .m = {
            .buf = (uint8_t*)buf,
            .pos = (uint8_t*)buf,
            .end = (uint8_t*)buf + size,
        },
    };
    n = eh_stream_vprintf(&stream, fmt, args);
    streamout_finish(&stream);
    return n;
}


int eh_snprintf(char *buf, size_t size, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_vsnprintf(buf, size, fmt, args);
    va_end(args);
    return n;
}

int eh_sprintf(char *buf, const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_vsnprintf(buf, (size_t)((char *)(LONG_MAX) - buf), fmt, args);
    va_end(args);
    return n;
}

int eh_vprintf(const char *fmt, va_list args){
    int n;
    n = eh_stream_vprintf(&_stdout, fmt, args);
    streamout_finish(&_stdout);
    return n;
}

int eh_printf(const char *fmt, ...){
    int n;
    va_list args;
    va_start(args, fmt);
    n = eh_stream_vprintf(&_stdout, fmt, args);
    va_end(args);
    streamout_finish(&_stdout);
    return n;
}

#define LOG_FILE_NAME_LEN     (sizeof("00000000_yyyymmdd_hhmmss.log") - 1)

static pthread_mutex_t         debug_mutex;
static uint8_t                 log_cache[DEBUG_CONFIG_STDOUT_MEM_CACHE_SIZE];
static FILE                 *log_fp = NULL;
static size_t                log_current_file_size = 0;
static uint32_t             log_current_file_write_num = 0;
static uint64_t                log_current_file_create_time = 0;
static size_t                 log_current_file_max_size = 0;
static int                     log_current_file_max_num = 0;
static uint64_t             log_current_file_max_interval = 0;
static char                 (*log_file_name_tab)[((LOG_FILE_NAME_LEN + 1 + sizeof(size_t) -1)/sizeof(size_t))*sizeof(size_t)] = NULL;
static char                 *log_dir_path;
static char                    *log_file_path;    /* 全路径，可供临时拼接计算 */
#define                     log_file_idx_is_used(idx)    (log_file_name_tab[idx][0] != '\0')

static void _debug_lock(void)
{
    pthread_mutex_lock(&debug_mutex);
}

static void _debug_unlock(void)
{
    pthread_mutex_unlock(&debug_mutex);
}

static uint64_t dbg_get_clock_monotonic_time_us(void){
    uint64_t microsecond;
        struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    microsecond = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
    return microsecond;
}


#include <sys/stat.h>
#include <errno.h>

static int _log_mkdir_p(const char *path) {
    char *copied_path = strdup(path);
    if (!copied_path) {
        errno = ENOMEM;
        return -1;
    }

    char *p = copied_path;

    // 处理绝对路径开头的'/'
    if (*p == '/') {
        p++;
    }

    while (*p != '\0') {
        while (*p != '/' && *p != '\0') 
            p++;

        char saved_char = *p;
        *p = '\0';

        // 创建当前层级的目录（忽略已存在的目录）
        if (mkdir(copied_path, 0755) < 0 && errno != EEXIST) {
            free(copied_path);
            return -1;
        }

        *p = saved_char;  // 恢复路径
        if (*p != '\0')
            p++;
    }

    free(copied_path);
    return 0;
}

static int _log_new_log_create(void){
    FILE *log_fp_tmp;
    uint32_t idx;
    time_t timestamp;
    struct tm tm;
    char str_time[16];
    char new_file_name[LOG_FILE_NAME_LEN + 1];

    time(&timestamp);
    localtime_r(&timestamp, &tm);
    /* yyyymmdd_hhmmss */
    strftime(str_time, sizeof(str_time), "%Y%m%d_%H%M%S", &tm);
    eh_sprintf(new_file_name, "%08x_%s.log", log_current_file_write_num, str_time);
    eh_sprintf(log_file_path, "%s/%s", log_dir_path, new_file_name);

    log_fp_tmp = fopen(log_file_path, "w");
    if(log_fp_tmp == NULL)
        return -1;
    // 设置行缓冲：遇到 \n 时自动刷新 
    setvbuf(log_fp_tmp, NULL, _IOLBF, BUFSIZ);
    if(log_fp)
        fclose(log_fp);

    idx = (uint32_t)log_current_file_write_num % (uint32_t)log_current_file_max_num;
    if(log_file_idx_is_used(idx)){
        eh_sprintf(log_file_path, "%s/%s", log_dir_path, log_file_name_tab[idx]);
        remove(log_file_path);
    }

    strcpy(log_file_name_tab[idx], new_file_name);

    log_current_file_write_num++;
    log_fp = log_fp_tmp;
    log_current_file_size = 0;
    log_current_file_create_time = dbg_get_clock_monotonic_time_us();
    return 0;
}

static void _log_fp_init(const char* log_dir, int max_log_file_num, int max_log_file_size, int max_log_interval_sec){
    DIR* dir_ptr;
    uint32_t *tmp_file_write_num_tab;
    struct dirent *dp;
    int ret;
    log_fp = NULL;

    if(log_dir == NULL || max_log_file_num <=0){
        return ;
    }
    if(_log_mkdir_p(log_dir) < 0){
        perror("mkdir log_dir fail!!");
        return ;
    }
    log_current_file_max_num = max_log_file_num;
    if(max_log_file_size <= 0){
        log_current_file_max_size = 0;    
    }else{
        log_current_file_max_size = (size_t)max_log_file_size;
    }
    

    if(max_log_interval_sec <= 0){
        log_current_file_max_interval = 0;
    }else{
        log_current_file_max_interval = (uint64_t)max_log_interval_sec * 1000 * 1000;
    }
    log_current_file_write_num = 0;
    log_dir_path = strdup(log_dir);
    if(log_dir_path == NULL){
        goto strdup_log_dir_error;
    }

    log_file_name_tab = malloc(sizeof(log_file_name_tab[0]) * (size_t)max_log_file_num);
    if(log_file_name_tab == NULL){
        goto malloc_log_file_name_tab_error;
    }
    tmp_file_write_num_tab = (uint32_t*)malloc(sizeof(uint32_t) * (size_t)max_log_file_num);
    if(tmp_file_write_num_tab == NULL){
        goto malloc_log_file_write_num_tab_error;
    }
    log_file_path = (char*)malloc(sizeof(char) * (size_t)(LOG_FILE_NAME_LEN + strlen(log_dir_path) + 2));
    if(log_file_path == NULL){
        goto malloc_log_file_path_error;
    }

    memset(log_file_name_tab, 0, sizeof(log_file_name_tab[0]) * (size_t)max_log_file_num);
    memset(tmp_file_write_num_tab, 0, sizeof(uint32_t) * (size_t)max_log_file_num);

    dir_ptr = opendir(log_dir_path);
    if(dir_ptr == NULL){
        goto opendir_error;
    }

    
    while((dp = readdir(dir_ptr))){
        uint32_t id;
        uint32_t idx;
        int n;
        if(dp->d_type != DT_REG)
            continue;
        /* 0000000a_20250417_155505.log */
        if(strlen(dp->d_name) != LOG_FILE_NAME_LEN)
            continue;
        n = sscanf(dp->d_name, "%8x_%*8d_%*6d.log", &id);
        if(n != 1)
            continue;

        if( id > log_current_file_write_num )
            log_current_file_write_num = id;
        idx = id%(uint32_t)log_current_file_max_num;

        if(!log_file_idx_is_used(idx)){
            strcpy(log_file_name_tab[idx], dp->d_name);
            tmp_file_write_num_tab[idx] = id;
            continue;
        }

        if( tmp_file_write_num_tab[idx] > id ){
            /* 删除旧日志 */
            eh_sprintf(log_file_path, "%s/%s", log_dir_path, log_file_name_tab[idx]);
            remove(log_file_path);
            strcpy(log_file_name_tab[idx], dp->d_name);
            tmp_file_write_num_tab[idx] = id;
        }else{
            /* 删除旧日志 */
            eh_sprintf(log_file_path, "%s/%s", log_dir_path, dp->d_name);
            remove(log_file_path);
        }

    }
    /* 生成新的日志 */
    log_current_file_write_num++;
    ret = _log_new_log_create();
    if(ret < 0)
        goto log_new_log_create_error;
    closedir(dir_ptr);
    free(tmp_file_write_num_tab);
    
    return ;
log_new_log_create_error:
    closedir(dir_ptr);
opendir_error:
    free(log_file_path);
malloc_log_file_path_error:
    free(tmp_file_write_num_tab);
malloc_log_file_write_num_tab_error:
    free(log_file_name_tab);
malloc_log_file_name_tab_error:
    free(log_dir_path);
strdup_log_dir_error:
    return ;
}

static void _log_fp_exit(void){
    if(log_fp == NULL)
        return ;
    fclose(log_fp);
    free(log_file_path);
    free(log_file_name_tab);
    free(log_dir_path);
}

static void _log_fp_refresh(void){
    if(log_fp == NULL)
        return ;
    if( log_current_file_max_interval > 0 && 
        (int64_t)(dbg_get_clock_monotonic_time_us() - log_current_file_create_time) > (int64_t)log_current_file_max_interval){
        _log_new_log_create();
    }

    if( log_current_file_max_size > 0 && log_current_file_size > log_current_file_max_size  ){
        _log_new_log_create();
    }
}

static void log_write(void *stream, const uint8_t *buf, size_t size){
    (void)stream;
    fwrite(buf, 1, size, stdout);
    if(log_fp){
        fwrite(buf, 1, size, log_fp);
        log_current_file_size += size;
    }
}

static struct stream_out _logout = {
    .type = STREAM_TYPE_FUNCTION,
    .f = {
        .write = log_write,
        .cache = log_cache,
        .pos = log_cache,
        .end = log_cache + DEBUG_CONFIG_STDOUT_MEM_CACHE_SIZE,
    },
};

#if (defined(DEBUG_CONFIG_DEFAULT_DEBUG_LEVEL))
static enum dbg_level dbg_level = DEBUG_CONFIG_DEFAULT_DEBUG_LEVEL;
#else
static enum dbg_level dbg_level = DBG_DEBUG;
#endif

const char *dbg_level_str[] = {
    [0]               = "U",
    [DBG_ERR]         = "E",
    [DBG_WARNING]     = "W",
    [DBG_SYS]         = "S",
    [DBG_INFO]         = "I",
    [DBG_DEBUG]     = "D",
};

int dbg_set_level(enum dbg_level level){
    dbg_level = level;
    return 0;
}

static int dbg_vprintf(enum dbg_level level, 
    enum dbg_flags flags, const char *fmt, va_list args){
    int n = 0;
    uint64_t now_usec;
    now_usec = dbg_get_clock_monotonic_time_us();
    if(flags & DBG_FLAGS_WALL_CLOCK){
        /* 打印墙上时间 */
        time_t timestamp;
        struct tm tm;
        char str_time[20];
        time(&timestamp);
        localtime_r(&timestamp, &tm);
        /* [xxxx-xx-xx xx:xx:xx] */
        strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S", &tm);
        n += eh_stream_printf(&_logout, "[%s] ", str_time);
    }
    if(flags & DBG_FLAGS_MONOTONIC_CLOCK){
        n += eh_stream_printf(&_logout, "[%5u.%06u] ", (unsigned int)(now_usec/1000000), 
            (unsigned int)(now_usec%1000000));
    }
    if(flags & DBG_FLAGS_DEBUG_TAG && level >= DBG_ERR && level <= DBG_DEBUG){
        n += eh_stream_printf(&_logout, "[%s] ", dbg_level_str[level]);
    }
    n += eh_stream_vprintf(&_logout, fmt, args);
    return n;
}

static int dbg_unlock_raw(enum dbg_level level, 
    enum dbg_flags flags, const char *fmt, ...){
    int n = 0;
    va_list args;
    va_start(args, fmt);
    n = dbg_vprintf(level, flags, fmt, args);
    va_end(args);
    return n;
}

int dbg_raw(enum dbg_level level, 
    enum dbg_flags flags, const char *fmt, ...){
    int n = 0;
    va_list args;
    if(level > dbg_level)
        return 0;
    va_start(args, fmt);
    _debug_lock();
    _log_fp_refresh();
    n = dbg_vprintf(level, flags, fmt, args);
    _debug_unlock();
    va_end(args);
    return n;
}

int vdbg_raw(enum dbg_level level, 
    enum dbg_flags flags, const char *fmt, va_list args){
    int n = 0;
    if(level > dbg_level){
        return 0;
    }
    _debug_lock();
    _log_fp_refresh();
    n = dbg_vprintf(level, flags, fmt, args);
    _debug_unlock();
    return n;
}

int dbg_hex(enum dbg_level level, 
    enum dbg_flags flags, size_t len, const void *buf){
    const uint8_t *pos = buf;
    int n = 0;
    size_t y_n, x_n;
    if(level > dbg_level)
        return 0;
    y_n = len / 16;
    x_n = len % 16;
    _debug_lock();
    _log_fp_refresh();
    n += dbg_unlock_raw(level, flags, "______________________________________________________________" DEBUG_ENTER_SIGN);
    n += dbg_unlock_raw(level, flags, "            | 0| 1| 2| 3| 4| 5| 6| 7| 8| 9| A| B| C| D| E| F||" DEBUG_ENTER_SIGN);
    n += dbg_unlock_raw(level, flags, "--------------------------------------------------------------" DEBUG_ENTER_SIGN);
    for(size_t i = 0; i < y_n; i++, pos += 16){
        n += dbg_unlock_raw(level, flags, "|0x%08x| %-47.*hhq||" DEBUG_ENTER_SIGN, 
            (unsigned int)(i*16), 16, pos);
    }
    if(x_n){
        n += dbg_unlock_raw(level, flags, "|0x%08x| %-47.*hhq||" DEBUG_ENTER_SIGN, 
                (unsigned int)(y_n*16), x_n, pos);
    }
    n += dbg_unlock_raw(level, flags, "--------------------------------------------------------------" DEBUG_ENTER_SIGN);
    _debug_unlock();
    return n;
}



int dbg_init(const char* log_dir, int max_log_file_num, int max_log_file_size, int max_log_interval_sec)
{
    _log_fp_init(log_dir, max_log_file_num, max_log_file_size, max_log_interval_sec);
    return pthread_mutex_init(&debug_mutex, NULL);
}

void dbg_exit(void)
{
    pthread_mutex_destroy(&debug_mutex);
    _log_fp_exit();
}

