#pragma once

#include <nil/crypto3/hash/algorithm/hash.hpp>
#include <nil/crypto3/hash/sha2.hpp>
#include "base_types.h"
#include "../utils/picosha2.h"

using namespace nil::crypto3;

namespace circuit_byte_utils {

#define countof(array) (sizeof(array) / sizeof(array[0]))

    using sha256_t = typename hashes::sha2<256>::block_type;

    bool sha256_equals(sha256_t hash1, sha256_t hash2) {
        bool result = true;
        for (auto i = 0; i < countof(hash1); ++i) {
            result = result && (hash1[i] == hash2[i]);
        }

        return result;
    }

    template<typename T>
    Byte get_nth_byte(const T val, unsigned int n) {
        static_assert(std::is_integral_v<typename std::remove_reference_t<T>>, "T must be integral");
        assert_true(n < sizeof(T));

        return Byte(val >> (n * 8));
    }

    Byte get_nth_byte(sha256_t val, size_t int_count, size_t byte_count) {
        // TODO: implement when we start using crypto3's sha256
        return Byte {};
    }

    bool get_nth_bit(uint64_t value, short i) {
        return bool(1 & (value >> i));
    }

    Byte set_nth_bit(Byte& value, short i) {
        assert_true(i < 8);
        return value | (Byte(1) << i);
    }

    Bytes32 sha256_to_bytes_array(sha256_t sha) {
        Bytes32 out;
        assert_true(out.size() >= sizeof(sha));
        for (int int_count = 0; int_count < countof(sha); int_count++) {

            for (int byte_count = 0; byte_count < sizeof(sha[0]); byte_count++) {
                out[int_count * sizeof(sha[0]) + byte_count] = get_nth_byte(sha, int_count, byte_count);
            }
        }
        return out;
    }

    template<std::size_t N, typename T, std::size_t InputSize>
    std::array<T, N> take(const std::array<T, InputSize>& val) {
        static_assert(N <= InputSize);
        std::array<T, N> ret {};
        std::copy(val.begin(), val.begin() + N, ret.begin());

        return ret;
    }

    template<typename T, bool LittleEndian = true>
    std::array<Byte, sizeof(T)> int_to_bytes(const T& paramInt) {
        static_assert(std::is_integral_v<typename std::remove_reference_t<T>>, "T must be integral");
        std::array<Byte, sizeof(T)> bytes {};
        if constexpr (LittleEndian) {
            for (int i = 0; i < sizeof(T); ++i) {
                bytes[i] = (paramInt >> (i * 8));
            }
        } else {
            for (int i = sizeof(T) - 1; i >= 0; i--) {
                bytes[i] = (paramInt >> (i * 8));
            }
        }
        return bytes;
    }

    template<typename T, bool LittleEndian = true>
    T bytes_to_int(const std::array<Byte, sizeof(T)>& bytes) {
        static_assert(std::is_integral_v<typename std::remove_reference_t<T>>, "T must be integral");
        T result = 0;
        if constexpr (LittleEndian) {
            for (int i = sizeof(T) - 1; i >= 0; i--) {
                result = (result << 8) + bytes[i];
            }
        } else {
            for (unsigned i = 0; i < sizeof(T); ++i) {
                result = (result << 8) + bytes[i];
            }
        }
        return result;
    }

    template<typename... Args>
    struct SizeSum;

    // Partial specialization for an empty parameter pack (base case)
    template<>
    struct SizeSum<> {
        static constexpr size_t value = 0;    // Sum is 0 when there are no arguments
    };

    // Recursive partial specialization for non-empty parameter pack
    template<typename First, typename... Rest>
    struct SizeSum<First, Rest...> {
        static constexpr size_t value = sizeof(First) + SizeSum<Rest...>::value;
    };

    template<typename T>
    struct HasPadding : std::conditional_t<std::has_unique_object_representations_v<T> || std::is_same_v<T, float> ||
                                               std::is_same_v<T, double>,
                                           std::false_type, std::true_type> { };

    template<typename T>
    struct CanConvertToBytes : std::false_type { };
    template<>
    struct CanConvertToBytes<Byte> : std::true_type { };
    template<>
    struct CanConvertToBytes<int> : std::true_type { };
    template<>
    struct CanConvertToBytes<size_t> : std::true_type { };
    template<size_t N>
    struct CanConvertToBytes<std::array<Byte, N>> : std::true_type { };

#define ASSERT_IS_HASHABLE(T)                                                                \
    using PureType = std::remove_cv_t<std::remove_reference_t<T>>;                           \
    static_assert(!std::is_pointer_v<PureType>, "The argument must not be a pointer.");      \
    static_assert(CanConvertToBytes<PureType>::value, "The argument cannot be serialized."); \
    static_assert(!HasPadding<PureType>::value, "The argument must not contain padding.");

    template<typename T, size_t N>
    void to_bytes(const T& val, std::array<Byte, N>& buffer, size_t offset) {
        ASSERT_IS_HASHABLE(T);
        memcpy(&buffer[offset], (char*)&val, sizeof(T));
    }

    template<typename T, size_t N>
    void to_bytes(const T& val, std::array<Byte, N>& buffer, size_t offset, size_t total_bytes) {
        ASSERT_IS_HASHABLE(T);
        assert_true(N >= offset + total_bytes);
        memcpy(&buffer[offset], (char*)&val, total_bytes);
    }

    template<size_t NBytesToHash>
    void hash_recursive(std::array<Byte, NBytesToHash>& buffer, size_t& offset) {
    }

    template<size_t NBytesToHash, typename First, typename... Rest>
    void hash_recursive(std::array<Byte, NBytesToHash>& buffer, size_t& offset, const First& first,
                        const Rest&... rest) {
        to_bytes(first, buffer, offset);
        offset += sizeof(First);
        hash_recursive(buffer, offset, rest...);
    }

    template<typename... Args>
    Bytes32 sha256(const Args&... args) {
        std::array<Byte, SizeSum<Args...>::value> buffer;
        size_t offset = 0;
        hash_recursive(buffer, offset, args...);

        // TODO: use crypto3 sha after stack smash bug is fixed.
        //!!! auto digest = hash<hashes::sha2<256>>();
        //!!! sha256_to_bytes();

        Bytes32 hashed;
        picosha2::hash256(buffer.begin(), buffer.end(), hashed.begin(), hashed.end());
        return hashed;
    }

    template<size_t NBytesToHash, typename Arg>
    Bytes32 sha256(const Arg& arg, size_t offset = 0) {
        std::array<Byte, NBytesToHash> buffer;
        to_bytes(arg, buffer, offset, NBytesToHash);

        // TODO: use crypto3 sha after stack smash bug is fixed.
        //!!! auto digest = hash<hashes::sha2<256>>();
        //!!! sha256_to_bytes();

        Bytes32 hashed;
        picosha2::hash256(buffer.begin(), buffer.begin() + NBytesToHash, hashed.begin(), hashed.end());
        return hashed;
    }

}    // namespace circuit_byte_utils
