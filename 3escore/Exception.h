//
// Author: Kazys Stepanas
//
#ifndef TES_CORE_EXCEPTION_H
#define TES_CORE_EXCEPTION_H

#include "CoreConfig.h"

#include <string>

namespace tes
{
class Exception : public std::exception
{
public:
  Exception(const char *msg = nullptr, const char *filename = nullptr, int line_number = 0);
  Exception(Exception &&other) noexcept;

  ~Exception() override;

  const char *what() const noexcept override;

  inline Exception &operator=(Exception other)
  {
    other.swap(*this);
    return *this;
  }

  void swap(Exception &other) noexcept;

  inline friend void swap(Exception &a, Exception &b) { a.swap(b); }

private:
  std::string _message;
};
}  // namespace tes

#endif  // TES_CORE_EXCEPTION_H