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

# The fformat test validates against gfortran when available. Point it at the
# gfortran the build already knows about (Fortran optional), and give it a
# writable scratch directory. If gfortran cannot be found the test self-skips.
find_program(X13_GFORTRAN gfortran)
if(X13_GFORTRAN)
  set_tests_properties(test_fformat PROPERTIES
    ENVIRONMENT "FFORMAT_GFORTRAN=${X13_GFORTRAN};FFORMAT_SCRATCH=${CMAKE_CURRENT_BINARY_DIR}")
endif()
