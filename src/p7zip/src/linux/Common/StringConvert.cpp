// Common/StringConvert.cpp

#include "StdAfx.h"
#include <stdlib.h>

#include "StringConvert.h"
extern "C"
{
int global_use_utf16_conversion = 0;
}

namespace utf8
{
// UTFConvert.cpp

#include "StdAfx.h"

#include "UTFConvert.h"
#include "Types.h"

static const Byte kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

static Bool Utf8_To_Utf16(wchar_t *dest, size_t *destLen, const char *src, size_t srcLen)
{
  size_t destPos = 0, srcPos = 0;
  for (;;)
  {
    Byte c;
    int numAdds;
    if (srcPos == srcLen)
    {
      *destLen = destPos;
      return True;
    }
    c = (Byte)src[srcPos++];

    if (c < 0x80)
    {
      if (dest)
        dest[destPos] = (wchar_t)c;
      destPos++;
      continue;
    }
    if (c < 0xC0)
      break;
    for (numAdds = 1; numAdds < 5; numAdds++)
      if (c < kUtf8Limits[numAdds])
        break;
    UInt32 value = (c - kUtf8Limits[numAdds - 1]);

    do
    {
      Byte c2;
      if (srcPos == srcLen)
        break;
      c2 = (Byte)src[srcPos++];
      if (c2 < 0x80 || c2 >= 0xC0)
        break;
      value <<= 6;
      value |= (c2 - 0x80);
    }
    while (--numAdds != 0);
    
    if (value < 0x10000)
    {
      if (dest)
        dest[destPos] = (wchar_t)value;
      destPos++;
    }
    else
    {
      value -= 0x10000;
      if (value >= 0x100000)
        break;
      if (dest)
      {
        dest[destPos + 0] = (wchar_t)(0xD800 + (value >> 10));
        dest[destPos + 1] = (wchar_t)(0xDC00 + (value & 0x3FF));
      }
      destPos += 2;
    }
  }
  *destLen = destPos;
  return False;
}

static Bool Utf16_To_Utf8(char *dest, size_t *destLen, const wchar_t *src, size_t srcLen)
{
  size_t destPos = 0, srcPos = 0;
  for (;;)
  {
    unsigned numAdds;
    UInt32 value;
    if (srcPos == srcLen)
    {
      *destLen = destPos;
      return True;
    }
    value = src[srcPos++];
    if (value < 0x80)
    {
      if (dest)
        dest[destPos] = (char)value;
      destPos++;
      continue;
    }
    if (value >= 0xD800 && value < 0xE000)
    {
      UInt32 c2;
      if (value >= 0xDC00 || srcPos == srcLen)
        break;
      c2 = src[srcPos++];
      if (c2 < 0xDC00 || c2 >= 0xE000)
        break;
      value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
    }
    for (numAdds = 1; numAdds < 5; numAdds++)
      if (value < (((UInt32)1) << (numAdds * 5 + 6)))
        break;
    if (dest)
      dest[destPos] = (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
    destPos++;
    do
    {
      numAdds--;
      if (dest)
        dest[destPos] = (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
      destPos++;
    }
    while (numAdds != 0);
  }
  *destLen = destPos;
  return False;
}

bool ConvertUTF8ToUnicode(const AString &src, UString &dest)
{
  dest.Empty();
  size_t destLen = 0;
  Utf8_To_Utf16(NULL, &destLen, src, src.Length());
  wchar_t *p = dest.GetBuffer((int)destLen);
  Bool res = Utf8_To_Utf16(p, &destLen, src, src.Length());
  p[destLen] = 0;
  dest.ReleaseBuffer();
  return res ? true : false;
}

bool ConvertUnicodeToUTF8(const UString &src, AString &dest)
{
  dest.Empty();
  size_t destLen = 0;
  Utf16_To_Utf8(NULL, &destLen, src, src.Length());
  char *p = dest.GetBuffer((int)destLen);
  Bool res = Utf16_To_Utf8(p, &destLen, src, src.Length());
  p[destLen] = 0;
  dest.ReleaseBuffer();
  return res ? true : false;
}

}


#ifdef LOCALE_IS_UTF8

UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    UString resultString;
    bool bret = utf8::ConvertUTF8ToUnicode(srcString,resultString);
    if (bret) return resultString;
  }

  UString resultString;
  for (int i = 0; i < srcString.Length(); i++)
    resultString += wchar_t(srcString[i] & 255);

  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    AString resultString;
    bool bret = utf8::ConvertUnicodeToUTF8(srcString,resultString);
    if (bret) return resultString;
  }

  AString resultString;
  for (int i = 0; i < srcString.Length(); i++)
  {
    if (srcString[i] >= 256) resultString += '?';
    else                     resultString += char(srcString[i]);
  }
  return resultString;
}

#else /* LOCALE_IS_UTF8 */

UString MultiByteToUnicodeString(const AString &srcString, UINT codePage)
{
#ifdef HAVE_MBSTOWCS
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    UString resultString;
    int numChars = mbstowcs(resultString.GetBuffer(srcString.Length()),srcString,srcString.Length()+1);
    if (numChars >= 0) {
      resultString.ReleaseBuffer(numChars);
      return resultString;
    }
  }
#endif

  UString resultString;
  for (int i = 0; i < srcString.Length(); i++)
    resultString += wchar_t(srcString[i] & 255);

  return resultString;
}

AString UnicodeStringToMultiByte(const UString &srcString, UINT codePage)
{
#ifdef HAVE_WCSTOMBS
  if ((global_use_utf16_conversion) && (!srcString.IsEmpty()))
  {
    AString resultString;
    int numRequiredBytes = srcString.Length() * 6+1;
    int numChars = wcstombs(resultString.GetBuffer(numRequiredBytes),srcString,numRequiredBytes);
    if (numChars >= 0) {
      resultString.ReleaseBuffer(numChars);
      return resultString;
    }
  }
#endif

  AString resultString;
  for (int i = 0; i < srcString.Length(); i++)
  {
    if (srcString[i] >= 256) resultString += '?';
    else                     resultString += char(srcString[i]);
  }
  return resultString;
}

#endif /* LOCALE_IS_UTF8 */

