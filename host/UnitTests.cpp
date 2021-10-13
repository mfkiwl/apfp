#include <ap_int.h>

#include <catch.hpp>
#include <iostream>
#include <limits>

#include "ArithmeticOperations.h"
#include "Karatsuba.h"
#include "PackedFloat.h"
#include "Random.h"

constexpr auto kNumRandom = 128;

TEST_CASE("PackedFloat to/from GMP Conversion") {
    // Simple example
    {
        mpf_t gmp_num;
        mpf_init2(gmp_num, 8 * sizeof(Mantissa));
        REQUIRE(mpf_get_prec(gmp_num) >= 8 * sizeof(Mantissa));
        mpf_set_si(gmp_num, -42);
        REQUIRE(gmp_num->_mp_exp == 1);
        REQUIRE(gmp_num->_mp_size == -1);
        REQUIRE(gmp_num->_mp_d[0] == 42);
        PackedFloat num(gmp_num);
        REQUIRE(num.sign == 1);
        REQUIRE(num.exponent == 1);
        REQUIRE(num.mantissa[0] == 42);
        for (int i = 1; i < kMantissaBytes; ++i) {
            REQUIRE(num.mantissa[i] == 0);
        }
        num.ToGmp(gmp_num);
        REQUIRE(gmp_num->_mp_exp == 1);
        REQUIRE(gmp_num->_mp_size == -1);
        REQUIRE(gmp_num->_mp_d[0] == 42);
        for (int i = 1; i < gmp_num->_mp_size; ++i) {
            REQUIRE(gmp_num->_mp_d[i] == 0);
        };
        mpf_clear(gmp_num);
    }
    // Run some random numbers
    {
        auto rng = RandomNumberGenerator();
        mpf_t gmp_num_a, gmp_num_b;
        PackedFloat num;
        mpf_init2(gmp_num_a, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_b, 8 * sizeof(Mantissa));
        for (int i = 0; i < kNumRandom; ++i) {
            rng.Generate(gmp_num_a);
            const int gmp_bytes = sizeof(mp_limb_t) * std::abs(gmp_num_a->_mp_size);
            const size_t offset = std::max(gmp_bytes - kMantissaBytes, 0);
            num = gmp_num_a;
            num.ToGmp(gmp_num_b);
            REQUIRE(gmp_num_b->_mp_exp == gmp_num_a->_mp_exp);
            REQUIRE(gmp_num_b->_mp_size == gmp_num_a->_mp_size);
            REQUIRE(std::memcmp(gmp_num_a->_mp_d, reinterpret_cast<uint8_t const *>(gmp_num_b->_mp_d) + offset,
                                std::min(gmp_bytes, kMantissaBytes)) == 0);
            REQUIRE(PackedFloat(gmp_num_b) == num);
        }
    }
}

TEST_CASE("PackedFloat to/from MPFR Conversion") {
    {
        mpfr_t mpfr_num_a, mpfr_num_b;
        mpfr_init2(mpfr_num_a, 8 * sizeof(Mantissa));
        mpfr_init2(mpfr_num_b, 8 * sizeof(Mantissa));
        REQUIRE(mpfr_get_prec(mpfr_num_a) >= mpfr_prec_t(8 * sizeof(Mantissa)));
        mpfr_set_si(mpfr_num_a, -42, kRoundingMode);
        PackedFloat num(mpfr_num_a);
        REQUIRE(num.sign == 1);
        num.ToMpfr(mpfr_num_b);
        REQUIRE(mpfr_num_a->_mpfr_exp == mpfr_num_a->_mpfr_exp);
        REQUIRE(mpfr_num_a->_mpfr_sign == mpfr_num_a->_mpfr_sign);
        REQUIRE(std::memcmp(mpfr_num_a->_mpfr_d, mpfr_num_b->_mpfr_d, kMantissaBytes) == 0);
        mpfr_clear(mpfr_num_a);
        mpfr_clear(mpfr_num_b);
    }
    // Run some random numbers
    {
        auto rng = RandomNumberGenerator();
        mpfr_t mpfr_num_a, mpfr_num_b;
        PackedFloat num;
        mpfr_init2(mpfr_num_a, 8 * sizeof(Mantissa));
        mpfr_init2(mpfr_num_b, 8 * sizeof(Mantissa));
        for (int i = 0; i < kNumRandom; ++i) {
            rng.Generate(mpfr_num_a);
            const int mpfr_bytes = std::abs(mpfr_num_a->_mpfr_prec) / 8;
            const size_t offset = std::max(mpfr_bytes - kMantissaBytes, 0);
            num = mpfr_num_a;
            num.ToMpfr(mpfr_num_b);
            REQUIRE(mpfr_num_b->_mpfr_exp == mpfr_num_a->_mpfr_exp);
            REQUIRE(std::memcmp(reinterpret_cast<uint8_t const *>(mpfr_num_a->_mpfr_d) + offset, mpfr_num_b->_mpfr_d,
                                std::min(mpfr_bytes, kMantissaBytes)) == 0);
            REQUIRE(PackedFloat(mpfr_num_b) == num);
        }
    }
}

