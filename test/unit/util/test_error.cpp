#include <tuvx/util/error.hpp>
#include <tuvx/util/internal_error.hpp>

#include <string>

#include <gtest/gtest.h>

TEST(ErrorTest, ErrorCodeMacroValues)
{
  // Verify configuration error codes
  EXPECT_EQ(TUVX_CONFIGURATION_ERROR_CODE_INVALID_KEY, 1);
  EXPECT_EQ(TUVX_CONFIGURATION_ERROR_CODE_MISSING_KEY, 2);
  EXPECT_EQ(TUVX_CONFIGURATION_ERROR_CODE_INVALID_TYPE, 3);
  EXPECT_EQ(TUVX_CONFIGURATION_ERROR_CODE_INVALID_FILE, 4);

  // Verify grid error codes
  EXPECT_EQ(TUVX_GRID_ERROR_CODE_INVALID_BOUNDS, 201);
  EXPECT_EQ(TUVX_GRID_ERROR_CODE_INVALID_SIZE, 202);
  EXPECT_EQ(TUVX_GRID_ERROR_CODE_OUT_OF_RANGE, 203);
  EXPECT_EQ(TUVX_GRID_ERROR_CODE_GRID_NOT_FOUND, 204);

  // Verify profile error codes
  EXPECT_EQ(TUVX_PROFILE_ERROR_CODE_INVALID_SIZE, 301);
  EXPECT_EQ(TUVX_PROFILE_ERROR_CODE_NEGATIVE_VALUES, 302);
  EXPECT_EQ(TUVX_PROFILE_ERROR_CODE_PROFILE_NOT_FOUND, 303);

  // Verify radiator error codes
  EXPECT_EQ(TUVX_RADIATOR_ERROR_CODE_INVALID_WAVELENGTH, 401);
  EXPECT_EQ(TUVX_RADIATOR_ERROR_CODE_INVALID_DATA, 402);
  EXPECT_EQ(TUVX_RADIATOR_ERROR_CODE_RADIATOR_NOT_FOUND, 403);

  // Verify internal error code
  EXPECT_EQ(TUVX_INTERNAL_ERROR_CODE_GENERAL, 901);
}

TEST(ErrorTest, ErrorCategoryStrings)
{
  // Verify error category strings
  EXPECT_STREQ(TUVX_ERROR_CATEGORY_CONFIGURATION, "TUVX Configuration");
  EXPECT_STREQ(TUVX_ERROR_CATEGORY_GRID, "TUVX Grid");
  EXPECT_STREQ(TUVX_ERROR_CATEGORY_PROFILE, "TUVX Profile");
  EXPECT_STREQ(TUVX_ERROR_CATEGORY_RADIATOR, "TUVX Radiator");
  EXPECT_STREQ(TUVX_ERROR_CATEGORY_INTERNAL, "TUVX Internal Error");
}

TEST(InternalErrorTest, ErrorCodeCreation)
{
  std::error_code ec = tuvx::make_error_code(tuvx::TuvxInternalErrc::General);

  EXPECT_EQ(ec.value(), static_cast<int>(tuvx::TuvxInternalErrc::General));
  EXPECT_EQ(ec.category().name(), std::string("TuvxInternalError"));
}

TEST(InternalErrorTest, ErrorMessage)
{
  std::error_code ec = tuvx::make_error_code(tuvx::TuvxInternalErrc::General);
  std::string message = ec.message();

  EXPECT_EQ(message, "General internal error");
}

TEST(InternalErrorTest, ThrowInternalError)
{
  EXPECT_THROW(
      {
        tuvx::ThrowInternalError(tuvx::TuvxInternalErrc::General, "test_file.cpp", 42, "Test error message");
      },
      tuvx::TuvxInternalException);
}

TEST(InternalErrorTest, ExceptionContainsFileAndLine)
{
  try
  {
    tuvx::ThrowInternalError(tuvx::TuvxInternalErrc::General, "test_file.cpp", 42, "Test error message");
    FAIL() << "Expected TuvxInternalException to be thrown";
  }
  catch (const tuvx::TuvxInternalException& e)
  {
    std::string what_str = e.what();

    // Check that file and line are in the message
    EXPECT_NE(what_str.find("test_file.cpp"), std::string::npos) << "Exception message should contain filename";
    EXPECT_NE(what_str.find("42"), std::string::npos) << "Exception message should contain line number";
    EXPECT_NE(what_str.find("Test error message"), std::string::npos) << "Exception message should contain custom message";

    // Check accessor methods
    EXPECT_STREQ(e.file(), "test_file.cpp");
    EXPECT_EQ(e.line(), 42);
    EXPECT_EQ(e.code().value(), static_cast<int>(tuvx::TuvxInternalErrc::General));
  }
}

TEST(InternalErrorTest, InternalErrorMacro)
{
  EXPECT_THROW({ TUVX_INTERNAL_ERROR("Macro test error"); }, tuvx::TuvxInternalException);

  try
  {
    TUVX_INTERNAL_ERROR("Macro test error");
    FAIL() << "Expected exception";
  }
  catch (const tuvx::TuvxInternalException& e)
  {
    std::string what_str = e.what();
    EXPECT_NE(what_str.find("Macro test error"), std::string::npos);
    // The file should be this test file
    EXPECT_NE(what_str.find("test_error.cpp"), std::string::npos);
  }
}

TEST(InternalErrorTest, ExceptionInheritance)
{
  // Verify TuvxInternalException inherits from std::runtime_error
  EXPECT_THROW({ TUVX_INTERNAL_ERROR("Test"); }, std::runtime_error);

  // Also catchable as std::exception
  EXPECT_THROW({ TUVX_INTERNAL_ERROR("Test"); }, std::exception);
}

TEST(InternalErrorTest, ErrorCodeConversion)
{
  // Test automatic conversion from enum to error_code
  std::error_code ec = tuvx::TuvxInternalErrc::General;
  EXPECT_EQ(ec.value(), 1);
  EXPECT_EQ(ec.category().name(), std::string("TuvxInternalError"));
}
