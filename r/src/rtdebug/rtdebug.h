/*
  libmedio - C++ I/O Library for loading/saving medical data formats
             https://github.com/hzdr-MedImaging/libmedio

  Copyright (C) 2004-2026 hzdr.de and contributors

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef RMEDIO_RTDEBUG_H
#define RMEDIO_RTDEBUG_H

// Replacement of the rtdebug.h header of librtdebug
// (https://github.com/hzdr-MedImaging/librtdebug) for the R package.
//
// The bundled libmedio sources are always compiled as a release build in
// Rmedio. There, all tracing macros of librtdebug (ENTER(), RETURN(), D(),
// W(), E(), ASSERT(), ...) expand to nothing, so the R package neither needs
// librtdebug nor writes to stdout/stderr or calls abort(), which R packages
// must not do. The info macros that librtdebug also keeps in release builds
// are routed to the R console via REprintf().

// rtdebug.h of librtdebug includes <iostream> through CRTDebug.h; keep it
// so that the libmedio sources see the same declarations.
#include <iostream>

#include <R_ext/Print.h>

#if defined(ENTER)
#undef ENTER
#endif
#if defined(LEAVE)
#undef LEAVE
#endif
#if defined(RETURN)
#undef RETURN
#endif
#if defined(SHOWVALUE)
#undef SHOWVALUE
#endif
#if defined(SHOWPOINTER)
#undef SHOWPOINTER
#endif
#if defined(SHOWSTRING)
#undef SHOWSTRING
#endif
#if defined(SHOWMSG)
#undef SHOWMSG
#endif
#if defined(STARTCLOCK)
#undef STARTCLOCK
#endif
#if defined(STOPCLOCK)
#undef STOPCLOCK
#endif
#if defined(D)
#undef D
#endif
#if defined(E)
#undef E
#endif
#if defined(W)
#undef W
#endif
#if defined(ASSERT)
#undef ASSERT
#endif

// tracing and debug macros: no-ops as in a release build of librtdebug
#define ENTER()             (void(0))
#define LEAVE()             (void(0))
#define RETURN(r)           (void(0))
#define SHOWVALUE(v)        (void(0))
#define SHOWPOINTER(p)      (void(0))
#define SHOWSTRING(s)       (void(0))
#define SHOWMSG(m)          (void(0))
#define STARTCLOCK(s)       (void(0))
#define STOPCLOCK(s)        (void(0))
#define D(...)              (void(0))
#define E(...)              (void(0))
#define W(...)              (void(0))
#define DN(...)             (void(0))
#define EN(...)             (void(0))
#define WN(...)             (void(0))
#define ASSERT(expression)  (void(0))

// info messages that librtdebug also prints in release builds
#define RMEDIO_RTDEBUG_PRINT(newline, ...) (REprintf(__VA_ARGS__), (newline) ? REprintf("\n") : (void)0)

#define Info(...)     RMEDIO_RTDEBUG_PRINT(true, __VA_ARGS__)
#define Verbose(...)  RMEDIO_RTDEBUG_PRINT(true, __VA_ARGS__)
#define Warning(...)  RMEDIO_RTDEBUG_PRINT(true, __VA_ARGS__)
#define Error(...)    RMEDIO_RTDEBUG_PRINT(true, __VA_ARGS__)
#define Fatal(...)    RMEDIO_RTDEBUG_PRINT(true, __VA_ARGS__)
#define Debug(...)    (void(0))

#define InfoN(...)    RMEDIO_RTDEBUG_PRINT(false, __VA_ARGS__)
#define VerboseN(...) RMEDIO_RTDEBUG_PRINT(false, __VA_ARGS__)
#define WarningN(...) RMEDIO_RTDEBUG_PRINT(false, __VA_ARGS__)
#define ErrorN(...)   RMEDIO_RTDEBUG_PRINT(false, __VA_ARGS__)
#define FatalN(...)   RMEDIO_RTDEBUG_PRINT(false, __VA_ARGS__)
#define DebugN(...)   (void(0))

#endif // RMEDIO_RTDEBUG_H
