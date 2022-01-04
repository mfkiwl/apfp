#include "Karatsuba.h"

#include <type_traits>  // std::enable_if

constexpr int AddLatency(int bits) {
    // 4 is the maximum supported latency of integer adds using the BIND_OP pragma
    return (bits >= 512) ? 4 : (bits >= 384) ? 3 : (bits >= 256) ? 2 : (bits >= 128) ? 1 : 0;
}

template <int bits>
auto _Karatsuba(ap_uint<bits> const &a, ap_uint<bits> const &b) ->
    typename std::enable_if<(bits > kMultBaseBits), ap_uint<2 * bits>>::type {
#pragma HLS INLINE
    static_assert(bits % 2 == 0, "Number of bits must be even.");
    using Full = ap_uint<bits>;
    using Half = ap_uint<bits / 2>;

    // Decompose input operands into halves for the recursive step
    Half a0 = a.range(bits / 2 - 1, 0);
    Half a1 = a.range(bits - 1, bits / 2);
    Half b0 = b.range(bits / 2 - 1, 0);
    Half b1 = b.range(bits - 1, bits / 2);

    // Recurse on a_0 * b_0 and a_1 * b_1
    Full z0 = _Karatsuba<bits / 2>(a0, b0);
    Full z2 = _Karatsuba<bits / 2>(a1, b1);

    // Compute |a_0 - a_1| and sign(a_0 - a_1)
    bool a0a1_is_neg = a0 < a1;
    Half a0a1 = a0a1_is_neg ? (a1 - a0) : (a0 - a1);
#pragma HLS BIND_OP variable = a0a1 op = sub impl = fabric latency = AddLatency(bits / 2)
    // Compute |b_1 - b_0| and sign(b_1 - b_0)
    bool b0b1_is_neg = b1 < b0;
    Half b0b1 = b0b1_is_neg ? (b0 - b1) : (b1 - b0);
#pragma HLS BIND_OP variable = b0b1 op = sub impl = fabric latency = AddLatency(bits / 2)

    // XOR the two signs to get the final sign
    bool a0a1b0b1_is_neg = a0a1_is_neg != b0b1_is_neg;
    // Recurse on |a_0 - a_1| * |b_0 - b_1|
    Full a0a1b0b1 = _Karatsuba<bits / 2>(a0a1, b0b1);
    ap_int<bits + 2> a0a1b0b1_signed = a0a1b0b1_is_neg ? -ap_int<bits + 1>(a0a1b0b1) : ap_int<bits + 2>(a0a1b0b1);
    ap_uint<bits + 2> z1 = ap_uint<bits + 2>(a0a1b0b1_signed) + z0 + z2;
#pragma HLS BIND_OP variable = z1 op = add impl = fabric latency = AddLatency(bits + 2)

    // Align everything and combine
    ap_uint<(2 * bits)> z0z2 = z0 | (ap_uint<(2 * bits)>(z2) << bits);
    ap_uint<(bits + 2 + bits / 2)> z1_aligned = ap_uint<(bits + 2 + bits / 2)>(z1) << (bits / 2);
    ap_uint<(2 * bits)> z;
    // Workaround to avoid HLS padding an extra bit for the add. This is necessary to support 2048 bit multiplication,
    // which required adding two 4096 numbers, because 4096 bits is the maximum width support by the ap_uint type.
    z.V = z1_aligned.V + z0z2.V;
#pragma HLS BIND_OP variable = z.V op = add impl = fabric latency = AddLatency(2 * bits)

    return z;
}

// Bottom out using SFINAE when the bit width is lower or equal to the specified base number of bits
template <int bits>
auto _Karatsuba(ap_uint<bits> const &a, ap_uint<bits> const &b) ->
    typename std::enable_if<(bits <= kMultBaseBits), ap_uint<2 * bits>>::type {
#pragma HLS INLINE
    return a * b;
}

ap_uint<2 * kBits> Karatsuba(ap_uint<kBits> const &a, ap_uint<kBits> const &b) {
#pragma HLS INLINE
    return _Karatsuba<kBits>(a, b);
}
