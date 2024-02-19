#pragma once

enum class ExtendBits { Zero, Sign };

template <typename XLEN_t, unsigned int applied, unsigned int... slices>
static inline constexpr XLEN_t swizzleHelper(XLEN_t source_bits, XLEN_t result) {

    constexpr unsigned int numSliceInts = sizeof...(slices);
    constexpr XLEN_t values[numSliceInts] = {slices...};

    XLEN_t sliceHi = values[applied];
    XLEN_t sliceLo = values[applied+1];
    XLEN_t slice_len = sliceHi-sliceLo+1;
    result <<= slice_len;
    result |= (source_bits >> sliceLo) & (((XLEN_t)1 << slice_len) - 1);

    if constexpr (applied + 2 == numSliceInts) {
        return result;
    } else if constexpr (applied + 2 == numSliceInts - 1) {
        return result << values[applied + 2];
    } else {
        return swizzleHelper<XLEN_t, applied+2, slices...>(source_bits, result);
    }
}

template <typename XLEN_t, ExtendBits extend, unsigned int... slices>
inline constexpr XLEN_t swizzle(XLEN_t source_bits) {
    if constexpr (extend == ExtendBits::Sign) {
        constexpr XLEN_t values[sizeof...(slices)] = {slices...};
        return swizzleHelper<XLEN_t, 0, slices...>(source_bits, source_bits & ((XLEN_t)1 << values[0]) ? ~(XLEN_t)0 : 0);
    }
    return swizzleHelper<XLEN_t, 0, slices...>(source_bits, 0);
}
