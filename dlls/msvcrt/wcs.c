/*
 * msvcrt.dll wide-char functions
 *
 * Copyright 1999 Alexandre Julliard
 * Copyright 2000 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include "msvcrt.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msvcrt);


/* INTERNAL: MSVCRT_malloc() based wstrndup */
MSVCRT_wchar_t* msvcrt_wstrndup(const MSVCRT_wchar_t *buf, unsigned int size)
{
  MSVCRT_wchar_t* ret;
  unsigned int len = strlenW(buf), max_len;

  max_len = size <= len? size : len + 1;

  ret = MSVCRT_malloc(max_len * sizeof (MSVCRT_wchar_t));
  if (ret)
  {
    memcpy(ret,buf,max_len * sizeof (MSVCRT_wchar_t));
    ret[max_len] = 0;
  }
  return ret;
}

/*********************************************************************
 *		_wcsdup (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsdup( const MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = NULL;
  if (str)
  {
    int size = (strlenW(str) + 1) * sizeof(MSVCRT_wchar_t);
    ret = MSVCRT_malloc( size );
    if (ret) memcpy( ret, str, size );
  }
  return ret;
}

/*********************************************************************
 *		_wcsicoll (MSVCRT.@)
 */
INT _wcsicoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpiW( str1, str2 );
}

/*********************************************************************
 *		_wcsnset (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsnset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c, MSVCRT_size_t n )
{
  MSVCRT_wchar_t* ret = str;
  while ((n-- > 0) && *str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		_wcsrev (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsrev( MSVCRT_wchar_t* str )
{
  MSVCRT_wchar_t* ret = str;
  MSVCRT_wchar_t* end = str + strlenW(str) - 1;
  while (end > str)
  {
    MSVCRT_wchar_t t = *end;
    *end--  = *str;
    *str++  = t;
  }
  return ret;
}

/*********************************************************************
 *		_wcsset (MSVCRT.@)
 */
MSVCRT_wchar_t* _wcsset( MSVCRT_wchar_t* str, MSVCRT_wchar_t c )
{
  MSVCRT_wchar_t* ret = str;
  while (*str) *str++ = c;
  return ret;
}

/*********************************************************************
 *		wcstod (MSVCRT.@)
 */
double MSVCRT_wcstod(const MSVCRT_wchar_t* lpszStr, MSVCRT_wchar_t** end)
{
  const MSVCRT_wchar_t* str = lpszStr;
  int negative = 0;
  double ret = 0, divisor = 10.0;

  TRACE("(%s,%p) semi-stub\n", debugstr_w(lpszStr), end);

  /* FIXME:
   * - Should set errno on failure
   * - Should fail on overflow
   * - Need to check which input formats are allowed
   */
  while (isspaceW(*str))
    str++;

  if (*str == '-')
  {
    negative = 1;
    str++;
  }

  while (isdigitW(*str))
  {
    ret = ret * 10.0 + (*str - '0');
    str++;
  }
  if (*str == '.')
    str++;
  while (isdigitW(*str))
  {
    ret = ret + (*str - '0') / divisor;
    divisor *= 10;
    str++;
  }

  if (*str == 'E' || *str == 'e' || *str == 'D' || *str == 'd')
  {
    int negativeExponent = 0;
    int exponent = 0;
    if (*(++str) == '-')
    {
      negativeExponent = 1;
      str++;
    }
    while (isdigitW(*str))
    {
      exponent = exponent * 10 + (*str - '0');
      str++;
    }
    if (exponent != 0)
    {
      if (negativeExponent)
        ret = ret / pow(10.0, exponent);
      else
        ret = ret * pow(10.0, exponent);
    }
  }

  if (negative)
    ret = -ret;

  if (end)
    *end = (MSVCRT_wchar_t*)str;

  TRACE("returning %g\n", ret);
  return ret;
}

