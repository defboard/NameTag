#pragma once
#include <Print.h>
#include <StreamString.h>

using StreamFormatter = void(Print&);


inline Print& operator<< (Print& out, StreamFormatter f)
{
  f(out);
  return out;
}

inline void endl(Print& out) {
  out.println();
  out.flush();
}

inline char digit(int number)
{
  return '0' + (number % 10);
}

template <class T>
inline Print& operator<< (Print& out, T value)
{
  out.print(value);
  return out;
}

template <class T>
String to_str(const T& obj)
{
  StreamString result;
  result << obj;
  return result;
}

template <class T>
String hex(const T& value)
{
  StreamString result;
  result.print(value, HEX);
  return result;
}

inline const char* checkSuccess(bool success)
{
  return success ? "SUCCESS!" : "FAILED!";
}
