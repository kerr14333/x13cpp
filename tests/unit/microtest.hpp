// microtest.hpp -- tiny zero-dependency test framework for X13cpp unit tests.
//
// Usage:
//   #include "microtest.hpp"
//   TEST("name") { CHECK(expr); CHECK_EQ(a, b); }
//   int main() { return mt::run_all(); }
//
// TEST registers a test via a static initializer. CHECK/REQUIRE record failures;
// REQUIRE aborts the current test. run_all() prints a summary and returns the
// number of failed tests (0 == success) so it doubles as a process exit code.
#ifndef X13_MICROTEST_HPP
#define X13_MICROTEST_HPP

#include <cstdio>
#include <functional>
#include <string>
#include <vector>
#include <sstream>

namespace mt {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> r;
    return r;
}

struct Ctx {
    int checks = 0;
    int failures = 0;
    bool aborted = false;
    std::vector<std::string> messages;
};

inline Ctx*& current() { static Ctx* c = nullptr; return c; }

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct RequireAbort {};

inline void report(bool cond, const std::string& expr, const char* file, int line, bool fatal) {
    Ctx* c = current();
    if (!c) return;
    ++c->checks;
    if (!cond) {
        ++c->failures;
        std::ostringstream os;
        os << "    FAIL " << file << ":" << line << "  " << expr;
        c->messages.push_back(os.str());
        if (fatal) { c->aborted = true; throw RequireAbort{}; }
    }
}

template <class A, class B>
void report_eq(const A& a, const B& b, const std::string& ea, const std::string& eb,
               const char* file, int line, bool fatal) {
    bool cond = (a == b);
    Ctx* c = current();
    if (!c) return;
    ++c->checks;
    if (!cond) {
        ++c->failures;
        std::ostringstream os;
        os << "    FAIL " << file << ":" << line << "  " << ea << " == " << eb
           << "  (" << a << " vs " << b << ")";
        c->messages.push_back(os.str());
        if (fatal) { c->aborted = true; throw RequireAbort{}; }
    }
}

inline int run_all() {
    int passed = 0, failed = 0;
    for (auto& tc : registry()) {
        Ctx ctx;
        current() = &ctx;
        try { tc.fn(); }
        catch (const RequireAbort&) {}
        catch (const std::exception& e) {
            ++ctx.failures;
            ctx.messages.push_back(std::string("    EXCEPTION: ") + e.what());
        }
        catch (...) {
            ++ctx.failures;
            ctx.messages.push_back("    EXCEPTION: unknown");
        }
        current() = nullptr;
        if (ctx.failures == 0) {
            ++passed;
            std::printf("[ PASS ] %s (%d checks)\n", tc.name.c_str(), ctx.checks);
        } else {
            ++failed;
            std::printf("[ FAIL ] %s (%d/%d checks failed)\n", tc.name.c_str(),
                        ctx.failures, ctx.checks);
            for (auto& m : ctx.messages) std::printf("%s\n", m.c_str());
        }
    }
    std::printf("\n==== %d passed, %d failed, %zu total ====\n",
                passed, failed, registry().size());
    return failed;
}

} // namespace mt

#define MT_CAT2(a,b) a##b
#define MT_CAT(a,b) MT_CAT2(a,b)
#define TEST(NAME) \
    static void MT_CAT(mt_test_fn_, __LINE__)(); \
    static ::mt::Registrar MT_CAT(mt_reg_, __LINE__)(NAME, MT_CAT(mt_test_fn_, __LINE__)); \
    static void MT_CAT(mt_test_fn_, __LINE__)()

#define CHECK(expr)   ::mt::report((expr), #expr, __FILE__, __LINE__, false)
#define REQUIRE(expr) ::mt::report((expr), #expr, __FILE__, __LINE__, true)
#define CHECK_EQ(a,b) ::mt::report_eq((a),(b), #a, #b, __FILE__, __LINE__, false)
#define REQUIRE_EQ(a,b) ::mt::report_eq((a),(b), #a, #b, __FILE__, __LINE__, true)

#endif // X13_MICROTEST_HPP
