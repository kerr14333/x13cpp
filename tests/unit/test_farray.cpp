#include "microtest.hpp"
#include "x13/farray.hpp"

using namespace x13;

TEST("farray1 basic 1-based access") {
    farray1<double, 5> a;
    a.fill(0.0);
    for (long i = 1; i <= 5; ++i) a(i) = double(i * 10);
    CHECK_EQ(a(1), 10.0);
    CHECK_EQ(a(5), 50.0);
    CHECK_EQ(a.size(), (std::size_t)5);
    // storage is contiguous, 0-based raw matches 1-based
    CHECK_EQ(a.data()[0], 10.0);
    CHECK_EQ(a.data()[4], 50.0);
}

TEST("farray1 bounds checking throws") {
    farray1<int, 3> a;
    bool threw = false;
    try { a(0) = 1; } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
    threw = false;
    try { a(4) = 1; } catch (const std::out_of_range&) { threw = true; }
    CHECK(threw);
}

TEST("farray1lb lower bound 0 (Fortran 0:N)") {
    farray1lb<int, 0, 4> a;  // indices 0..3
    for (long i = 0; i <= 3; ++i) a(i) = int(i);
    CHECK_EQ(a(0), 0);
    CHECK_EQ(a(3), 3);
    CHECK_EQ(a.lbound, (long)0);
    CHECK_EQ(a.ubound, (long)3);
}

TEST("farray2 column-major layout") {
    farray2<double, 3, 2> a;  // 3 rows, 2 cols
    // Fill so we can check storage order: element(i,j) at (i-1)+(j-1)*3
    a(1,1) = 1; a(2,1) = 2; a(3,1) = 3;
    a(1,2) = 4; a(2,2) = 5; a(3,2) = 6;
    // column-major: data = [1,2,3,4,5,6]
    const double* d = a.data();
    CHECK_EQ(d[0], 1.0);
    CHECK_EQ(d[1], 2.0);
    CHECK_EQ(d[2], 3.0);
    CHECK_EQ(d[3], 4.0);
    CHECK_EQ(d[4], 5.0);
    CHECK_EQ(d[5], 6.0);
    CHECK_EQ(a(3,2), 6.0);
}

TEST("farray1d dynamic with lower bound") {
    farray1d<double> a(10, 1);
    CHECK_EQ(a.size(), (std::size_t)10);
    CHECK_EQ(a.lbound(), (long)1);
    CHECK_EQ(a.ubound(), (long)10);
    a(10) = 99.0;
    CHECK_EQ(a(10), 99.0);
    a.resize(4, 0);
    CHECK_EQ(a.lbound(), (long)0);
    CHECK_EQ(a.ubound(), (long)3);
}

TEST("farray2d dynamic column-major") {
    farray2d<int> a(2, 3);
    a(1,1)=1; a(2,1)=2; a(1,2)=3; a(2,2)=4; a(1,3)=5; a(2,3)=6;
    const int* d = a.data();
    CHECK_EQ(d[0], 1); CHECK_EQ(d[1], 2); CHECK_EQ(d[2], 3);
    CHECK_EQ(d[3], 4); CHECK_EQ(d[4], 5); CHECK_EQ(d[5], 6);
    CHECK_EQ(a.rows(), (std::size_t)2);
    CHECK_EQ(a.cols(), (std::size_t)3);
}

int main() { return mt::run_all(); }
