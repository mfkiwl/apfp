#pragma once
#ifdef APFP_GMP_INTERFACE_TYPE // Interface with GMP types
#include <gmp.h>
using ApfpInterfaceType = mpf_t;
using ApfpInterfaceTypePtr = mpf_ptr;
using ApfpInterfaceTypeConstPtr = mpf_srcptr;

#else
#include <mpfr.h>
using ApfpInterfaceType = mpfr_t;
using ApfpInterfaceTypePtr = mpfr_ptr;
using ApfpInterfaceTypeConstPtr = mpfr_srcptr;
#endif

void InitApfpInterfaceType(ApfpInterfaceTypePtr value);

void Init2ApfpInterfaceType(ApfpInterfaceTypePtr value, unsigned long precision);

void ClearApfpInterfaceType(ApfpInterfaceTypePtr value);

void SwapApfpInterfaceType(ApfpInterfaceTypePtr a, ApfpInterfaceTypePtr b);

void SetApfpInterfaceType(ApfpInterfaceTypePtr dest, ApfpInterfaceTypeConstPtr source);

void SetApfpInterfaceType(ApfpInterfaceTypePtr dest, long int source);

void AddApfpInterfaceType(ApfpInterfaceTypeConstPtr a, ApfpInterfaceTypeConstPtr b, ApfpInterfaceTypePtr dest);

void MulApfpInterfaceType(ApfpInterfaceTypeConstPtr a, ApfpInterfaceTypeConstPtr b, ApfpInterfaceTypePtr dest);

/// Smart pointer-like wrapper class for GMP/MPFR types
class ApfpInterfaceWrapper {
    ApfpInterfaceType data_;

public:
    ~ApfpInterfaceWrapper();

    ApfpInterfaceWrapper();

    ApfpInterfaceWrapper(unsigned long precision);

    ApfpInterfaceWrapper(ApfpInterfaceWrapper&&);

    ApfpInterfaceWrapper(ApfpInterfaceWrapper&) = delete;

    ApfpInterfaceWrapper& operator=(const ApfpInterfaceWrapper&) = delete;

    ApfpInterfaceWrapper& operator=(ApfpInterfaceWrapper&&);
    
    // This decays to the pointer type
    ApfpInterfaceTypePtr get() { return data_; }

    ApfpInterfaceTypeConstPtr get() const { return data_; }
};