test_that("seasonal_adjust() stubs out with an informative error", {
  expect_error(seasonal_adjust(airline), class = "simpleError")
  expect_error(seasonal_adjust(airline), regexp = "not yet expose")
})

test_that("seasonal_adjust() validates arguments before hitting the stub", {
  expect_error(seasonal_adjust(as.numeric(airline)), regexp = "ts object")
  expect_error(seasonal_adjust(airline, x11 = TRUE, seats = TRUE),
               regexp = "not both")
})
