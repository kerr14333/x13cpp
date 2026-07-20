make_fake_result <- function() {
  s <- airline
  new_x13_result(
    call = quote(seasonal_adjust(airline)),
    series = s,
    udg = list(nobs = length(s), aicc = 123.45),
    fct = stats::ts(rep(NA_real_, 12), start = stats::end(s), frequency = 12),
    save = list(d11 = s, d12 = s, d13 = s / s),
    status = "ok"
  )
}

test_that("accessors read the right fields", {
  res <- make_fake_result()
  expect_equal(udg(res), list(nobs = length(airline), aicc = 123.45))
  expect_true(stats::is.ts(fct(res)))
  expect_equal(seasadj(res), res$save$d11)
  expect_equal(trend(res), res$save$d12)
  expect_equal(irregular(res), res$save$d13)
  expect_equal(save_table(res, "d11"), res$save$d11)
})

test_that("save_table() errors clearly on an unknown table", {
  res <- make_fake_result()
  expect_error(save_table(res, "s10"), regexp = "not present")
})

test_that("print and summary methods run without error", {
  res <- make_fake_result()
  expect_output(print(res), "x13_result")
  expect_output(summary(res), "Diagnostics")
})

test_that("write_outputs() writes files only when explicitly called", {
  res <- make_fake_result()
  tmp <- tempfile("x13cpp-test-")
  dir.create(tmp)
  on.exit(unlink(tmp, recursive = TRUE), add = TRUE)

  expect_false(file.exists(file.path(tmp, "series.udg")))
  write_outputs(res, tmp)
  expect_true(file.exists(file.path(tmp, "series.udg")))
  expect_true(file.exists(file.path(tmp, "series.fct")))
  expect_true(file.exists(file.path(tmp, "series.d11")))

  expect_error(write_outputs(res, tmp), regexp = "overwrite")
  expect_message(write_outputs(res, tmp, overwrite = TRUE), regexp = "Wrote")
})
