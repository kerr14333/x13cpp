# Unit tests -- self-contained (microtest.hpp), zero external dependencies.

set(X13_UNIT_TESTS
    test_farray
    test_fstring
    test_channels
    test_fformat)

foreach(t ${X13_UNIT_TESTS})
  add_executable(${t} ${t}.cpp)
  target_include_directories(${t} PRIVATE
    "${CMAKE_SOURCE_DIR}/core/include"
    "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(${t} PRIVATE x13_fp_flags)
  add_test(NAME ${t} COMMAND ${t})
endforeach()

# test_numeric links the core library (M3 Tier-0 numeric leaves live in
# core/src/numeric) and needs core/src on the include path for numeric/numeric.hpp.
add_executable(test_numeric test_numeric.cpp)
target_include_directories(test_numeric PRIVATE
  "${CMAKE_SOURCE_DIR}/core/include"
  "${CMAKE_SOURCE_DIR}/core/src"
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_numeric PRIVATE x13core x13_fp_flags)
add_test(NAME test_numeric COMMAND test_numeric)

# test_x11 links the core library (X-11 Tier-0/Tier-1 leaves live in
# core/src/x11); endsf reuses totals() from core/src/numeric. Needs core/src on
# the include path for x11/x11filt.hpp and numeric/numeric.hpp.
add_executable(test_x11 test_x11.cpp)
target_include_directories(test_x11 PRIVATE
  "${CMAKE_SOURCE_DIR}/core/include"
  "${CMAKE_SOURCE_DIR}/core/src"
  "${CMAKE_SOURCE_DIR}/core/prm"
  "${CMAKE_SOURCE_DIR}/core/prm/gen"
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_x11 PRIVATE x13core x13_fp_flags)
add_test(NAME test_x11 COMMAND test_x11)

# test_seats links the core library (SEATS poly/root leaves live in
# core/src/seats) and needs core/src on the include path for seats/seatspoly.hpp.
add_executable(test_seats test_seats.cpp)
target_include_directories(test_seats PRIVATE
  "${CMAKE_SOURCE_DIR}/core/include"
  "${CMAKE_SOURCE_DIR}/core/src"
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_seats PRIVATE x13core x13_fp_flags)
add_test(NAME test_seats COMMAND test_seats)

# The fformat test validates against gfortran when available. Point it at the
# gfortran the build already knows about (Fortran optional), and give it a
# writable scratch directory. If gfortran cannot be found the test self-skips.
find_program(X13_GFORTRAN gfortran)
if(X13_GFORTRAN)
  set_tests_properties(test_fformat PROPERTIES
    ENVIRONMENT "FFORMAT_GFORTRAN=${X13_GFORTRAN};FFORMAT_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}")
endif()
