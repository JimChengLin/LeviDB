#ifndef LEVIDB_RESULT_H
#define LEVIDB_RESULT_H

/*
 * 记录字符串已匹配的开始和结束
 * 成功或失败
 */

#include <cassert>
#include <climits>
#include <string>

namespace LeviDB {
    namespace Regex {
        class Result {
        public:
            int _op = 1;
            int _ed = 0;
            int _select_from = INT_MIN;
            int _select_to = INT_MAX;
            bool _success{};

        public:
            Result() noexcept = default;

            Result(int op, int ed, bool success) noexcept
                    : _op(op), _ed(ed), _success(success) { assert(!isContinue()); }

            Result & asSuccess() noexcept {
                assert(!isContinue());
                _success = true;
                return *this;
            }

            Result & asFail() noexcept {
                assert(!isContinue());
                _success = false;
                return *this;
            }

            Result & invert() noexcept {
                // don't care if it is Continue
                _success = !_success;
                return *this;
            }

            bool isSuccess() const noexcept {
                assert(!isContinue());
                return _success;
            }

            bool isFail() const noexcept {
                assert(!isContinue());
                return !_success;
            }

            bool isContinue() const noexcept { return _op > _ed; }

            std::string toString() const noexcept {
                if (isContinue()) {
                    return "Continue";
                }
                std::string r = isSuccess() ? "Success:" : "Fail:";
                return r + std::to_string(_op) + ',' + std::to_string(_ed);
            }

            bool operator==(const Result & another) const noexcept {
                return _op == another._op && _ed == another._ed &&
                       _select_from == another._select_from && _select_to == another._select_to &&
                       _success == another._success;
            }
        };
    }
}

#endif // LEVIDB_RESULT_H