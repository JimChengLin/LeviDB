#include "../src/env_io.h"
#include "../src/log_writer.h"
#include <iostream>
#include <array>

void compress_bench() noexcept {
    const std::string src_fname = "movies.txt";
    if (LeviDB::IOEnv::fileExists(src_fname)) {
        LeviDB::SequentialFile src(src_fname);

        const std::string dst_fname = "/tmp/levidb_test";
        if (LeviDB::IOEnv::fileExists(dst_fname)) {
            LeviDB::IOEnv::deleteFile(dst_fname);
        }

        {
            LeviDB::AppendableFile dst(dst_fname);
            LeviDB::LogWriter writer(&dst);

            char * line = nullptr;
            size_t len = 0;
            ssize_t n_read;

            int i = 0;
            std::array<std::string, 9> que;
            while ((n_read = getline(&line, &len, src.getFILE())) != -1) {
                que[i] = std::string(line, static_cast<size_t>(n_read - 1));
                if (++i == 9) {
                    i = 0;
                    std::string key = que[0] + que[1];
                    std::string val = que[2] + que[3] + que[4] + que[5] + que[6] + que[7];
                    LeviDB::LogWriter::Record record{key, val, false};
                    std::cout << "key: " << key << std::endl;
                    std::cout << "val: " << val << std::endl;
                    writer.addRecord(record);
                }
            }
            free(line);
        }

        std::cout << "file size: " << LeviDB::IOEnv::getFileSize(dst_fname) << std::endl;
        LeviDB::IOEnv::deleteFile(dst_fname);
        std::cout << __FUNCTION__ << std::endl;
    }
}