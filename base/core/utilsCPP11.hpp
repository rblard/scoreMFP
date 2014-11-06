#pragma once

//make_unique is not available in C++11, only in C++14
//cf http://stackoverflow.com/questions/7038357/make-unique-and-perfect-forwarding
//warning: this implementation will not work with VS2012 and earlier
// cf http://stackoverflow.com/questions/12547983/is-there-a-way-to-write-make-unique-in-vs2012
#include <memory>
#if (__cplusplus < 201300L)
namespace std {
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args)
  {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
}
#endif
