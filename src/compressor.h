#ifndef LEVIDB_COMPRESSOR_H
#define LEVIDB_COMPRESSOR_H

/*
 * 压缩器(compressor)按执行顺序由两部分组成,
 * 1. 重复字符串检测器(repeat_detector)
 * 2. 算术编码器(coder)
 *
 * 长度缩短12.5%以上才接受压缩后的编码,
 * 否则返回 原编码 + checksum
 *
 * 调用步骤:
 * 提交源字符串, 再不断申请片段,
 * 必须回填上段实际写入位置
 *
 * 内部实现:
 * KV 会被转换为
 * content = varint32 k_length + key + val
 */

#include "arena.h"
#include "coder.h"
#include "repeat_detector.h"
#include "slice.h"
#include <vector>

namespace LeviDB {
    class Compressor {

    public:
        void submit(const Slice & key, const Slice & val) noexcept;

        // 单次申请最大长度 2**15(max log record length)
        std::vector<int> request(int size, bool compress) noexcept;

        void report(uint64_t offset) noexcept;
    };
}

#endif //LEVIDB_COMPRESSOR_H