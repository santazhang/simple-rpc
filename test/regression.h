#pragma once

#include <map>
#include <string>

class TestRunner {
    std::map<std::string, void (*)(int, char**)> tests_;

public:

    void reg(const char* name, void (*test_func)(int, char**)) {
        if (tests_.find(name) != tests_.end()) {
            printf("duplicate test name: %s\n", name);
            exit(1);
        }
        tests_[name] = test_func;
    }

    void run(int argc, char* argv[]) {
        if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            printf("usage: %s <test-name>   run test\n", argv[0]);
            printf("       %s --list        list all tests\n", argv[0]);
            printf("       %s --help        show this info\n", argv[0]);
        } else if (strcmp(argv[1], "--list") == 0) {
            for (std::map<std::string, void (*)(int, char**)>::iterator it = tests_.begin(); it != tests_.end(); ++it) {
                printf("%s\n", it->first.c_str());
            }
        } else {
            std::map<std::string, void (*)(int, char**)>::iterator it = tests_.find(argv[1]);
            if (it == tests_.end()) {
                printf("test '%s' not found!\n", argv[1]);
            } else {
                it->second(argc, argv);
            }
        }
    }

};
