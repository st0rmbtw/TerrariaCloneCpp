#pragma once

#ifndef MATH_CONSTEXPR_MATH_HPP
#define MATH_CONSTEXPR_MATH_HPP

#include <type_traits>
#include <limits>

// https://github.com/kthohr/gcem
namespace internal
{

#ifndef GCEM_LOG_2
    #define GCEM_LOG_2 0.6931471805599453094172321214581765680755L
#endif

#ifndef GCEM_LOG_10
    #define GCEM_LOG_10 2.3025850929940456840179914546843642076011L
#endif

#ifndef GCEM_PI
    #define GCEM_PI 3.1415926535897932384626433832795028841972L
#endif

#ifndef GCEM_LOG_PI
    #define GCEM_LOG_PI 1.1447298858494001741434273513530587116473L
#endif

#ifndef GCEM_LOG_2PI
    #define GCEM_LOG_2PI 1.8378770664093454835606594728112352797228L
#endif

#ifndef GCEM_LOG_SQRT_2PI
    #define GCEM_LOG_SQRT_2PI 0.9189385332046727417803297364056176398614L
#endif

#ifndef GCEM_SQRT_2
    #define GCEM_SQRT_2 1.4142135623730950488016887242096980785697L
#endif

#ifndef GCEM_HALF_PI
    #define GCEM_HALF_PI 1.5707963267948966192313216916397514420986L
#endif

#ifndef GCEM_SQRT_PI
    #define GCEM_SQRT_PI 1.7724538509055160272981674833411451827975L
#endif

#ifndef GCEM_SQRT_HALF_PI
    #define GCEM_SQRT_HALF_PI 1.2533141373155002512078826424055226265035L
#endif

#ifndef GCEM_E
    #define GCEM_E 2.7182818284590452353602874713526624977572L
#endif

#ifndef GCEM_ERF_MAX_ITER
    #define GCEM_ERF_MAX_ITER 60
#endif

#ifndef GCEM_ERF_INV_MAX_ITER
    #define GCEM_ERF_INV_MAX_ITER 60
#endif

#ifndef GCEM_EXP_MAX_ITER_SMALL
    #define GCEM_EXP_MAX_ITER_SMALL 25
#endif

// #ifndef GCEM_LOG_TOL
//     #define GCEM_LOG_TOL 1E-14
// #endif

#ifndef GCEM_LOG_MAX_ITER_SMALL
    #define GCEM_LOG_MAX_ITER_SMALL 25
#endif

#ifndef GCEM_LOG_MAX_ITER_BIG
    #define GCEM_LOG_MAX_ITER_BIG 255
#endif

#ifndef GCEM_INCML_BETA_TOL
    #define GCEM_INCML_BETA_TOL 1E-15
#endif

#ifndef GCEM_INCML_BETA_MAX_ITER
    #define GCEM_INCML_BETA_MAX_ITER 205
#endif

#ifndef GCEM_INCML_BETA_INV_MAX_ITER
    #define GCEM_INCML_BETA_INV_MAX_ITER 35
#endif

#ifndef GCEM_INCML_GAMMA_MAX_ITER
    #define GCEM_INCML_GAMMA_MAX_ITER 55
#endif

#ifndef GCEM_INCML_GAMMA_INV_MAX_ITER
    #define GCEM_INCML_GAMMA_INV_MAX_ITER 35
#endif

#ifndef GCEM_SQRT_MAX_ITER
    #define GCEM_SQRT_MAX_ITER 100
#endif

#ifndef GCEM_INV_SQRT_MAX_ITER
    #define GCEM_INV_SQRT_MAX_ITER 100
#endif

#ifndef GCEM_TAN_MAX_ITER
    #define GCEM_TAN_MAX_ITER 35
#endif

#ifndef GCEM_TANH_MAX_ITER
    #define GCEM_TANH_MAX_ITER 35
#endif

template<typename T>
using return_t = typename std::conditional<std::is_integral<T>::value,double,T>::type;

template<class T>
using GCLIM = std::numeric_limits<T>;

using llint_t = long long int;

template<typename T>
constexpr
T
abs(const T x)
noexcept
{
    return( // deal with signed-zeros
            x == T(0) ? \
                T(0) :
            // else
            x < T(0) ? \
                - x : x );
}

template<typename T>
constexpr
bool
is_nan(const T x)
noexcept
{
    return x != x;
}

template<typename T>
constexpr
T
mantissa(const T x)
noexcept
{
    return( x < T(1) ? \
                mantissa(x * T(10)) : 
            x > T(10) ? \
                mantissa(x / T(10)) :
            // else
                x );
}

template<typename T>
constexpr
llint_t
find_exponent(const T x, const llint_t exponent)
noexcept
{
    return( // < 1
            x < T(1e-03)  ? \
                find_exponent(x * T(1e+04), exponent - llint_t(4)) :
            x < T(1e-01)  ? \
                find_exponent(x * T(1e+02), exponent - llint_t(2)) :
            x < T(1)  ? \
                find_exponent(x * T(10), exponent - llint_t(1)) :
            // > 10
            x > T(10) ? \
                find_exponent(x / T(10), exponent + llint_t(1)) :
            x > T(1e+02) ? \
                find_exponent(x / T(1e+02), exponent + llint_t(2)) :
            x > T(1e+04) ? \
                find_exponent(x / T(1e+04), exponent + llint_t(4)) :
            // else
                exponent );
}

// continued fraction seems to be a better approximation for small x
// see http://functions.wolfram.com/ElementaryFunctions/Log/10/0005/

#if __cplusplus >= 201402L // C++14 version

template<typename T>
constexpr
T
log_cf_main(const T xx, const int depth_end)
noexcept
{
    int depth = GCEM_LOG_MAX_ITER_SMALL - 1;
    T res = T(2*(depth+1) - 1);

    while (depth > depth_end - 1) {
        res = T(2*depth - 1) - T(depth*depth) * xx / res;

        --depth;
    }

    return res;
}

#else // C++11 version

template<typename T>
constexpr
T
log_cf_main(const T xx, const int depth)
noexcept
{
    return( depth < GCEM_LOG_MAX_ITER_SMALL ? \
            // if 
                T(2*depth - 1) - T(depth*depth) * xx / log_cf_main(xx,depth+1) :
            // else 
                T(2*depth - 1) );
}

#endif

template<typename T>
constexpr
T
log_cf_begin(const T x)
noexcept
{ 
    return( T(2)*x/log_cf_main(x*x,1) );
}

template<typename T>
constexpr
T
log_main(const T x)
noexcept
{ 
    return( log_cf_begin((x - T(1))/(x + T(1))) );
}

constexpr
long double
log_mantissa_integer(const int x)
noexcept
{
    return( x == 2  ? 0.6931471805599453094172321214581765680755L :
            x == 3  ? 1.0986122886681096913952452369225257046475L :
            x == 4  ? 1.3862943611198906188344642429163531361510L :
            x == 5  ? 1.6094379124341003746007593332261876395256L :
            x == 6  ? 1.7917594692280550008124773583807022727230L :
            x == 7  ? 1.9459101490553133051053527434431797296371L :
            x == 8  ? 2.0794415416798359282516963643745297042265L :
            x == 9  ? 2.1972245773362193827904904738450514092950L :
            x == 10 ? 2.3025850929940456840179914546843642076011L :
                      0.0L );
}

template<typename T>
constexpr
T
log_mantissa(const T x)
noexcept
{   // divide by the integer part of x, which will be in [1,10], then adjust using tables
    return( log_main(x/T(static_cast<int>(x))) + T(log_mantissa_integer(static_cast<int>(x))) );
}

template<typename T>
constexpr
T
log_breakup(const T x)
noexcept
{   // x = a*b, where b = 10^c
    return( log_mantissa(mantissa(x)) + T(GCEM_LOG_10) * T(find_exponent(x,0)) );
}

template<typename T>
constexpr
T
log_check(const T x)
noexcept
{
    return( is_nan(x) ? \
                GCLIM<T>::quiet_NaN() :
            // x < 0
            x < T(0) ? \
                GCLIM<T>::quiet_NaN() :
            // x ~= 0
            GCLIM<T>::min() > x ? \
                - GCLIM<T>::infinity() :
            // indistinguishable from 1
            GCLIM<T>::min() > abs(x - T(1)) ? \
                T(0) : 
            // 
            x == GCLIM<T>::infinity() ? \
                GCLIM<T>::infinity() :
            // else
                (x < T(0.5) || x > T(1.5)) ?
                // if 
                    log_breakup(x) :
                // else
                    log_main(x) );
}

template<typename T>
constexpr
return_t<T>
log_integral_check(const T x)
noexcept
{
    return( std::is_integral<T>::value ? \
                x == T(0) ? \
                    - GCLIM<return_t<T>>::infinity() :
                x > T(1) ? \
                    log_check( static_cast<return_t<T>>(x) ) :
                    static_cast<return_t<T>>(0) :
            log_check( static_cast<return_t<T>>(x) ) );
}

}

/**
 * Compile-time natural logarithm function
 *
 * @param x a real-valued input.
 * @return \f$ \log_e(x) \f$ using \f[ \log\left(\frac{1+x}{1-x}\right) = \dfrac{2x}{1-\dfrac{x^2}{3-\dfrac{4x^2}{5 - \dfrac{9x^3}{7 - \ddots}}}}, \ \ x \in [-1,1] \f] 
 * The continued fraction argument is split into two parts: \f$ x = a \times 10^c \f$, where \f$ c \f$ is an integer.
 */

namespace gcem {
    template<typename T>
    constexpr internal::return_t<T> log(const T x) noexcept {
        return internal::log_integral_check( x );
    }

    constexpr int32_t ceil(float num)
    {
        return (static_cast<float>(static_cast<int32_t>(num)) == num)
            ? static_cast<int32_t>(num)
            : static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
    }
}

#endif