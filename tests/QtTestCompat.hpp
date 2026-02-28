#ifndef TESTS_QTTESTCOMPAT_HPP
#define TESTS_QTTESTCOMPAT_HPP

#include <QDebug>
#include <QtGlobal>

#include <functional>
#include <vector>

namespace qttest_compat {

struct TestEntry {
    const char* suite;
    const char* name;
    std::function<void()> fn;
};

inline std::vector<TestEntry>& registry() {
    static std::vector<TestEntry> tests;
    return tests;
}

inline int& failureCount() {
    static int count = 0;
    return count;
}

inline void reportFailure(const char* expression, const char* file, int line) {
    ++failureCount();
    qWarning().noquote() << QString("FAIL: %1 (%2:%3)").arg(expression).arg(file).arg(line);
}

class Registrar {
public:
    Registrar(const char* suite, const char* name, const std::function<void()>& fn) {
        registry().push_back({suite, name, fn});
    }
};

inline int runAllTests() {
    const int total = static_cast<int>(registry().size());

    qInfo().noquote() << QString("Running %1 QtTest-compat tests").arg(total);

    int index = 0;
    for (const auto& test : registry()) {
        ++index;
        qInfo().noquote() << QString("[%1/%2] %3.%4").arg(index).arg(total).arg(test.suite).arg(test.name);
        test.fn();
    }

    const int failures = failureCount();
    qInfo().noquote() << QString("Completed. Failures: %1").arg(failures);

    return (failures == 0 ? 0 : 1);
}

}  // namespace qttest_compat

#define TEST(_suite, _name)                                                            \
    static void _suite##_##_name##_Test();                                             \
    static qttest_compat::Registrar _suite##_##_name##_registrar(                      \
        #_suite, #_name, []() { _suite##_##_name##_Test(); });                         \
    static void _suite##_##_name##_Test()

#define EXPECT_TRUE(_expr)                                                              \
    do {                                                                                 \
        if (!(_expr)) {                                                                  \
            qttest_compat::reportFailure(#_expr, __FILE__, __LINE__);                   \
        }                                                                                \
    } while (0)

#define EXPECT_FALSE(_expr) EXPECT_TRUE(!(_expr))
#define ASSERT_TRUE(_expr)                                                               \
    do {                                                                                 \
        if (!(_expr)) {                                                                  \
            qttest_compat::reportFailure(#_expr, __FILE__, __LINE__);                   \
            return;                                                                      \
        }                                                                                \
    } while (0)

#define ASSERT_FALSE(_expr) ASSERT_TRUE(!(_expr))

#define EXPECT_EQ(_lhs, _rhs)                                                            \
    do {                                                                                 \
        const auto& _l = (_lhs);                                                        \
        const auto& _r = (_rhs);                                                        \
        if (!(_l == _r)) {                                                               \
            qttest_compat::reportFailure(#_lhs " == " #_rhs, __FILE__, __LINE__);      \
        }                                                                                \
    } while (0)

#define EXPECT_NE(_lhs, _rhs)                                                            \
    do {                                                                                 \
        const auto& _l = (_lhs);                                                        \
        const auto& _r = (_rhs);                                                        \
        if (_l == _r) {                                                                  \
            qttest_compat::reportFailure(#_lhs " != " #_rhs, __FILE__, __LINE__);      \
        }                                                                                \
    } while (0)

#define ASSERT_EQ(_lhs, _rhs)                                                            \
    do {                                                                                 \
        const auto& _l = (_lhs);                                                        \
        const auto& _r = (_rhs);                                                        \
        if (!(_l == _r)) {                                                               \
            qttest_compat::reportFailure(#_lhs " == " #_rhs, __FILE__, __LINE__);      \
            return;                                                                      \
        }                                                                                \
    } while (0)

#define ASSERT_NE(_lhs, _rhs)                                                            \
    do {                                                                                 \
        const auto& _l = (_lhs);                                                        \
        const auto& _r = (_rhs);                                                        \
        if (_l == _r) {                                                                  \
            qttest_compat::reportFailure(#_lhs " != " #_rhs, __FILE__, __LINE__);      \
            return;                                                                      \
        }                                                                                \
    } while (0)

#endif  // TESTS_QTTESTCOMPAT_HPP
