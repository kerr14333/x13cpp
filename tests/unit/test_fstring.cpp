#include "microtest.hpp"
#include "x13/fstring.hpp"
#include <string>

using namespace x13;

TEST("fstring pads with blanks on assignment") {
    fstring<6> s("abc");
    CHECK_EQ(s.raw(), std::string_view("abc   "));
    CHECK_EQ(s.str(), std::string("abc"));
    CHECK_EQ(s.len_trim(), (std::size_t)3);
    CHECK_EQ(s.size(), (std::size_t)6);
}

TEST("fstring truncates when source longer") {
    fstring<3> s("abcdef");
    CHECK_EQ(s.str(), std::string("abc"));
    CHECK_EQ(s.raw(), std::string_view("abc"));
}

TEST("fstring comparison ignores trailing blanks") {
    fstring<6> a("abc");
    fstring<3> b("abc");
    CHECK(a == b);          // "abc   " == "abc"
    CHECK(a == std::string_view("abc"));
    CHECK(std::string_view("abc") == a);
    fstring<6> c("abd");
    CHECK(a < c);
    CHECK(c > a);
    CHECK(a != c);
}

TEST("fstring blank-extension in comparison") {
    fstring<5> a("ab");
    fstring<2> b("ab");
    CHECK(a == b);          // both "ab" ignoring blanks
    fstring<5> empty("");
    CHECK(empty == std::string_view(""));
    CHECK(empty == std::string_view("   "));  // all blanks equal empty
}

TEST("fstring 1-based char access") {
    fstring<5> s("hello");
    CHECK_EQ(s(1), 'h');
    CHECK_EQ(s(5), 'o');
    s(1) = 'H';
    CHECK_EQ(s.str(), std::string("Hello"));
}

TEST("fstring assignment operator") {
    fstring<4> s;
    s = "xy";
    CHECK_EQ(s.str(), std::string("xy"));
    s = std::string("wxyz!");
    CHECK_EQ(s.str(), std::string("wxyz"));  // truncated to 4
}

int main() { return mt::run_all(); }