static int MSVCRT_vsnprintfW(WCHAR *str, size_t len, const WCHAR *format, va_list valist)
{
    unsigned int written = 0;
    const WCHAR *iter = format;
    char bufa[256], fmtbufa[64], *fmta;

    while (*iter)
    {
        while (*iter && *iter != '%')
        {
            if (written++ >= len)
                return -1;
            *str++ = *iter++;
        }
        if (*iter == '%')
        {
            if (iter[1] == '%')
            {
                if (written++ >= len)
                    return -1;
                *str++ = '%'; /* "%%"->'%' */
                iter += 2;
                continue;
            }

            fmta = fmtbufa;
            *fmta++ = *iter++;
            while (*iter == '0' ||
                   *iter == '+' ||
                   *iter == '-' ||
                   *iter == ' ' ||
                   *iter == '*' ||
                   *iter == '#')
            {
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    int fieldlen = va_arg(valist, int);
                    sprintf(buffiter, "%d", fieldlen);
                    while (*buffiter)
                        *fmta++ = *buffiter++;
                }
                else
                    *fmta++ = *iter;
                iter++;
            }

            while (isdigit(*iter))
                *fmta++ = *iter++;

            if (*iter == '.')
            {
                *fmta++ = *iter++;
                if (*iter == '*')
                {
                    char *buffiter = bufa;
                    int fieldlen = va_arg(valist, int);
                    sprintf(buffiter, "%d", fieldlen);
                    while (*buffiter)
                        *fmta++ = *buffiter++;
                }
                else
                    while (isdigit(*iter))
                        *fmta++ = *iter++;
            }
            if (*iter == 'h' || *iter == 'l')
                *fmta++ = *iter++;

            switch (*iter)
            {
            case 'S':
            {
                static const char *none = "(null)";
                const char *astr = va_arg(valist, const char *);
                const char *striter = astr ? astr : none;
                int r, n;
                while (*striter)
                {
                    if (written >= len)
                        return -1;
                    n = 1;
                    if( IsDBCSLeadByte( *striter ) )
                        n++;
                    r = MultiByteToWideChar( CP_ACP, 0,
                               striter, n, str, len - written );
                    striter += n;
                    str += r;
                    written += r;
                }
                iter++;
                break;
            }

            case 's':
            {
                static const WCHAR none[] = { '(','n','u','l','l',')',0 };
                const WCHAR *wstr = va_arg(valist, const WCHAR *);
                const WCHAR *striter = wstr ? wstr : none;
                while (*striter)
                {
                    if (written++ >= len)
                        return -1;
                    *str++ = *striter++;
                }
                iter++;
                break;
            }

            case 'c':
                if (written++ >= len)
                    return -1;
                *str++ = (WCHAR)va_arg(valist, int);
                iter++;
                break;

            default:
            {
                /* For non wc types, use system sprintf and append to wide char output */
                /* FIXME: for unrecognised types, should ignore % when printing */
                char *bufaiter = bufa;
                if (*iter == 'p')
                    sprintf(bufaiter, "%08lX", va_arg(valist, long));
                else
                {
                    *fmta++ = *iter;
                    *fmta = '\0';
                    if (*iter == 'a' || *iter == 'A' ||
                        *iter == 'e' || *iter == 'E' ||
                        *iter == 'f' || *iter == 'F' || 
                        *iter == 'g' || *iter == 'G')
                        sprintf(bufaiter, fmtbufa, va_arg(valist, double));
                    else
                    {
                        /* FIXME: On 32 bit systems this doesn't handle int 64's.
                         *        on 64 bit systems this doesn't work for 32 bit types
			 */
                        sprintf(bufaiter, fmtbufa, va_arg(valist, void *));
                    }
                }
                while (*bufaiter)
                {
                    if (written++ >= len)
                        return -1;
                    *str++ = *bufaiter++;
                }
                iter++;
                break;
            }
            }
        }
    }
    if (written >= len)
        return -1;
    *str++ = 0;
    return (int)written;
}

/*********************************************************************
 *		swprintf (MSVCRT.@)
 */
