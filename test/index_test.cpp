#include "../src/index.h"
#include <iostream>

#ifndef __clang__
#include <cstring> // GCC
#endif

void index_test() noexcept {
    {
        auto a = new char[2];
        auto b = new char[2];
        auto c = new char[2];
        a[0] = 'A';
        a[1] = 0;
        b[0] = 'F';
        b[1] = 0;
        c[0] = 'G';
        c[1] = 0;

        LeviDB::BitDegradeTree tree;
        tree.insert(a);
        tree.insert(c);
        tree.insert(b);

        auto x = new char[2];
        x[0] = 'H';
        x[1] = 0;
        tree.insert(x);

        x = new char[2];
        x[0] = 'I';
        x[1] = 0;
        tree.insert(x);

        x = new char[2];
        x[0] = 'B';
        x[1] = 0;
        tree.insert(x);

        for (const char * ptr:{"A", "G", "F", "B", "I", "H"}) {
            assert(tree.find(ptr)[0] == ptr[0]);
        }
        assert(tree.find("_")[0] != '_');

        tree.remove("A");
        for (const char * ptr:{"G", "F", "B", "I", "H"}) {
            assert(tree.find(ptr)[0] == ptr[0]);
        }
        for (const char * ptr:{"G", "F", "B", "I", "H"}) {
            tree.remove(ptr);
        }
        assert(tree.getSize(tree._root) == 0);
    }

    {
        LeviDB::BitDegradeTree tree;
        std::array<const char *, 1000> sources;
        std::array<char, 5> alphabet{{'A', 'B', 'C', 'D', 'E'}};

        srand(19950207);
        for (int i = 0; i < sources.size(); ++i) {
            auto x = new char[5];
            x[4] = 0;
            for (int j = 0; j < 4; ++j) {
                x[j] = alphabet[rand() % alphabet.size()];
            }
            auto res = tree.find(x);
            if (res != nullptr && strcmp(res, x) == 0) {
                delete[] x;
                sources[i] = nullptr;
                continue;
            }

            tree.insert(x);
            sources[i] = x;
            for (int j = 0; j <= i; ++j) {
                if (sources[j] != nullptr) assert(strcmp(tree.find(sources[j]), sources[j]) == 0);
            }

            if (rand() % 2 == 0) {
                tree.remove(x);
                sources[i] = nullptr;
                for (int j = 0; j <= i; ++j) {
                    if (sources[j] != nullptr) assert(strcmp(tree.find(sources[j]), sources[j]) == 0);
                }
            }
        }

        for (int i = 0; i < sources.size(); ++i) {
            if (sources[i] != nullptr) {
                tree.remove(sources[i]);
                sources[i] = nullptr;
            }
            for (const char * src:sources) {
                if (src != nullptr) assert(strcmp(tree.find(src), src) == 0);
            }
        }
        assert(tree.getSize(tree._root) == 0);
    }

    std::cout << __FUNCTION__ << std::endl;
}