#include "microtest.hpp"
#include "x13/channels.hpp"
#include "x13/x13error.hpp"
#include <string>

using namespace x13;

TEST("OutBuffer plain records") {
    OutBuffer b;
    b.put_record("hello");
    b.put_record("world");
    CHECK_EQ(b.str(), std::string("hello\nworld\n"));
    b.clear();
    CHECK_EQ(b.str(), std::string(""));
}

TEST("OutBuffer raw put") {
    OutBuffer b;
    b.put("ab");
    b.put("cd");
    CHECK_EQ(b.str(), std::string("abcd"));
}

TEST("OutBuffer carriage control") {
    OutBuffer b;
    b.set_carriage_control(true);
    b.put_record(" single");   // leading space -> single spacing, stripped
    b.put_record("0double");   // '0' -> extra blank line
    std::string s = b.str();
    CHECK_EQ(s, std::string("single\n\ndouble\n"));
}

TEST("ChannelRegistry maps unit numbers") {
    ChannelRegistry reg;
    const int Mt1 = 6, Mt2 = 7, Ng = 8, Nform = 9;
    reg.unit(Mt1).put_record("main");
    reg.unit(Mt2).put_record("err");
    reg.unit(Ng).put("log");
    reg.unit(Nform).put("udg");
    CHECK(reg.has(Mt1));
    CHECK(reg.has(Nform));
    CHECK(!reg.has(42));
    CHECK_EQ(reg.unit(Mt1).str(), std::string("main\n"));
    CHECK_EQ(reg.unit(Mt2).str(), std::string("err\n"));
    CHECK_EQ(reg.unit(Ng).str(), std::string("log"));
    reg.reset();
    CHECK(!reg.has(Mt1));
}

TEST("ErrorState and X13Error") {
    ErrorState es;
    CHECK(!es.lfatal);
    es.lfatal = true;
    CHECK(es.lfatal);
    bool threw = false;
    try { throw X13Error("boom"); }
    catch (const std::runtime_error& e) { threw = true; CHECK_EQ(std::string(e.what()), std::string("boom")); }
    CHECK(threw);
}

int main() { return mt::run_all(); }