template <int bits>
ap_uint<2 * bits> MultOverflow(ap_uint<bits> const &a, ap_uint<bits> const &b) {
    return ap_uint<2 * bits>(a) * ap_uint<2 * bits>(b);
}

TEST_CASE("Karatsuba") {
    {
        ap_uint<kMantissaBits> a, b;
        a = 1;
        b = 1;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = 0;
        b = 1;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = 0;
        b = 0;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = -1;
        b = 1;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = 12345;
        b = 67890;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = 1234567890;
        b = 6789012345;
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        a = std::numeric_limits<uint64_t>::max();
        b = std::numeric_limits<uint64_t>::max();
        REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
    }

    {
        auto rng = RandomNumberGenerator();
        for (int i = 0; i < kNumRandom; ++i) {
            const auto _a = rng.Generate();
            const auto _b = rng.Generate();
            ap_uint<kMantissaBits> a, b;
            for (int j = 0; j < kMantissaBytes; ++j) {
                a.range((j + 1) * 8 - 1, j * 8) = _a.mantissa[j];
                b.range((j + 1) * 8 - 1, j * 8) = _b.mantissa[j];
            }
            REQUIRE(MultOverflow(a, b) == Karatsuba(a, b));
        }
    }
}

TEST_CASE("Add GMP") {
    {
        mpf_t gmp_a, gmp_b, gmp_c;
        mpf_init2(gmp_a, kMantissaBits);
        mpf_init2(gmp_b, kMantissaBits);
        mpf_init2(gmp_c, kMantissaBits);

        auto add = [&gmp_a, &gmp_b, &gmp_c](int64_t a, int64_t b) {
            mpf_set_si(gmp_a, a);
            mpf_set_si(gmp_b, b);
            mpf_add(gmp_c, gmp_a, gmp_b);
        };

        add(1, 0);
        REQUIRE(Add(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        add(1, 1);
        REQUIRE(Add(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        add(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
        REQUIRE(Add(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        add(std::numeric_limits<int64_t>::max(), 1);
        REQUIRE(Add(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));

        mpf_clear(gmp_a);
        mpf_clear(gmp_b);
        mpf_clear(gmp_c);
    }

    {
        auto rng = RandomNumberGenerator();
        mpf_t gmp_num_a, gmp_num_b, gmp_num_c;
        PackedFloat num;
        mpf_init2(gmp_num_a, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_b, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_c, 8 * sizeof(Mantissa));
        for (int i = 0; i < kNumRandom; ++i) {
            rng.Generate(gmp_num_a);
            rng.Generate(gmp_num_b);
            mpf_add(gmp_num_c, gmp_num_a, gmp_num_b);
            REQUIRE(PackedFloat(gmp_num_c) == Add(PackedFloat(gmp_num_a), PackedFloat(gmp_num_b)));
        }
    }
}

TEST_CASE("Add MPFR") {
    auto rng = RandomNumberGenerator();
    mpfr_t mpfr_num_a, mpfr_num_b, mpfr_num_c;
    PackedFloat num;
    mpfr_init2(mpfr_num_a, 8 * sizeof(Mantissa));
    mpfr_init2(mpfr_num_b, 8 * sizeof(Mantissa));
    mpfr_init2(mpfr_num_c, 8 * sizeof(Mantissa));
    for (int i = 0; i < kNumRandom; ++i) {
        rng.Generate(mpfr_num_a);
        rng.Generate(mpfr_num_b);
        mpfr_add(mpfr_num_c, mpfr_num_a, mpfr_num_b, kRoundingMode);
        REQUIRE(PackedFloat(mpfr_num_c) == Add(PackedFloat(mpfr_num_a), PackedFloat(mpfr_num_b)));
    }
}

#ifdef APFP_GMP_SEMANTICS

TEST_CASE("Multiply GMP") {
    {
        mpf_t gmp_a, gmp_b, gmp_c;
        mpf_init2(gmp_a, kMantissaBits);
        mpf_init2(gmp_b, kMantissaBits);
        mpf_init2(gmp_c, kMantissaBits);

        auto multiply = [&gmp_a, &gmp_b, &gmp_c](int64_t a, int64_t b) {
            mpf_set_si(gmp_a, a);
            mpf_set_si(gmp_b, b);
            mpf_mul(gmp_c, gmp_a, gmp_b);
        };

        multiply(1, 0);
        REQUIRE(Multiply(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        multiply(1, 1);
        REQUIRE(Multiply(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        multiply(std::numeric_limits<int64_t>::max(), 1);
        REQUIRE(Multiply(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));
        multiply(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max());
        REQUIRE(Multiply(PackedFloat(gmp_a), PackedFloat(gmp_b)) == PackedFloat(gmp_c));

        mpf_clear(gmp_a);
        mpf_clear(gmp_b);
        mpf_clear(gmp_c);
    }
    {
        auto rng = RandomNumberGenerator();
        mpf_t gmp_num_a, gmp_num_b, gmp_num_c;
        PackedFloat num;
        mpf_init2(gmp_num_a, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_b, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_c, 8 * sizeof(Mantissa));
        for (int i = 0; i < kNumRandom; ++i) {
            rng.Generate(gmp_num_a);
            rng.Generate(gmp_num_b);
            mpf_mul(gmp_num_c, gmp_num_a, gmp_num_b);
            REQUIRE(PackedFloat(gmp_num_c) == Add(PackedFloat(gmp_num_a), PackedFloat(gmp_num_b)));
        }
    }
}

TEST_CASE("MultiplyAccumulate") {
    {
        mpf_t gmp_a, gmp_b, gmp_c, gmp_tmp;
        mpf_init2(gmp_a, kMantissaBits);
        mpf_init2(gmp_b, kMantissaBits);
        mpf_init2(gmp_c, kMantissaBits);
        mpf_init2(gmp_tmp, kMantissaBits);

        auto multiply_accumulate = [&gmp_a, &gmp_b, &gmp_c, &gmp_tmp](int64_t a, int64_t b, int64_t c) {
            mpf_set_si(gmp_a, a);
            mpf_set_si(gmp_b, b);
            mpf_set_si(gmp_c, c);
            mpf_mul(gmp_tmp, gmp_a, gmp_b);
            mpf_add(gmp_tmp, gmp_tmp, gmp_c);
        };

        multiply_accumulate(1, 1, 0);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(1, 2, 0);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(1, 1, 1);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(1, 2, 1);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(std::numeric_limits<int64_t>::max(), 1, 0);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(std::numeric_limits<int64_t>::max(), 1, std::numeric_limits<int64_t>::max());
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));

        multiply_accumulate(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(), 0);
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));
        multiply_accumulate(std::numeric_limits<int64_t>::max(), std::numeric_limits<int64_t>::max(),
                            std::numeric_limits<int64_t>::max());
        REQUIRE(MultiplyAccumulate(PackedFloat(gmp_a), PackedFloat(gmp_b), PackedFloat(gmp_c)) == PackedFloat(gmp_tmp));

        mpf_clear(gmp_a);
        mpf_clear(gmp_b);
        mpf_clear(gmp_c);
        mpf_clear(gmp_tmp);
    }
    {
        auto rng = RandomNumberGenerator();
        mpf_t gmp_num_a, gmp_num_b, gmp_num_c, gmp_num_tmp;
        mpf_init2(gmp_num_a, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_b, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_c, 8 * sizeof(Mantissa));
        mpf_init2(gmp_num_tmp, 8 * sizeof(Mantissa));
        for (int i = 0; i < kNumRandom; ++i) {
            rng.Generate(gmp_num_a);
            rng.Generate(gmp_num_b);
            rng.Generate(gmp_num_c);
            mpf_mul(gmp_num_tmp, gmp_num_a, gmp_num_b);
            mpf_add(gmp_num_c, gmp_num_c, gmp_num_tmp);
            REQUIRE(PackedFloat(gmp_num_c) ==
                    MultiplyAccumulate(PackedFloat(gmp_num_a), PackedFloat(gmp_num_b), PackedFloat(gmp_num_c)));
        }
    }
}

#else

TEST_CASE("Multiply MPFR") {
    auto rng = RandomNumberGenerator();
    mpfr_t gmp_num_a, gmp_num_b, gmp_num_c;
    PackedFloat num;
    mpfr_init2(gmp_num_a, 8 * sizeof(Mantissa));
    mpfr_init2(gmp_num_b, 8 * sizeof(Mantissa));
    mpfr_init2(gmp_num_c, 8 * sizeof(Mantissa));
    for (int i = 0; i < kNumRandom; ++i) {
        rng.Generate(gmp_num_a);
        rng.Generate(gmp_num_b);
        mpfr_mul(gmp_num_c, gmp_num_a, gmp_num_b, kRoundingMode);
        REQUIRE(PackedFloat(gmp_num_c) == Add(PackedFloat(gmp_num_a), PackedFloat(gmp_num_b)));
    }
}

TEST_CASE("MultiplyAccumulate") {
    auto rng = RandomNumberGenerator();
    mpfr_t gmp_num_a, gmp_num_b, gmp_num_c, gmp_num_tmp;
    mpfr_init2(gmp_num_a, 8 * sizeof(Mantissa));
    mpfr_init2(gmp_num_b, 8 * sizeof(Mantissa));
    mpfr_init2(gmp_num_c, 8 * sizeof(Mantissa));
    mpfr_init2(gmp_num_tmp, 8 * sizeof(Mantissa));
    for (int i = 0; i < kNumRandom; ++i) {
        rng.Generate(gmp_num_a);
        rng.Generate(gmp_num_b);
        rng.Generate(gmp_num_c);
        mpfr_mul(gmp_num_tmp, gmp_num_a, gmp_num_b, kRoundingMode);
        mpfr_add(gmp_num_c, gmp_num_c, gmp_num_tmp, kRoundingMode);
        REQUIRE(PackedFloat(gmp_num_c) ==
                MultiplyAccumulate(PackedFloat(gmp_num_a), PackedFloat(gmp_num_b), PackedFloat(gmp_num_c)));
    }
}

#endif
