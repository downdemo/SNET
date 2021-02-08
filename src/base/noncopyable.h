#pragma once

namespace jc {

// the same as boost::noncopyable
class noncopyable {
 protected:
  constexpr noncopyable() = default;
  ~noncopyable() = default;
  noncopyable(const noncopyable&) = delete;
  noncopyable& operator=(const noncopyable&) = delete;
};

}  // namespace jc
