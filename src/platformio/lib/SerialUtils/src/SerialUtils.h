#ifndef _SERIALUTILS_SERIALUTILS_H
#define _SERIALUTILS_SERIALUTILS_H

#include <StandardCplusplus.h>
#include <utility.h>

template <typename T>
std::pair<bool, unsigned char> GetLastByteFromSerial(T* serial) {
  std::pair<bool, unsigned char> result(false, 0);
  const auto available = serial->available();
  if (available > 0) {
    byte buffer[available];
    serial->readBytes(buffer, available);
    result.second = buffer[available - 1];
    result.first = true;
  }
  return result;
}

#endif // _SERIALUTILS_SERIALUTILS_H
