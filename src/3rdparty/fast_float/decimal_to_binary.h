#ifndef FASTFLOAT_DECIMAL_TO_BINARY_H
#define FASTFLOAT_DECIMAL_TO_BINARY_H

#include <cfloat>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "fast_table.h"
#include "float_common.h"

namespace fast_float
{

// This will compute or rather approximate w * 5**q and return a pair of 64-bit
// words approximating the result, with the "high" part corresponding to the
// most significant bits and the low part corresponding to the least significant
// bits.
//
template <int bit_precision>
fastfloat_really_inline value128 compute_product_approximation(int64_t q, uint64_t w)
{
    const int index = 2 * int(q - powers::smallest_power_of_five);
    // For small values of q, e.g., q in [0,27], the answer is always exact
    // because The line value128 firstproduct = full_multiplication(w,
    // power_of_five_128[index]); gives the exact answer.
    value128 firstproduct = full_multiplication(w, powers::power_of_five_128[index]);
    static_assert((bit_precision >= 0) && (bit_precision <= 64), " precision should  be in (0,64]");
    constexpr uint64_t precision_mask = (bit_precision < 64) ?
                                            (uint64_t(0xFFFFFFFFFFFFFFFF) >> bit_precision) :
                                            uint64_t(0xFFFFFFFFFFFFFFFF);
    if ((firstproduct.high & precision_mask) == precision_mask)
    { // could further guard with  (lower + w < lower)
        // regarding the second product, we only need secondproduct.high, but our
        // expectation is that the compiler will optimize this extra work away if
        // needed.
        value128 secondproduct = full_multiplication(w, powers::power_of_five_128[index + 1]);
        firstproduct.low += secondproduct.high;
        if (secondproduct.high > firstproduct.low)
        {
            firstproduct.high++;
        }
    }
    return firstproduct;
}

namespace detail
{
/**
 * For q in (0,350), we have that
 *  f = (((152170 + 65536) * q ) >> 16);
 * is equal to
 *   floor(p) + q
 * where
 *   p = log(5**q)/log(2) = q * log(5)/log(2)
 *
 * For negative values of q in (-400,0), we have that
 *  f = (((152170 + 65536) * q ) >> 16);
 * is equal to
 *   -ceil(p) + q
 * where
 *   p = log(5**-q)/log(2) = -q * log(5)/log(2)
 */
fastfloat_really_inline int power(int q) noexcept
{
    return (((152170 + 65536) * q) >> 16) + 63;
}
} // namespace detail

// w * 10 ** q
// The returned value should be a valid ieee64 number that simply need to be
// packed. However, in some very rare cases, the computation will fail. In such
// cases, we return an adjusted_mantissa with a negative power of 2: the caller
// should recompute in such cases.
template <typename binary>
fastfloat_really_inline adjusted_mantissa compute_float(int64_t q, uint64_t w) noexcept
{
    adjusted_mantissa answer;
    if ((w == 0) || (q < binary::smallest_power_of_ten()))
    {
        answer.power2 = 0;
        answer.mantissa = 0;
        // result should be zero
        return answer;
    }
    if (q > binary::largest_power_of_ten())
    {
        // we want to get infinity:
        answer.power2 = binary::infinite_power();
        answer.mantissa = 0;
        return answer;
    }
    // At this point in time q is in [powers::smallest_power_of_five,
    // powers::largest_power_of_five].

    // We want the most significant bit of i to be 1. Shift if needed.
    int lz = leading_zeroes(w);
    w <<= lz;

    // The required precision is binary::mantissa_explicit_bits() + 3 because
    // 1. We need the implicit bit
    // 2. We need an extra bit for rounding purposes
    // 3. We might lose a bit due to the "upperbit" routine (result too small,
    // requiring a shift)

    value128 product = compute_product_approximation<binary::mantissa_explicit_bits() + 3>(q, w);
    if (product.low == 0xFFFFFFFFFFFFFFFF)
    { //  could guard it further
        // In some very rare cases, this could happen, in which case we might need a
        // more accurate computation that what we can provide cheaply. This is very,
        // very unlikely.
        //
        const bool inside_safe_exponent =
            (q >= -27) && (q <= 55); // always good because 5**q <2**128 when q>=0,
        // and otherwise, for q<0, we have 5**-q<2**64 and the 128-bit reciprocal
        // allows for exact computation.
        if (!inside_safe_exponent)
        {
            answer.power2 = -1; // This (a negative value) indicates an error condition.
            return answer;
        }
    }
    // The "compute_product_approximation" function can be slightly slower than a
    // branchless approach: value128 product = compute_product(q, w); but in
    // practice, we can win big with the compute_product_approximation if its
    // additional branch is easily predicted. Which is best is data specific.
    int upperbit = int(product.high >> 63);

    answer.mantissa = product.high >> (upperbit + 64 - binary::mantissa_explicit_bits() - 3);

    answer.power2 = int(detail::power(int(q)) + upperbit - lz - binary::minimum_exponent());
    if (answer.power2 <= 0)
    { // we have a subnormal?
        // Here have that answer.power2 <= 0 so -answer.power2 >= 0
        if (-answer.power2 + 1 >= 64)
        { // if we have more than 64 bits below the minimum exponent, you
          // have a zero for sure.
            answer.power2 = 0;
            answer.mantissa = 0;
            // result should be zero
            return answer;
        }
        // next line is safe because -answer.power2 + 1 < 64
        answer.mantissa >>= -answer.power2 + 1;
        // Thankfully, we can't have both "round-to-even" and subnormals because
        // "round-to-even" only occurs for powers close to 0.
        answer.mantissa += (answer.mantissa & 1); // round up
        answer.mantissa >>= 1;
        // There is a weird scenario where we don't have a subnormal but just.
        // Suppose we start with 2.2250738585072013e-308, we end up
        // with 0x3fffffffffffff x 2^-1023-53 which is technically subnormal
        // whereas 0x40000000000000 x 2^-1023-53  is normal. Now, we need to round
        // up 0x3fffffffffffff x 2^-1023-53  and once we do, we are no longer
        // subnormal, but we can only know this after rounding.
        // So we only declare a subnormal if we are smaller than the threshold.
        answer.power2 =
            (answer.mantissa < (uint64_t(1) << binary::mantissa_explicit_bits())) ? 0 : 1;
        return answer;
    }

    // usually, we round *up*, but if we fall right in between and and we have an
    // even basis, we need to round down
    // We are only concerned with the cases where 5**q fits in single 64-bit word.
    if ((product.low <= 1) && (q >= binary::min_exponent_round_to_even()) &&
        (q <= binary::max_exponent_round_to_even()) && ((answer.mantissa & 3) == 1))
    { // we may fall between two floats!
        // To be in-between two floats we need that in doing
        //   answer.mantissa = product.high >> (upperbit + 64 -
        //   binary::mantissa_explicit_bits() - 3);
        // ... we dropped out only zeroes. But if this happened, then we can go
        // back!!!
        if ((answer.mantissa << (upperbit + 64 - binary::mantissa_explicit_bits() - 3)) ==
            product.high)
        {
            answer.mantissa &= ~uint64_t(1); // flip it so that we do not round up
        }
    }

    answer.mantissa += (answer.mantissa & 1); // round up
    answer.mantissa >>= 1;
    if (answer.mantissa >= (uint64_t(2) << binary::mantissa_explicit_bits()))
    {
        answer.mantissa = (uint64_t(1) << binary::mantissa_explicit_bits());
        answer.power2++; // undo previous addition
    }

    answer.mantissa &= ~(uint64_t(1) << binary::mantissa_explicit_bits());
    if (answer.power2 >= binary::infinite_power())
    { // infinity
        answer.power2 = binary::infinite_power();
        answer.mantissa = 0;
    }
    return answer;
}

} // namespace fast_float

#endif
