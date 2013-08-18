/*
Copyright 2010 Ed Bow <edxbow@gmail.com>

This file is part of Quake Live - Demo Tools (QLDT).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include <qglobal.h>

#ifdef Q_OS_LINUX
    #define XP_UNIX
#elif defined Q_OS_WIN
    #define XP_WIN
#endif

#include "npupp.h"
#include "jstypes.h"

#if defined(XP_WIN)
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (__stdcall * _name)
#elif defined(XP_OS2)
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (_System * _name)
#else
#define NS_4XPLUGIN_CALLBACK(_type, _name) _type (* _name)
#endif

////////////////////////////////////////////////////////////////////////

// XXX These are defined in platform specific FE directories right now :-/

#if defined(XP_WIN) || defined(XP_UNIX) || defined(XP_BEOS) || defined(XP_OS2)
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_GETENTRYPOINTS) (NPPluginFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGININIT) (const NPNetscapeFuncs* pCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINUNIXINIT) (const NPNetscapeFuncs* pCallbacks,NPPluginFuncs* fCallbacks);
typedef NS_4XPLUGIN_CALLBACK(NPError, NP_PLUGINSHUTDOWN) (void);
#endif


extern "C" {

typedef long jsword;

/* Scalar typedefs. */
typedef uint16    jschar;
typedef int32     jsint;
typedef uint32    jsuint;
//typedef float64   jsdouble;
typedef jsword    jsval;
typedef jsword    jsid;
typedef int32     jsrefcount;   /* PRInt32 if JS_THREADSAFE, see jslock.h */


struct JSString {
    size_t          length;
    union {
        jschar      *chars;
        JSString    *base;
    } u;
};


/*
 * Type tags stored in the low bits of a jsval.
 */
#define JSVAL_OBJECT            0x0     /* untagged reference to object */
#define JSVAL_INT               0x1     /* tagged 31-bit integer value */
#define JSVAL_DOUBLE            0x2     /* tagged reference to double */
#define JSVAL_STRING            0x4     /* tagged reference to string */
#define JSVAL_BOOLEAN           0x6     /* tagged boolean value */

#define JSVAL_SETTAG(v,t)       ((v) | (t))


#define JSVAL_TAGBITS           3
#define JSVAL_TAGMASK           JS_BITMASK(JSVAL_TAGBITS)

#define JSVAL_CLRTAG(v)         ((v) & ~(jsval)JSVAL_TAGMASK)

#define JSVAL_TO_GCTHING(v)     ((void *)JSVAL_CLRTAG(v))
#define JSVAL_TO_STRING(v)      ((JSString *)JSVAL_TO_GCTHING(v))

#define STRING_TO_JSVAL(str)    JSVAL_SETTAG((jsval)(str), JSVAL_STRING)


}
