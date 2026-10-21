#pragma once

//-----------------------------------------------------------------------------
// Minimal test runner: one line per test, and the details only for the checks that fail.
//-----------------------------------------------------------------------------

#include <cstdio>
#include <exception>
#include <string>
#include <vector>

namespace TestRunner {

    //---------------------------------
    struct Test {
        const char  *name;
        void        (*function)();
    };

    //---------------------------------
    inline std::vector<Test> &
    GetTests() {
        static std::vector<Test> tests;
        return tests;
    }

    //---------------------------------
    inline std::vector<std::string> &
    GetFailures() {
        static std::vector<std::string> failures;
        return failures;
    }

    //---------------------------------
    inline int &
    GetNumChecks() {
        static int numChecks = 0;
        return numChecks;
    }

    //---------------------------------
    struct Register {
        Register(const char *name, void (*function)()) { GetTests().push_back(Test { name, function }); }
    };

    //---------------------------------
    inline void
    Check(bool ok, const char *expression, const char *file, int line) {
        ++GetNumChecks();
        if (!ok) {
            const char *name = file;
            for (const char *c = file; *c != '\0'; ++c) {
                if (*c == '/' || *c == '\\') {
                    name = c + 1;
                }
            }

            char buffer[512];
            snprintf(buffer, sizeof(buffer), "%s:%d: %s", name, line, expression);
            GetFailures().push_back(buffer);
        }
    }

    //---------------------------------
    inline int
    RunAll() {
        size_t numFailed = 0;

        for (const Test &test : GetTests()) {
            GetFailures().clear();
            GetNumChecks() = 0;

            try {
                test.function();
            }
            catch (const std::exception &e) {
                GetFailures().push_back(std::string("unexpected exception: ") + e.what());
            }
            catch (...) {
                GetFailures().push_back("unexpected unknown exception");
            }

            bool ok = GetFailures().empty();
            printf("[%s] %s (%d checks)\n", ok ? " OK " : "FAIL", test.name, GetNumChecks());
            for (const std::string &failure : GetFailures()) {
                printf("         %s\n", failure.c_str());
            }

            if (!ok) {
                ++numFailed;
            }
        }

        printf("\n%zu tests, %zu failed\n", GetTests().size(), numFailed);
        return numFailed == 0 ? 0 : 1;
    }

} // end of namespace

#define TEST(name) \
    static void name(); \
    static TestRunner::Register name##Register(#name, name); \
    static void name()

#define CHECK(expression)   TestRunner::Check((expression), #expression, __FILE__, __LINE__)
