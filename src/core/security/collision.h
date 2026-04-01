#ifndef IDEAM_CORE_COLLISION_H
#define IDEAM_CORE_COLLISION_H

#include <cstdint>
#include <cstddef>
#include <algorithm>

// --- SIMD Detection ---
#if defined(__AVX2__)
    #include <immintrin.h>
    #define IDEAM_SIMD_AVX2
#elif defined(__SSE4_1__) || defined(__SSE2__) || defined(_M_AMD64) || defined(_M_IX86)
    #include <immintrin.h>
    #define IDEAM_SIMD_SSE
#endif

namespace ideam::core {

/**
 * CollisionUtils
 * High-performance bitset operations for the DOD Memory Manager.
 * Optimized for raw pointer access to avoid std::vector indirection.
 */
struct CollisionUtils {

    /**
     * has_intersection
     * Returns true if any bit is set in both p_a and p_b.
     */
    [[nodiscard]] static inline bool has_intersection(const uint64_t* p_a, const uint64_t* p_b, size_t p_count) {
        if (p_count == 0) return false;

        size_t i = 0;

#if defined(IDEAM_SIMD_AVX2)
        // Process 256 bits (4 uint64_t) at a time.
        // Padded selections will always satisfy (p_count % 4 == 0).
        for (; i + 3 < p_count; i += 4) {
            __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&p_a[i]));
            __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&p_b[i]));
            
            if (!_mm256_testz_si256(va, vb)) {
                return true;
            }
        }
#elif defined(IDEAM_SIMD_SSE)
        // Process 128 bits (2 uint64_t) at a time.
        for (; i + 1 < p_count; i += 2) {
            __m128i va = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&p_a[i]));
            __m128i vb = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&p_b[i]));
            
            #if defined(__SSE4_1__)
            if (!_mm_testz_si128(va, vb)) {
                return true;
            }
            #else
            __m128i res = _mm_and_si128(va, vb);
            __m128i zero = _mm_setzero_si128();
            __m128i cmp = _mm_cmpeq_epi8(res, zero);
            if (static_cast<uint16_t>(_mm_movemask_epi8(cmp)) != 0xFFFF) {
                return true;
            }
            #endif
        }
#endif

        // Scalar Fallback / Tail handling
        for (; i < p_count; ++i) {
            if ((p_a[i] & p_b[i]) != 0) {
                return true;
            }
        }

        return false;
    }

    /**
     * apply_union
     * Performs p_target |= p_source.
     */
    static inline void apply_union(uint64_t* r_target, const uint64_t* p_source, size_t p_count) {
        size_t i = 0;

#if defined(IDEAM_SIMD_AVX2)
        for (; i + 3 < p_count; i += 4) {
            __m256i vt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&r_target[i]));
            __m256i vs = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&p_source[i]));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&r_target[i]), _mm256_or_si256(vt, vs));
        }
#elif defined(IDEAM_SIMD_SSE)
        for (; i + 1 < p_count; i += 2) {
            __m128i vt = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&r_target[i]));
            __m128i vs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&p_source[i]));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&r_target[i]), _mm_or_si128(vt, vs));
        }
#endif

        for (; i < p_count; ++i) {
            r_target[i] |= p_source[i];
        }
    }

    /**
     * apply_difference
     * Performs p_target &= ~p_source.
     */
    static inline void apply_difference(uint64_t* r_target, const uint64_t* p_source, size_t p_count) {
        size_t i = 0;

#if defined(IDEAM_SIMD_AVX2)
        for (; i + 3 < p_count; i += 4) {
            __m256i vt = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&r_target[i]));
            __m256i vs = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&p_source[i]));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&r_target[i]), _mm256_andnot_si256(vs, vt));
        }
#elif defined(IDEAM_SIMD_SSE)
        for (; i + 1 < p_count; i += 2) {
            __m128i vt = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&r_target[i]));
            __m128i vs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&p_source[i]));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&r_target[i]), _mm_andnot_si128(vs, vt));
        }
#endif

        for (; i < p_count; ++i) {
            r_target[i] &= ~p_source[i];
        }
    }

    /**
     * copy_inverse
     * Performs r_target = ~p_source.
     * Vectorized inversion to quickly flip global active masks into selection masks.
     */
    static inline void copy_inverse(uint64_t* r_target, const uint64_t* p_source, size_t p_count) {
        size_t i = 0;

#if defined(IDEAM_SIMD_AVX2)
        const __m256i ones = _mm256_set1_epi32(-1);
        for (; i + 3 < p_count; i += 4) {
            __m256i vs = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&p_source[i]));
            // _mm256_andnot_si256(a, b) computes (~a) & b. Since b is all 1s, it yields ~a.
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&r_target[i]), _mm256_andnot_si256(vs, ones));
        }
#elif defined(IDEAM_SIMD_SSE)
        const __m128i ones = _mm_set1_epi32(-1);
        for (; i + 1 < p_count; i += 2) {
            __m128i vs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&p_source[i]));
            _mm_storeu_si128(reinterpret_cast<__m128i*>(&r_target[i]), _mm_andnot_si128(vs, ones));
        }
#endif

        // Tail processing for trailing QWORDs
        for (; i < p_count; ++i) {
            r_target[i] = ~p_source[i];
        }
    }

    /**
     * fill_all
     * High-speed raw memset for 0xFF fill.
     */
    static inline void fill_all(uint64_t* r_target, size_t p_count) {
        std::memset(r_target, 0xFF, p_count * sizeof(uint64_t));
    }
};

} // namespace ideam::core

#endif // IDEAM_CORE_COLLISION_H