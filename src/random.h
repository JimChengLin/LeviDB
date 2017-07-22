#ifndef LEVIDB_RANDOM_H
#define LEVIDB_RANDOM_H

#include <cstdint>

/*
 * 简单随机数生成器
 */

namespace LeviDB {
    class Random {
    private:
        uint32_t _seed;

    public:
        explicit Random(uint32_t s) noexcept
                : _seed(s & 0x7fffffffu) {
            if (_seed == 0 || _seed == 2147483647L) {
                _seed = 1;
            }
        }

        ~Random() noexcept = default;

        uint32_t next() noexcept {
            // 余数法
            static constexpr uint32_t M = 2147483647L;
            static constexpr uint64_t A = 16807;

            uint64_t product = _seed * A;
            _seed = static_cast<uint32_t>((product >> 31) + (product & M));
            if (_seed > M) {
                _seed -= M;
            }
            return _seed;
        }

        uint32_t uniform(int n) noexcept { return next() % n; }

        bool oneIn(int n) noexcept { return (next() % n) == 0; }
    };
}

#endif //LEVIDB_RANDOM_H