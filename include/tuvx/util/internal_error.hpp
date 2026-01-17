#pragma once

#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

namespace tuvx
{
  /// @brief Error codes for internal TUV-x errors
  enum class TuvxInternalErrc
  {
    General = 1
  };

  /// @brief Error category for TUV-x internal errors
  class TuvxInternalErrorCategory : public std::error_category
  {
   public:
    /// @brief Get the name of this error category
    /// @return Category name
    const char* name() const noexcept override
    {
      return "TuvxInternalError";
    }

    /// @brief Get the error message for a given error code
    /// @param ev Error code value
    /// @return Human-readable error message
    std::string message(int ev) const override
    {
      switch (static_cast<TuvxInternalErrc>(ev))
      {
        case TuvxInternalErrc::General: return "General internal error";
        default: return "Unknown error";
      }
    }
  };

  /// @brief Get the singleton instance of the TUV-x internal error category
  /// @return Reference to the error category
  inline const TuvxInternalErrorCategory& GetTuvxInternalErrorCategory()
  {
    static TuvxInternalErrorCategory category;
    return category;
  }

  /// @brief Create an error code from a TuvxInternalErrc value
  /// @param e Error code enum value
  /// @return std::error_code instance
  inline std::error_code make_error_code(TuvxInternalErrc e)
  {
    return { static_cast<int>(e), GetTuvxInternalErrorCategory() };
  }

  /// @brief Exception class for TUV-x internal errors
  class TuvxInternalException : public std::runtime_error
  {
   public:
    /// @brief Construct an internal exception with file/line information
    /// @param code Error code
    /// @param file Source file where the error occurred
    /// @param line Line number where the error occurred
    /// @param msg Additional error message
    TuvxInternalException(TuvxInternalErrc code, const char* file, int line, const char* msg)
        : std::runtime_error(FormatMessage(code, file, line, msg)),
          error_code_(make_error_code(code)),
          file_(file),
          line_(line)
    {
    }

    /// @brief Get the error code associated with this exception
    /// @return Error code
    std::error_code code() const
    {
      return error_code_;
    }

    /// @brief Get the source file where the error occurred
    /// @return File path
    const char* file() const
    {
      return file_;
    }

    /// @brief Get the line number where the error occurred
    /// @return Line number
    int line() const
    {
      return line_;
    }

   private:
    std::error_code error_code_;
    const char* file_;
    int line_;

    static std::string FormatMessage(TuvxInternalErrc code, const char* file, int line, const char* msg)
    {
      std::ostringstream oss;
      oss << "[" << file << ":" << line << "] " << GetTuvxInternalErrorCategory().message(static_cast<int>(code));
      if (msg && msg[0] != '\0')
      {
        oss << ": " << msg;
      }
      return oss.str();
    }
  };

  /// @brief Throw an internal error with file/line information
  /// @param e Error code
  /// @param file Source file
  /// @param line Line number
  /// @param msg Error message
  [[noreturn]] inline void ThrowInternalError(TuvxInternalErrc e, const char* file, int line, const char* msg)
  {
    throw TuvxInternalException(e, file, line, msg);
  }

}  // namespace tuvx

// Enable automatic conversion of TuvxInternalErrc to std::error_code
template<>
struct std::is_error_code_enum<tuvx::TuvxInternalErrc> : std::true_type
{
};

/// @brief Macro to throw an internal error with current file/line information
/// @param msg Error message string
#define TUVX_INTERNAL_ERROR(msg) tuvx::ThrowInternalError(tuvx::TuvxInternalErrc::General, __FILE__, __LINE__, msg)