int MSVCRT_swprintf( MSVCRT_wchar_t *str, const MSVCRT_wchar_t *format, ... )
{
    va_list ap;
    int r;

    va_start( ap, format );
    r = MSVCRT_vsnprintfW( str, INT_MAX, format, ap );
    va_end( ap );
    return r;
}

/*********************************************************************
 *		_vsnwprintf (MSVCRT.@)
 */
int _vsnwprintf(MSVCRT_wchar_t *str, unsigned int len,
                const MSVCRT_wchar_t *format, va_list valist)
{
    return MSVCRT_vsnprintfW(str, len, format, valist);
}

/*********************************************************************
 *		vswprintf (MSVCRT.@)
 */
int MSVCRT_vswprintf( MSVCRT_wchar_t* str, const MSVCRT_wchar_t* format, va_list args )
{
    return MSVCRT_vsnprintfW( str, INT_MAX, format, args );
}

/*********************************************************************
 *		wcscoll (MSVCRT.@)
 */
int MSVCRT_wcscoll( const MSVCRT_wchar_t* str1, const MSVCRT_wchar_t* str2 )
{
  /* FIXME: handle collates */
  return strcmpW( str1, str2 );
}

/*********************************************************************
 *		wcspbrk (MSVCRT.@)
 */
MSVCRT_wchar_t* MSVCRT_wcspbrk( const MSVCRT_wchar_t* str, const MSVCRT_wchar_t* accept )
{
  const MSVCRT_wchar_t* p;
  while (*str)
  {
    for (p = accept; *p; p++) if (*p == *str) return (MSVCRT_wchar_t*)str;
      str++;
  }
  return NULL;
}

/*********************************************************************
 *		wctomb (MSVCRT.@)
 */
INT MSVCRT_wctomb( char *dst, MSVCRT_wchar_t ch )
{
  return WideCharToMultiByte( CP_ACP, 0, &ch, 1, dst, 6, NULL, NULL );
}

/*********************************************************************
 *		iswalnum (MSVCRT.@)
 */
INT MSVCRT_iswalnum( MSVCRT_wchar_t wc )
{
    return isalnumW( wc );
}

/*********************************************************************
 *		iswalpha (MSVCRT.@)
 */
INT MSVCRT_iswalpha( MSVCRT_wchar_t wc )
{
    return isalphaW( wc );
}

/*********************************************************************
 *		iswcntrl (MSVCRT.@)
 */
INT MSVCRT_iswcntrl( MSVCRT_wchar_t wc )
{
    return iscntrlW( wc );
}

/*********************************************************************
 *		iswdigit (MSVCRT.@)
 */
INT MSVCRT_iswdigit( MSVCRT_wchar_t wc )
{
    return isdigitW( wc );
}

/*********************************************************************
 *		iswgraph (MSVCRT.@)
 */
INT MSVCRT_iswgraph( MSVCRT_wchar_t wc )
{
    return isgraphW( wc );
}

/*********************************************************************
 *		iswlower (MSVCRT.@)
 */
INT MSVCRT_iswlower( MSVCRT_wchar_t wc )
{
    return islowerW( wc );
}

/*********************************************************************
 *		iswprint (MSVCRT.@)
 */
INT MSVCRT_iswprint( MSVCRT_wchar_t wc )
{
    return isprintW( wc );
}

/*********************************************************************
 *		iswpunct (MSVCRT.@)
 */
INT MSVCRT_iswpunct( MSVCRT_wchar_t wc )
{
    return ispunctW( wc );
}

/*********************************************************************
 *		iswspace (MSVCRT.@)
 */
INT MSVCRT_iswspace( MSVCRT_wchar_t wc )
{
    return isspaceW( wc );
}

/*********************************************************************
 *		iswupper (MSVCRT.@)
 */
INT MSVCRT_iswupper( MSVCRT_wchar_t wc )
{
    return isupperW( wc );
}

/*********************************************************************
 *		iswxdigit (MSVCRT.@)
 */
INT MSVCRT_iswxdigit( MSVCRT_wchar_t wc )
{
    return isxdigitW( wc );
}
