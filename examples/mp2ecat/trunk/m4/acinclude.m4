dnl aclocal.m4 generated automatically by aclocal 1.4-p6

dnl Copyright (C) 1994, 1995-8, 1999, 2001 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

dnl This program is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY, to the extent permitted by law; without
dnl even the implied warranty of MERCHANTABILITY or FITNESS FOR A
dnl PARTICULAR PURPOSE.


dnl Autoconf Macros from gnu.org

AC_DEFUN([AC_CXX_NAMESPACES],
[AC_CACHE_CHECK(whether the compiler implements namespaces,
ac_cv_cxx_namespaces,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([namespace Outer { namespace Inner { int i = 0; }}],
                [using namespace Outer::Inner; return i;],
 ac_cv_cxx_namespaces=yes, ac_cv_cxx_namespaces=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_namespaces" = yes; then
  AC_DEFINE(HAVE_NAMESPACES,,[define if the compiler implements namespaces])
fi
])

AC_DEFUN([AC_CXX_STATIC_CAST],
[AC_CACHE_CHECK(whether the compiler supports static_cast<>,
ac_cv_cxx_static_cast,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <typeinfo>
class Base { public : Base () {} virtual void f () = 0; };
class Derived : public Base { public : Derived () {} virtual void f () {} };
int g (Derived&) { return 0; }],[
Derived d; Base& b = d; Derived& s = static_cast<Derived&> (b); return g (s);],
 ac_cv_cxx_static_cast=yes, ac_cv_cxx_static_cast=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_cxx_static_cast" = yes; then
  AC_DEFINE(HAVE_STATIC_CAST,,
            [define if the compiler supports static_cast<>])
fi
])

dnl Copyright (c) 2003 Jens Langner

AC_DEFUN(AC_ANSI_COLOR,
[
	AC_MSG_CHECKING(whether ANSI color should be used for terminal output)
	AC_ARG_ENABLE(ansi-color,
								[AC_HELP_STRING([--disable-ansi-color], [ansi-color terminal output should be disabled [default=no]])],
								[case "${enableval}" in
									yes) test_on_ansi_color=yes ;;
									no)  test_on_ansi_color=no ;;
									*)   AC_MSG_ERROR(bad value ${enableval} for --disable-ansi-color) ;;
								esac], 
								[test_on_ansi_color=no])

	if test "$test_on_ansi_color" = "yes"; then
		AC_MSG_RESULT(no)
	else
		ANSI_COLOR="ansi_color" 
		AC_DEFINE(WITH_ANSI_COLOR) 		
		AC_MSG_RESULT(yes)
	fi
	AC_DEFINE([WITH_ANSI_COLOR], [], [Use ANSI color scheme in terminal output])
	
	AC_SUBST(ANSI_COLOR) 
])

# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-debug)
AC_DEFUN(AC_ENABLE_DEBUG,
[
	AC_MSG_CHECKING(whether to enable debugging)
	AC_ARG_ENABLE(debug,
								[AC_HELP_STRING([--enable-debug], [turn on debugging mode [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_debug=yes	;;
									no)	 test_on_enable_debug=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
								esac],
								[test_on_enable_debug=no])

	if test "$test_on_enable_debug" = "yes"; then
		COMPILE_LEVEL="debug"
		AC_MSG_RESULT(yes)
	else
		COMPILE_LEVEL="release"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(COMPILE_LEVEL) 
])

# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-static-qt)
AC_DEFUN(AC_ENABLE_STATIC_QT,
[
	AC_MSG_CHECKING(whether to link the QT library static)
	AC_ARG_ENABLE(static-qt,
								[AC_HELP_STRING([--enable-static-qt], [turn on static linking of QT libs [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_qt=yes	;;
									no)	 test_on_enable_static_qt=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-qt) ;;
								esac],
								[test_on_enable_static_qt=no])

	if test "$test_on_enable_static_qt" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticqt"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

dnl
dnl AC_ENABLE_STATIC_RTDEBUG: provides a switch to control the link level (static/shared)
dnl of a linked rtdebug library
dnl
AC_DEFUN(AC_ENABLE_STATIC_RTDEBUG,
[
	AC_MSG_CHECKING(whether to link the rtdebug library static)
	AC_ARG_ENABLE(static-rtdebug,
								[AC_HELP_STRING([--enable-static-rtdebug], [turn on static linking of rtdebug lib [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_rtdebug=yes	;;
									no)	 test_on_enable_static_rtdebug=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-rtdebug) ;;
								esac],
								[test_on_enable_static_rtdebug=no])

	if test "$test_on_enable_static_rtdebug" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticrtdebug"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-static-medio)
AC_DEFUN(AC_ENABLE_STATIC_MEDIO,
[
	AC_MSG_CHECKING(whether to link the medio library static)
	AC_ARG_ENABLE(static-medio,
								[AC_HELP_STRING([--enable-static-medio], [turn on static linking of medio libs [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_medio=yes	;;
									no)	 test_on_enable_static_medio=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-medio) ;;
								esac],
								[test_on_enable_static_medio=no])

	if test "$test_on_enable_static_medio" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticmedio"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-static-medrecon)
AC_DEFUN(AC_ENABLE_STATIC_MEDRECON,
[
	AC_MSG_CHECKING(whether to link the medrecon library static)
	AC_ARG_ENABLE(static-medrecon,
								[AC_HELP_STRING([--enable-static-medrecon], [turn on static linking of medrecon libs [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_medrecon=yes	;;
									no)	 test_on_enable_static_medrecon=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-medrecon) ;;
								esac],
								[test_on_enable_static_medrecon=no])

	if test "$test_on_enable_static_medrecon" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticmedrecon"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])


# this macro is used to get the arguments supplied
# to the configure script (./configure --enable-static-gsl)
AC_DEFUN(AC_ENABLE_STATIC_GSL,
[
	AC_MSG_CHECKING(whether to link the gsl library static)
	AC_ARG_ENABLE(static-gsl,
								[AC_HELP_STRING([--enable-static-gsl], [turn on static linking of gsl libs [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_gsl=yes	;;
									no)	 test_on_enable_static_gsl=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-gsl) ;;
								esac],
								[test_on_enable_static_gsl=no])

	if test "$test_on_enable_static_gsl" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticgsl"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

AC_DEFUN(AC_PATH_GSL_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(gsl-lib,
              [AC_HELP_STRING([--with-gsl-lib], [where the GSL library is located.])],
							[ac_gsl_libraries="$withval"], ac_gsl_libraries="")

  AC_MSG_CHECKING(for GNU Scientific library)

  AC_CACHE_VAL(ac_cv_lib_gsllib, [

  gsl_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_gsl_libraries"; then
    gsl_library_dirs="$gsl_library_dirs \
		                  /usr/local/lib \
											/usr/local/lib/gsl \
		                  /usr/lib \
		                  /usr/lib/gsl \
		                  /Developer/gsl/lib"
  else
    gsl_library_dirs="$ac_gsl_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  gsl_found="0"
  ac_gsl_libdir=
  ac_gsl_libname="-lgsl -lgslcblas"
  
  LIBS="$ac_gsl_libname $save_LIBS"
  for gsl_dir in $gsl_library_dirs; do
    LDFLAGS="-L$gsl_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [gsl_found="1"], [gsl_found="0"])
    if test $gsl_found = 1; then
      ac_gsl_libdir="$gsl_dir"
      break;
    else
      echo "tried $gsl_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_gsllib="ac_gsl_libname=\"$ac_gsl_libname\" ac_gsl_libdir=\"$ac_gsl_libdir\""
  ])

  eval "$ac_cv_lib_gsllib"

  dnl Define a shell variable for later checks
  if test -z "$ac_gsl_libdir"; then
    have_gsl_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required GNU Scientific (libgsl) library in linker path.
Try --with-gsl-lib to specify the path, manually.])
  else
    have_gsl_lib="yes"
    AC_MSG_RESULT([yes, $ac_gsl_libname in $ac_gsl_libdir found.])
  fi

  GSL_LDFLAGS="-L$ac_gsl_libdir"
  GSL_LIBDIR="$ac_gsl_libdir"
  LIB_GSL="$ac_gsl_libname"
  AC_SUBST(GSL_LDFLAGS)
  AC_SUBST(GSL_LIBDIR)
  AC_SUBST(LIB_GSL)
])

AC_DEFUN(AC_PATH_GSL_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for libgsl includes)

  AC_ARG_WITH(gsl-inc,
              [AC_HELP_STRING([--with-gsl-inc], [where the libgsl headers are located.])],
              [gsl_include_dirs="$withval"], gsl_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_gslinc, [

    dnl Did the user give --with-gsl-includes?
    if test -z "$gsl_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      gsl_include_dirs="\
        /usr/local/gsl/include \
        /usr/include/gsl \
        /usr/lib/gsl/include \
        /usr/local/include/gsl"
    fi

    for gsl_dir in $gsl_include_dirs; do
      if test -r "$gsl_dir/gsl_version.h"; then
        if test -r "$gsl_dir/gsl_types.h"; then
          ac_gsl_includes=$gsl_dir
          break;
        fi
      fi
    done

    ac_cv_header_gslinc=$ac_gsl_includes

  ])

  if test -z "$ac_cv_header_gslinc"; then
    have_gsl_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([libgsl include directory not found, you may run into problems.
Try --with-gsl-inc to specify the path, manually.])
  else
    have_gsl_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_gslinc])
  fi

  GSL_INCLUDES="-I$ac_cv_header_gslinc"
  GSL_INCDIR="$ac_cv_header_gslinc"
  AC_SUBST(GSL_INCLUDES)
  AC_SUBST(GSL_INCDIR)
])

dnl
dnl AC_PATH_RTDEBUG_LIB: checks for the existance of the rtdebug library in the
dnl default pathes and allows to override them as well
dnl
AC_DEFUN(AC_PATH_RTDEBUG_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(rtdebug-lib,
              [AC_HELP_STRING([--with-rtdebug-lib], [where the librtdebug library is located.])],
							[ac_rtdebug_libraries="$withval"], ac_rtdebug_libraries="")

  AC_MSG_CHECKING(for runtime debugging library)

  AC_CACHE_VAL(ac_cv_lib_rtdebuglib, [

  rtdebug_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_rtdebug_libraries"; then
    rtdebug_library_dirs="$rtdebug_library_dirs \
												/usr/local/petlib/lib \
												/usr/local/petlib/lib/rtdebug \	
		                    /usr/local/lib \
												/usr/local/lib/rtdebug \
		                    /usr/lib \
		                    /usr/lib/rtdebug \
		                    /Developer/rtdebug/lib"
  else
    rtdebug_library_dirs="$ac_rtdebug_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  rtdebug_found="0"
  ac_rtdebug_libdir=
  ac_rtdebug_libname="-lrtdebug"
  
  LIBS="$ac_rtdebug_libname $save_LIBS"
  for rtdebug_dir in $rtdebug_library_dirs; do
    LDFLAGS="-L$rtdebug_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [rtdebug_found="1"], [rtdebug_found="0"])
    if test $rtdebug_found = 1; then
      ac_rtdebug_libdir="$rtdebug_dir"
      break;
    else
      echo "tried $rtdebug_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_rtdebuglib="ac_rtdebug_libname=$ac_rtdebug_libname ac_rtdebug_libdir=$ac_rtdebug_libdir"
  ])

  eval "$ac_cv_lib_rtdebuglib"

  dnl Define a shell variable for later checks
  if test -z "$ac_rtdebug_libdir"; then
    have_rtdebug_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required runtime debugging library in linker path.
Try --with-rtdebug-lib to specify the path, manually.])
  else
    have_rtdebug_lib="yes"
    AC_MSG_RESULT([yes, $ac_rtdebug_libname in $ac_rtdebug_libdir found.])
  fi

  RTDEBUG_LDFLAGS="-L$ac_rtdebug_libdir"
  RTDEBUG_LIBDIR="$ac_rtdebug_libdir"
  LIB_RTDEBUG="$ac_rtdebug_libname"
  AC_SUBST(RTDEBUG_LDFLAGS)
  AC_SUBST(RTDEBUG_LIBDIR)
  AC_SUBST(LIB_RTDEBUG)
])

dnl
dnl AC_PATH_RTDEBUG_INC: checks the existance of the includes files for successfully
dnl compiling support for the rtdebug library and also allows to override the default
dnl path to that includes.
dnl
AC_DEFUN(AC_PATH_RTDEBUG_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for librtdebug includes)

  AC_ARG_WITH(rtdebug-inc,
              [AC_HELP_STRING([--with-rtdebug-inc], [where the librtdebug headers are located.])],
              [rtdebug_include_dirs="$withval"], rtdebug_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_rtdebuginc, [

    dnl Did the user give --with-rtdebug-includes?
    if test -z "$rtdebug_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      rtdebug_include_dirs="\
			  /usr/local/petlib/include \
				/usr/local/petlib/include/rtdebug \		
        /usr/local/rtdebug/include \
        /usr/include/rtdebug \
        /usr/lib/rtdebug/include \
        /usr/local/include/rtdebug"
    fi

    for rtdebug_dir in $rtdebug_include_dirs; do
      if test -r "$rtdebug_dir/CRTDebug.h"; then
        if test -r "$rtdebug_dir/rtdebug.h"; then
          ac_rtdebug_includes=$rtdebug_dir
          break;
        fi
      fi
    done

    ac_cv_header_rtdebuginc=$ac_rtdebug_includes

  ])

  if test -z "$ac_cv_header_rtdebuginc"; then
    have_rtdebug_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([librtdebug include directory not found, you may run into problems.
Try --with-rtdebug-inc to specify the path, manually.])
  else
    have_rtdebug_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_rtdebuginc])
  fi

  RTDEBUG_INCLUDES="-I$ac_cv_header_rtdebuginc"
  RTDEBUG_INCDIR="$ac_cv_header_rtdebuginc"
  AC_SUBST(RTDEBUG_INCLUDES)
  AC_SUBST(RTDEBUG_INCDIR)
])

AC_DEFUN(AC_PATH_MEDIO_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(medio-lib,
              [AC_HELP_STRING([--with-medio-lib], [where the libmedio library is located.])],
							[ac_medio_libraries="$withval"], ac_medio_libraries="")

  AC_MSG_CHECKING(for medical IO library)

  AC_CACHE_VAL(ac_cv_lib_mediolib, [

  medio_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_medio_libraries"; then
    medio_library_dirs="$medio_library_dirs \
		                    /usr/local/lib \
												/usr/local/lib/medio \
		                    /usr/lib \
		                    /usr/lib/medio \
		                    /Developer/medio/lib"
  else
    medio_library_dirs="$ac_medio_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  medio_found="0"
  ac_medio_libdir=
  ac_medio_libname="-lmedio"
  
  LIBS="$ac_medio_libname $save_LIBS"
  for medio_dir in $medio_library_dirs; do
    LDFLAGS="-L$medio_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [medio_found="1"], [medio_found="0"])
    if test $medio_found = 1; then
      ac_medio_libdir="$medio_dir"
      break;
    else
      echo "tried $medio_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_mediolib="ac_medio_libname=$ac_medio_libname ac_medio_libdir=$ac_medio_libdir"
  ])

  eval "$ac_cv_lib_mediolib"

  dnl Define a shell variable for later checks
  if test -z "$ac_medio_libdir"; then
    have_medio_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required medical IO (libmedio) library in linker path.
Try --with-medio-lib to specify the path, manually.])
  else
    have_medio_lib="yes"
    AC_MSG_RESULT([yes, $ac_medio_libname in $ac_medio_libdir found.])
  fi

  MEDIO_LDFLAGS="-L$ac_medio_libdir"
  MEDIO_LIBDIR="$ac_medio_libdir"
  LIB_MEDIO="$ac_medio_libname"
  AC_SUBST(MEDIO_LDFLAGS)
  AC_SUBST(MEDIO_LIBDIR)
  AC_SUBST(LIB_MEDIO)
])

AC_DEFUN(AC_PATH_MEDRECON_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(medrecon-lib,
              [AC_HELP_STRING([--with-medrecon-lib], [where the libmedrecon library is located.])],
							[ac_medrecon_libraries="$withval"], ac_medrecon_libraries="")

  AC_MSG_CHECKING(for medical reconstruction library)

  AC_CACHE_VAL(ac_cv_lib_medreconlib, [

  medrecon_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_medrecon_libraries"; then
    medrecon_library_dirs="$medrecon_library_dirs \
		                    /usr/local/lib \
												/usr/local/lib/medrecon \
		                    /usr/lib \
		                    /usr/lib/medrecon \
		                    /Developer/medrecon/lib"
  else
    medrecon_library_dirs="$ac_medrecon_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  medrecon_found="0"
  ac_medrecon_libdir=
  ac_medrecon_libname="-lmedrecon"
  
  LIBS="$ac_medrecon_libname $save_LIBS"
  for medrecon_dir in $medrecon_library_dirs; do
    LDFLAGS="-L$medrecon_dir $save_LDFLAGS -L$MEDIO_LIBDIR $LIB_MEDIO"
    AC_TRY_LINK_FUNC(main, [medrecon_found="1"], [medrecon_found="0"])
    if test $medrecon_found = 1; then
      ac_medrecon_libdir="$medrecon_dir"
      break;
    else
      echo "tried $medrecon_dir"
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_medreconlib="ac_medrecon_libname=$ac_medrecon_libname ac_medrecon_libdir=$ac_medrecon_libdir"
  ])

  eval "$ac_cv_lib_medreconlib"

  dnl Define a shell variable for later checks
  if test -z "$ac_medrecon_libdir"; then
    have_medrecon_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required medical reconstruction (libmedrecon) library in linker path.
Try --with-medrecon-lib to specify the path, manually.])
  else
    have_medrecon_lib="yes"
    AC_MSG_RESULT([yes, $ac_medrecon_libname in $ac_medrecon_libdir found.])
  fi

  MEDRECON_LDFLAGS="-L$ac_medrecon_libdir"
  MEDRECON_LIBDIR="$ac_medrecon_libdir"
  LIB_MEDRECON="$ac_medrecon_libname"
  AC_SUBST(MEDRECON_LDFLAGS)
  AC_SUBST(MEDRECON_LIBDIR)
  AC_SUBST(LIB_MEDRECON)
])


AC_DEFUN(AC_PATH_MEDIO_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for libmedio includes)

  AC_ARG_WITH(medio-inc,
              [AC_HELP_STRING([--with-medio-inc], [where the libmedio headers are located.])],
              [medio_include_dirs="$withval"], medio_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_medioinc, [

    dnl Did the user give --with-medio-includes?
    if test -z "$medio_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      medio_include_dirs="\
        /usr/local/medio/include \
        /usr/include/medio \
        /usr/lib/medio/include \
        /usr/local/include/medio"
    fi

    for medio_dir in $medio_include_dirs; do
      if test -r "$medio_dir/CECATFile.h"; then
        if test -r "$medio_dir/CECATDirectory.h"; then
          ac_medio_includes=$medio_dir
          break;
        fi
      fi
    done

    ac_cv_header_medioinc=$ac_medio_includes

  ])

  if test -z "$ac_cv_header_medioinc"; then
    have_medio_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([libmedio include directory not found, you may run into problems.
Try --with-medio-inc to specify the path, manually.])
  else
    have_medio_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_medioinc])
  fi

  MEDIO_INCLUDES="-I$ac_cv_header_medioinc"
  MEDIO_INCDIR="$ac_cv_header_medioinc"
  AC_SUBST(MEDIO_INCLUDES)
  AC_SUBST(MEDIO_INCDIR)
])



AC_DEFUN(AC_PATH_MEDRECON_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for libmedrecon includes)

  AC_ARG_WITH(medrecon-inc,
              [AC_HELP_STRING([--with-medrecon-inc], [where the libmedrecon headers are located.])],
              [medrecon_include_dirs="$withval"], medrecon_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_medreconinc, [

    dnl Did the user give --with-medrecon-includes?
    if test -z "$medrecon_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      medrecon_include_dirs="\
        /usr/local/medrecon/include \
        /usr/include/medrecon \
        /usr/lib/medrecon/include \
        /usr/local/include/medrecon"
    fi

    for medrecon_dir in $medrecon_include_dirs; do
      dnl if test -r "$medrecon_dir/CReconstructionFactory.h"; then
        dnl if test -r "$medrecon_dir/CReconstruction.h"; then
          ac_medrecon_includes=$medrecon_dir
          dnl break;
        dnl fi
      dnl fi
    done

    ac_cv_header_medreconinc=$ac_medrecon_includes

  ])

  if test -z "$ac_cv_header_medreconinc"; then
    have_medrecon_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([libmedrecon include directory not found, you may run into problems.
Try --with-medrecon-inc to specify the path, manually.])
  else
    have_medrecon_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_medreconinc])
  fi

  MEDRECON_INCLUDES="-I$ac_cv_header_medreconinc"
  MEDRECON_INCDIR="$ac_cv_header_medreconinc"
  AC_SUBST(MEDRECON_INCLUDES)
  AC_SUBST(MEDRECON_INCDIR)
])

AC_DEFUN(AC_PATH_STIR_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(stir-lib,
              [AC_HELP_STRING([--with-stir-lib], [where the stir library is located.])],
							[ac_stir_libraries="$withval"], ac_stir_libraries="")

  AC_MSG_CHECKING(for stir library)

  AC_CACHE_VAL(ac_cv_lib_stirlib, [

  stir_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_stir_libraries"; then
    stir_library_dirs="$stir_library_dirs \
		                    /usr/local/lib \
												/usr/local/lib/stir \
		                    /usr/lib \
		                    /usr/lib/stir \
		                    /Developer/stir/lib"
  else
    stir_library_dirs="$ac_stir_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  stir_found="0"
  ac_stir_libdir=
  ac_stir_libname="-lstir"
  
  LIBS="$ac_stir_libname $save_LIBS"
  for stir_dir in $stir_library_dirs; do
    LDFLAGS="-L$stir_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [stir_found="1"], [stir_found="0"])
    if test $stir_found = 1; then
      ac_stir_libdir="$stir_dir"
      break;
    else
      echo "tried $stir_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_stirlib="ac_stir_libname=$ac_stir_libname ac_stir_libdir=$ac_stir_libdir"
  ])

  eval "$ac_cv_lib_stirlib"

  dnl Define a shell variable for later checks
  if test -z "$ac_stir_libdir"; then
    have_stir_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required stir library in linker path.
Try --with-stir-lib to specify the path, manually.])
  else
    have_stir_lib="yes"
    AC_MSG_RESULT([yes, $ac_stir_libname in $ac_stir_libdir found.])
  fi

  STIR_LDFLAGS="-L$ac_stir_libdir"
  STIR_LIBDIR="$ac_stir_libdir"
  LIB_STIR="$ac_stir_libname"
  AC_SUBST(STIR_LDFLAGS)
  AC_SUBST(STIR_LIBDIR)
  AC_SUBST(LIB_STIR)
])

AC_DEFUN(AC_PATH_STIR_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for libstir includes)

  AC_ARG_WITH(stir-inc,
              [AC_HELP_STRING([--with-stir-inc], [where the stir headers are located.])],
              [stir_include_dirs="$withval"], stir_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_stirinc, [

    dnl Did the user give --with-stir-includes?
    if test -z "$stir_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      stir_include_dirs="\
        /usr/local/stir/include \
        /usr/include/stir \
        /usr/lib/stir/include \
        /usr/local/include/stir"
    fi

    for stir_dir in $stir_include_dirs; do
      if test -r "$stir_dir/Array.h"; then
        if test -r "$stir_dir/Array.inl"; then
          ac_stir_includes=$stir_dir
          break;
        fi
      fi
    done

    ac_cv_header_stirinc=$ac_stir_includes

  ])

  if test -z "$ac_cv_header_stirinc"; then
    have_stir_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([libstir include directory not found, you may run into problems.
Try --with-stir-inc to specify the path, manually.])
  else
    have_stir_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_stirinc])
  fi

  STIR_INCLUDES="-I$ac_cv_header_stirinc"
  STIR_INCDIR="$ac_cv_header_stirinc"
  AC_SUBST(STIR_INCLUDES)
  AC_SUBST(STIR_INCDIR)
])

AC_DEFUN(AC_PATH_STIRBOOST_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for libstir boost includes)

  AC_ARG_WITH(stir-boost-inc,
              [AC_HELP_STRING([--with-stir-boost-inc], [where the stirboost headers are located.])],
              [stirboost_include_dirs="$withval"], stirboost_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_stirboostinc, [

    dnl Did the user give --with-stir-boost-includes?
    if test -z "$stirboost_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      stirboost_include_dirs="\
        /usr/local/stir/include \
        /usr/include/boost \
        /usr/lib/stir/include \
        /usr/local/include/boost"
    fi

    for stirboost_dir in $stirboost_include_dirs; do
      if test -r "$stirboost_dir/any.hpp"; then
        if test -r "$stirboost_dir/cstdint.hpp"; then
          ac_stirboost_includes=$stirboost_dir
          break;
        fi
      fi
    done

    ac_cv_header_stirboostinc=$ac_stirboost_includes

  ])

  if test -z "$ac_cv_header_stirboostinc"; then
    have_stirboost_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([libstir boost include directory not found, you may run into problems.
Try --with-stir-boost-inc to specify the path, manually.])
  else
    have_stirboost_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_stirboostinc])
  fi

  STIRBOOST_INCLUDES="-I$ac_cv_header_stirboostinc"
  STIRBOOST_INCDIR="$ac_cv_header_stirboostinc"
  AC_SUBST(STIRBOOST_INCLUDES)
  AC_SUBST(STIRBOOST_INCDIR)
])

dnl
dnl AC_ENABLE_STATIC_LIB: if the project is a library this macro can be used to control
dnl if the library should be build as a shared or static library
dnl
AC_DEFUN(AC_ENABLE_STATIC_LIB,
[
	AC_MSG_CHECKING(whether to link as a static library)
	AC_ARG_ENABLE(static-lib,
								[AC_HELP_STRING([--enable-static-lib], [turn on static linking [default=no]])],
								[case "${enableval}" in
									yes) test_on_enable_static_lib=yes	;;
									no)  test_on_enable_static_lib=no	;;
									*)	 AC_MSG_ERROR(bad value ${enableval} for --enable-static-lib) ;;
								esac],
								[test_on_enable_static_lib=no])

	if test "$test_on_enable_static_lib" = "yes"; then
		QTLINK_LEVEL="${QTLINK_LEVEL} staticlib"
		AC_MSG_RESULT(yes)
	else
		QTLINK_LEVEL="${QTLINK_LEVEL}"
		AC_MSG_RESULT(no)
	fi

	AC_SUBST(QTLINK_LEVEL) 
])

#
# find out the used gcc version and validate that this one is
# a working one for lmmc
# 
AC_DEFUN(AC_PROG_GCC_VERSION,[

 AC_MSG_CHECKING([for gcc version])

 dnl check if $CC exists or not
 if eval $CC -v 2>/dev/null >/dev/null; then
   dnl Check if version of gcc is sufficient
	 cc_name=`( $CC -v ) 2>&1 | tail -n 1 | cut -d ' ' -f 1`
	 cc_version=`( $CC -dumpversion ) 2>&1`
	 if test "$?" -gt 0; then
		 cc_version="not found"
	 fi
	 changequote(,)dnl	 
	 case $cc_version in
	   '')
		   cc_version="v. ?.??, bad"
			 cc_verc_fail=yes
			 ;;
		 2.95.[2-9]|2.95.[2-9][-.]*|3.[0-9]*|3.[0-9].[0-9]*|4.[0-9]*|4.[0-9].[0-9]*)
			 _cc_major=`echo $cc_version | cut -d '.' -f 1`
			 _cc_minor=`echo $cc_version | cut -d '.' -f 2`
			 _cc_mini=`echo $cc_version | cut -d '.' -f 3`
			 cc_version="$cc_version, ok"
			 cc_verc_fail=no
		   ;;
		 'not found')
		   cc_verc_fail=yes
			 ;;
		 *)
		   cc_version="$cc_version, bad"
			 cc_verc_fail=yes
			 ;;
	 esac
	 changequote([, ])dnl

	 if test "$cc_verc_fail" = yes ; then
	   AC_MSG_RESULT([$cc_version])
		 AC_MSG_ERROR([gcc version check failed])
   else
 	   AC_MSG_RESULT([$cc_version])
		 GCC_VERSION=$_cc_major
   fi
 else
   AC_MSG_RESULT(FAILED)
   AC_MSG_ERROR([gcc was not found '$CC'])
 fi

 AC_SUBST(GCC_VERSION)
])

AC_DEFUN(AC_QTSHAREDBUILD,
[
  AC_REQUIRE_CPP()
  AC_REQUIRE([AC_PATH_QT_INC])
  AC_REQUIRE([AC_PATH_QT_LIB])

  AC_MSG_CHECKING(for qSharedBuild in lib qt-mt)
  
  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
 
  ac_cv_lib_qtlib="ac_qt_libname=$ac_qt_libname ac_qt_libdir=$ac_qt_libdir"
  dnl Save some global vars
  save_CXXFLAGS="$CXXFLAGS"
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  LIBS="$ac_qt_libname $save_LIBS"
  LDFLAGS="-L$ac_qt_libdir $save_LDFLAGS"
  CXXFLAGS="-I$ac_cv_header_qtinc $save_CXXFLAGS"

  AC_TRY_LINK([#include <qglobal.h>], qSharedBuild();, ac_have_qsharedbuild="yes", ac_have_qsharedbuild="no")

  dnl Define a shell variable for later checks
  if test "$ac_have_qsharedbuild" = "no"; then
    AC_MSG_RESULT([no])
  else
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_QSHAREDBUILD], [], [Is this a shared build of QT])
  fi

  dnl Restore the saved vars
  CXXFLAGS="$save_CXXFLAGS"
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  AC_LANG_RESTORE
])

AC_DEFUN(AC_PATH_QTDIR,
[
  AC_ARG_WITH(qtdir,
              [AC_HELP_STRING([--with-qtdir], [set the global QTDIR variable manually.])],
              [QTDIR="$withval" ])

  if test -z "$QTDIR"; then
    AC_MSG_WARN([environment variable QTDIR is not set, you may run into problems])
  fi

])

AC_DEFUN(AC_PATH_QT3_LIB,
[
  AC_REQUIRE_CPP()
  dnl AC_REQUIRE([AC_PATH_X])
  AC_ARG_WITH(qt-lib,
                          [AC_HELP_STRING([--with-qt-lib], [where the QT multithreaded library is located.])],
                      [ac_qt_libraries="$withval"], ac_qt_libraries="")

  AC_MSG_CHECKING(for QT multithreaded library)

  AC_CACHE_VAL(ac_cv_lib_qtlib, [

  qt_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_qt_libraries"; then
    if test -n "$QTDIR"; then
      qt_library_dirs="$QTDIR/lib $qt_library_dirs"
    fi

    if test -n "$QTLIB"; then
      qt_library_dirs="$QTLIB $qt_library_dirs"
    fi

    qt_library_dirs="$qt_library_dirs \
                     /usr/qt/lib \
                     /usr/local/lib/qt \
                     /usr/lib/qt3/lib \
                     /usr/lib \
                     /usr/local/lib \
                     /usr/lib/qt \
                     /usr/lib/qt/lib \
                     /usr/local/lib/qt \
                     /usr/X11/lib \
                     /usr/X11/lib/qt \
                     /usr/X11R6/lib \
                     /usr/X11R6/lib/qt \
                     /Developer/qt/lib"
  else
    qt_library_dirs="$ac_qt_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  qt_found="0"
  ac_qt_libdir=
  ac_qt_libname="-lqt-mt"
  
  LIBS="$ac_qt_libname $save_LIBS"
  for qt_dir in $qt_library_dirs; do
    dnl echo $qt_dir;
    LDFLAGS="-L$qt_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [qt_found="1"], [qt_found="0"])
    dnl AC_TRY_RUN([#include <qglobal.h>
    dnl int 
    dnl main()
    dnl {
    dnl  main();
    dnl  ;
    dnl  return QT_VERSION;
    dnl }])
    if test $qt_found = 1; then
      ac_qt_libdir="$qt_dir"
      break;
    else
      echo "tried $qt_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_qtlib="ac_qt_libname=$ac_qt_libname ac_qt_libdir=$ac_qt_libdir"
  ])

  eval "$ac_cv_lib_qtlib"

  dnl Define a shell variable for later checks
  if test -z "$ac_qt_libdir"; then
    have_qt_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required QT multithreaded library in linker path.
Try --with-qt-lib to specify the path, manualy.])
  else
    have_qt_lib="yes"
    AC_MSG_RESULT([yes, $ac_qt_libname in $ac_qt_libdir found.])
  fi

  QT_LDFLAGS="-L$ac_qt_libdir"
  QT_LIBDIR="$ac_qt_libdir"
  LIB_QT="$ac_qt_libname"
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_LIBDIR)
  AC_SUBST(LIB_QT)
])

AC_DEFUN(AC_PATH_QT3_INC,
[
  AC_REQUIRE_CPP()
  AC_REQUIRE([AC_PATH_X])
  AC_MSG_CHECKING(for QT includes)

  AC_ARG_WITH(qt-inc,
                          [AC_HELP_STRING([--with-qt-inc], [where the QT headers are located.])],
                      [qt_include_dirs="$withval"], qt_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_qtinc, [

    dnl Did the user give --with-qt-includes?
    if test -z "$qt_include_dirs"; then

      dnl No they didn't, so lets look for them...
      dnl If you need to add extra directories to check, add them here.
      qt_include_dirs="\
        /usr/lib/qt3/include \
        /usr/lib/qt2/include \
        /usr/lib/qt/include \
        /usr/include/qt \
        /usr/local/qt/include \
        /usr/local/include/qt \
        /usr/X11/include/qt \
        /usr/X11/include/X11/qt \
        /usr/X11R6/include \
        /usr/X11R6/include/qt \
        /usr/X11R6/include/X11/qt \
        /usr/X11/lib/qt/include"

      if test -n "$QTDIR"; then
        qt_include_dirs="$QTDIR/include $qt_include_dirs"
      fi

      if test -n "$QTINC"; then
        qt_include_dirs="$QTINC $qt_include_dirs"
      fi
    fi

    for qt_dir in $qt_include_dirs; do
      if test -r "$qt_dir/qt.h"; then
        if test -r "$qt_dir/qglobal.h"; then
          if test -r "$qt_dir/qbig5codec.h"; then
            if test -r "$qt_dir/qtranslatordialog.h"; then
              AC_MSG_ERROR([
                This is not Qt 2.1.0 or later. Somebody cheated you.

                Most likely this is because you've installed a crappy
                outdated Redhat 6.2 RPM. Go to ftp://people.redhat.com/bero/qt
                and update to the correct one.
              ])
            fi
          fi
          ac_qt_includes=$qt_dir
          break;
        fi
      fi
    done

    ac_cv_header_qtinc=$ac_qt_includes

  ])

  if test -z "$ac_cv_header_qtinc"; then
    have_qt_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([Qt include directory not found, you may run into problems.
Try --with-qt-inc to specify the path, manualy.])
  else
    have_qt_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_qtinc])
  fi

  QT_INCLUDES="-I$ac_cv_header_qtinc"
  QT_INCDIR="$ac_cv_header_qtinc"
  AC_SUBST(QT_INCLUDES)
  AC_SUBST(QT_INCDIR)
])

dnl
dnl AC_PATH_QT3_QMAKE: tries to find out if the "qmake" binary of Qt3 is reachable or not and
dnl allows to override the default path to it
dnl
AC_DEFUN(AC_PATH_QT3_QMAKE,
[
  AC_ARG_WITH(qt3-qmake,[AC_HELP_STRING([--with-qt3-qmake], [where the Qt3 qmake binary is located.])],
                        [ac_qt_qmake="$withval"], ac_qt_qmake="")

  if test -z "$ac_qt_qmake"; then
    dnl search on our own

    if test -z "$QTDIR"; then
      AC_MSG_WARN(environment variable QTDIR is not set, qmake might not be found)
    fi

    AC_PATH_PROG(
      QMAKE_PATH,
      qmake,
      $QTDIR/bin/qmake,
      $QTDIR/bin:/usr/lib/qt2/bin:/usr/bin:/usr/X11R6/bin:/usr/lib/qt/bin:/usr/local/qt/bin:/Developer/qt/bin:$PATH
    )
  else
    AC_MSG_CHECKING(for qmake)

    if test -f $ac_qt_qmake && test -x $ac_qt_qmake; then
      QMAKE_PATH=$ac_qt_qmake
    else
      AC_MSG_ERROR(
        --with-qt3-qmake expects path and name of the qmake tool
      )
    fi

    AC_MSG_RESULT($QMAKE_PATH)
  fi

  if test -z "$QMAKE_PATH"; then
    AC_MSG_ERROR(couldn't find Qt3 qmake. Please use --with-qt3-qmake)
  fi

  dnl Check if we have the right qmake by outputing the version
	dnl information
	qmake_vers=`"$QMAKE_PATH" -v 2>&1 | grep "Qt 3"`
  if test -z "$qmake_vers"; then
    AC_MSG_ERROR([didn't find the correct Qt3 version of qmake, Please use --with-qt3-qmake])
  fi

  AC_SUBST(QMAKE_PATH)
])

AC_DEFUN(AC_PATH_QT3_MOC,
[
  AC_ARG_WITH(qt-moc,
                          [AC_HELP_STRING([--with-qt-moc], [where the QT 3.1.x moc is located.])],
                      [ac_qt_moc="$withval"], ac_qt_moc="")

  if test -z "$ac_qt_moc"; then
    dnl search on our own

    if test -z "$QTDIR"; then
      AC_MSG_WARN(environment variable QTDIR is not set, moc might not be found)
    fi

    AC_PATH_PROG(
      MOC,
      moc,
      $QTDIR/bin/moc,
      $QTDIR/bin:/usr/lib/qt2/bin:/usr/bin:/usr/X11R6/bin:/usr/lib/qt/bin:/usr/local/qt/bin:$PATH
    )
  else
    AC_MSG_CHECKING(for moc)

    if test -f $ac_qt_moc && test -x $ac_qt_moc; then
      MOC=$ac_qt_moc
    else
      AC_MSG_ERROR(
        --with-qt-moc expects path and name of the moc
      )
    fi

    AC_MSG_RESULT($MOC)
  fi

  if test -z "$MOC"; then
    AC_MSG_ERROR(couldn't find Qt moc. Please use --with-qt-moc)
  fi

  dnl Check if we have the right moc
  if ! fgrep QCString "$MOC" > /dev/null; then
    AC_MSG_ERROR([

        The Qt meta object compiler (moc)
        $MOC
        found by configure is not the one part of Qt 2.1.x.

        It's likely that the found one is the one shipped with
        Qt 1.xx. That one will not work.

        Please check your installation.
        Use the --with-qt-moc option to specify the path and name
        of the moc compiler shipped with your Qt 2.1 lib.
        Some distributions rename it to "moc2", maybe you find that
        on your system.

        see ./configure --help for details.
    ])
  fi

  AC_SUBST(MOC)
])

dnl
dnl AC_PATH_QT4_LIB: checks if the Qt4 libraries are reachable and provides means
dnl of overriding the default search path
dnl
AC_DEFUN(AC_PATH_QT4_LIB,
[
  AC_REQUIRE_CPP()
  AC_ARG_WITH(qt4-lib,[AC_HELP_STRING([--with-qt4-lib], [where the Qt4 libraries are located.])],
                      [ac_qt_libraries="$withval"], ac_qt_libraries="")

  AC_MSG_CHECKING(for Qt4 libraries)

  AC_CACHE_VAL(ac_cv_lib_qtlib, [

  qt_libdir=

  dnl No they didnt, so lets look for them...
  dnl If you need to add extra directories to check, add them here.
  if test -z "$ac_qt_libraries"; then
    qt_library_dirs="/usr/lib/qt4 \
										 /usr/local/qt4/lib \
										 /usr/local/qt/lib \
                     /usr/local/lib/qt4 \
                     /usr/local/lib/qt \
                     /usr/lib \
                     /usr/local/lib \
                     /usr/lib/qt \
                     /usr/lib/qt/lib \
                     /usr/local/lib/qt \
                     /usr/X11/lib \
                     /usr/X11/lib/qt \
                     /usr/X11R6/lib \
                     /usr/X11R6/lib/qt \
										 /Developer/qt4/lib
                     /Developer/qt/lib"
  else
    qt_library_dirs="$ac_qt_libraries"
  fi

  dnl Save some global vars
  save_LDFLAGS="$LDFLAGS"
  save_LIBS="$LIBS"

  qt_found="0"
  ac_qt_libdir=
  ac_qt_libname="-lQtCore"
  
  LIBS="$ac_qt_libname $save_LIBS"
  for qt_dir in $qt_library_dirs; do
    LDFLAGS="-L$qt_dir $save_LDFLAGS"
    AC_TRY_LINK_FUNC(main, [qt_found="1"], [qt_found="0"])
    if test $qt_found = 1; then
      ac_qt_libdir="$qt_dir"
      break;
    else
      echo "tried $qt_dir" >&AC_FD_CC 
    fi
  done

  dnl Restore the saved vars
  LDFLAGS="$save_LDFLAGS"
  LIBS="$save_LIBS"

  ac_cv_lib_qtlib="ac_qt_libname=$ac_qt_libname ac_qt_libdir=$ac_qt_libdir"
  ])

  eval "$ac_cv_lib_qtlib"

  dnl Define a shell variable for later checks
  if test -z "$ac_qt_libdir"; then
    have_qt_lib="no"
    AC_MSG_RESULT([no])
    AC_MSG_ERROR([Cannot find required Qt4 libraries in linker path. Try --with-qt4-lib to specify the path, manualy.])
  else
    have_qt_lib="yes"
    AC_MSG_RESULT([yes, $ac_qt_libname in $ac_qt_libdir found.])
  fi

  QT_LDFLAGS="-L$ac_qt_libdir"
  QT_LIBDIR="$ac_qt_libdir"
  LIB_QT="$ac_qt_libname"
  AC_SUBST(QT_LDFLAGS)
  AC_SUBST(QT_LIBDIR)
  AC_SUBST(LIB_QT)
])

dnl
dnl AC_PATH_QT4_INC: checks for the existance of the Qt4 includes and provides means
dnl to override the default search pathes to it
dnl
AC_DEFUN(AC_PATH_QT4_INC,
[
  AC_REQUIRE_CPP()
  AC_MSG_CHECKING(for Qt4 includes)

  AC_ARG_WITH(qt4-inc, [AC_HELP_STRING([--with-qt4-inc], [where the Qt4 includes are located.])],
                      [qt_include_dirs="$withval"], qt_include_dirs="")

  AC_CACHE_VAL(ac_cv_header_qtinc, [

    dnl Did the user give --with-qt-includes?
    if test -z "$qt_include_dirs"; then
      qt_include_dirs="\
			  /usr/include/qt4/ \
				/usr/local/qt4/include \
        /usr/lib/qt/include \
        /usr/include/qt \
        /usr/local/qt/include \
        /usr/local/include/qt \
        /usr/X11/include/qt \
        /usr/X11/include/X11/qt \
        /usr/X11R6/include \
        /usr/X11R6/include/qt \
        /usr/X11R6/include/X11/qt \
        /usr/X11/lib/qt/include"
    fi

		dnl now we do check for the QtCore subdir and the Qt include
    for qt_dir in $qt_include_dirs; do
      if test -r "$qt_dir/QtCore"; then
        if test -r "$qt_dir/QtCore/Qt"; then
          ac_qt_includes=$qt_dir
          break;
        fi
      fi
    done

    ac_cv_header_qtinc=$ac_qt_includes

  ])

  if test -z "$ac_cv_header_qtinc"; then
    have_qt_inc="no"
    AC_MSG_RESULT([no])
    AC_MSG_WARN([Qt4 include directory not found, you may run into problems. Try --with-qt4-inc to specify the path, manualy.])
  else
    have_qt_inc="yes"
    AC_MSG_RESULT([yes, in $ac_cv_header_qtinc])
  fi

  QT_INCLUDES="-I$ac_cv_header_qtinc"
  QT_INCDIR="$ac_cv_header_qtinc"
  AC_SUBST(QT_INCLUDES)
  AC_SUBST(QT_INCDIR)
])

dnl
dnl AC_PATH_QT4_QMAKE: tries to find out if the "qmake" binary of Qt4 is reachable or not and
dnl allows to override the default path to it
dnl
AC_DEFUN(AC_PATH_QT4_QMAKE,
[
  AC_ARG_WITH(qt4-qmake,[AC_HELP_STRING([--with-qt4-qmake], [where the Qt4 qmake binary is located.])],
                        [ac_qt_qmake="$withval"], ac_qt_qmake="")

  if test -z "$ac_qt_qmake"; then
    AC_PATH_PROG(
      QMAKE_PATH,
      qmake,
      qmake,
      /usr/lib/qt4/bin:/usr/bin:/usr/X11R6/bin:/usr/lib/qt/bin:/usr/local/qt/bin:/Developer/qt4/bin:$PATH
    )
  else
    AC_MSG_CHECKING(for qmake)

    if test -f $ac_qt_qmake && test -x $ac_qt_qmake; then
      QMAKE_PATH=$ac_qt_qmake
    else
      AC_MSG_ERROR(
        --with-qt4-qmake expects path and name of the qmake tool
      )
    fi

    AC_MSG_RESULT($QMAKE_PATH)
  fi

  if test -z "$QMAKE_PATH"; then
    AC_MSG_ERROR(couldn't find Qt4 qmake. Please use --with-qt4-qmake)
  fi

  dnl Check if we have the right qmake by outputing the version
	dnl information
	qmake_vers=`"$QMAKE_PATH" -v 2>&1 | grep "Qt version 4"`
  if test -z "$qmake_vers"; then
    AC_MSG_ERROR([didn't find the correct Qt4 version of qmake, Please use --with-qt4-qmake])
  fi

  AC_SUBST(QMAKE_PATH)
])


# lib-prefix.m4 serial 3 (gettext-0.13)
dnl Copyright (C) 2001-2003 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

dnl AC_LIB_ARG_WITH is synonymous to AC_ARG_WITH in autoconf-2.13, and
dnl similar to AC_ARG_WITH in autoconf 2.52...2.57 except that is doesn't
dnl require excessive bracketing.
ifdef([AC_HELP_STRING],
[AC_DEFUN([AC_LIB_ARG_WITH], [AC_ARG_WITH([$1],[[$2]],[$3],[$4])])],
[AC_DEFUN([AC_][LIB_ARG_WITH], [AC_ARG_WITH([$1],[$2],[$3],[$4])])])

dnl AC_LIB_PREFIX adds to the CPPFLAGS and LDFLAGS the flags that are needed
dnl to access previously installed libraries. The basic assumption is that
dnl a user will want packages to use other packages he previously installed
dnl with the same --prefix option.
dnl This macro is not needed if only AC_LIB_LINKFLAGS is used to locate
dnl libraries, but is otherwise very convenient.
AC_DEFUN([AC_LIB_PREFIX],
[
  AC_BEFORE([$0], [AC_LIB_LINKFLAGS])
  AC_REQUIRE([AC_PROG_CC])
  AC_REQUIRE([AC_CANONICAL_HOST])
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  dnl By default, look in $includedir and $libdir.
  use_additional=yes
  AC_LIB_WITH_FINAL_PREFIX([
    eval additional_includedir=\"$includedir\"
    eval additional_libdir=\"$libdir\"
  ])
  AC_LIB_ARG_WITH([lib-prefix],
[  --with-lib-prefix[=DIR] search for libraries in DIR/include and DIR/lib
  --without-lib-prefix    don't search for libraries in includedir and libdir],
[
    if test "X$withval" = "Xno"; then
      use_additional=no
    else
      if test "X$withval" = "X"; then
        AC_LIB_WITH_FINAL_PREFIX([
          eval additional_includedir=\"$includedir\"
          eval additional_libdir=\"$libdir\"
        ])
      else
        additional_includedir="$withval/include"
        additional_libdir="$withval/lib"
      fi
    fi
])
  if test $use_additional = yes; then
    dnl Potentially add $additional_includedir to $CPPFLAGS.
    dnl But don't add it
    dnl   1. if it's the standard /usr/include,
    dnl   2. if it's already present in $CPPFLAGS,
    dnl   3. if it's /usr/local/include and we are using GCC on Linux,
    dnl   4. if it doesn't exist as a directory.
    if test "X$additional_includedir" != "X/usr/include"; then
      haveit=
      for x in $CPPFLAGS; do
        AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
        if test "X$x" = "X-I$additional_includedir"; then
          haveit=yes
          break
        fi
      done
      if test -z "$haveit"; then
        if test "X$additional_includedir" = "X/usr/local/include"; then
          if test -n "$GCC"; then
            case $host_os in
              linux*) haveit=yes;;
            esac
          fi
        fi
        if test -z "$haveit"; then
          if test -d "$additional_includedir"; then
            dnl Really add $additional_includedir to $CPPFLAGS.
            CPPFLAGS="${CPPFLAGS}${CPPFLAGS:+ }-I$additional_includedir"
          fi
        fi
      fi
    fi
    dnl Potentially add $additional_libdir to $LDFLAGS.
    dnl But don't add it
    dnl   1. if it's the standard /usr/lib,
    dnl   2. if it's already present in $LDFLAGS,
    dnl   3. if it's /usr/local/lib and we are using GCC on Linux,
    dnl   4. if it doesn't exist as a directory.
    if test "X$additional_libdir" != "X/usr/lib"; then
      haveit=
      for x in $LDFLAGS; do
        AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
        if test "X$x" = "X-L$additional_libdir"; then
          haveit=yes
          break
        fi
      done
      if test -z "$haveit"; then
        if test "X$additional_libdir" = "X/usr/local/lib"; then
          if test -n "$GCC"; then
            case $host_os in
              linux*) haveit=yes;;
            esac
          fi
        fi
        if test -z "$haveit"; then
          if test -d "$additional_libdir"; then
            dnl Really add $additional_libdir to $LDFLAGS.
            LDFLAGS="${LDFLAGS}${LDFLAGS:+ }-L$additional_libdir"
          fi
        fi
      fi
    fi
  fi
])

dnl AC_LIB_PREPARE_PREFIX creates variables acl_final_prefix,
dnl acl_final_exec_prefix, containing the values to which $prefix and
dnl $exec_prefix will expand at the end of the configure script.
AC_DEFUN([AC_LIB_PREPARE_PREFIX],
[
  dnl Unfortunately, prefix and exec_prefix get only finally determined
  dnl at the end of configure.
  if test "X$prefix" = "XNONE"; then
    acl_final_prefix="$ac_default_prefix"
  else
    acl_final_prefix="$prefix"
  fi
  if test "X$exec_prefix" = "XNONE"; then
    acl_final_exec_prefix='${prefix}'
  else
    acl_final_exec_prefix="$exec_prefix"
  fi
  acl_save_prefix="$prefix"
  prefix="$acl_final_prefix"
  eval acl_final_exec_prefix=\"$acl_final_exec_prefix\"
  prefix="$acl_save_prefix"
])

dnl AC_LIB_WITH_FINAL_PREFIX([statement]) evaluates statement, with the
dnl variables prefix and exec_prefix bound to the values they will have
dnl at the end of the configure script.
AC_DEFUN([AC_LIB_WITH_FINAL_PREFIX],
[
  acl_save_prefix="$prefix"
  prefix="$acl_final_prefix"
  acl_save_exec_prefix="$exec_prefix"
  exec_prefix="$acl_final_exec_prefix"
  $1
  exec_prefix="$acl_save_exec_prefix"
  prefix="$acl_save_prefix"
])

# lib-link.m4 serial 4 (gettext-0.12)
dnl Copyright (C) 2001-2003 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl From Bruno Haible.

dnl AC_LIB_LINKFLAGS(name [, dependencies]) searches for libname and
dnl the libraries corresponding to explicit and implicit dependencies.
dnl Sets and AC_SUBSTs the LIB${NAME} and LTLIB${NAME} variables and
dnl augments the CPPFLAGS variable.
AC_DEFUN([AC_LIB_LINKFLAGS],
[
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])
  define([Name],[translit([$1],[./-], [___])])
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])
  AC_CACHE_CHECK([how to link with lib[]$1], [ac_cv_lib[]Name[]_libs], [
    AC_LIB_LINKFLAGS_BODY([$1], [$2])
    ac_cv_lib[]Name[]_libs="$LIB[]NAME"
    ac_cv_lib[]Name[]_ltlibs="$LTLIB[]NAME"
    ac_cv_lib[]Name[]_cppflags="$INC[]NAME"
  ])
  LIB[]NAME="$ac_cv_lib[]Name[]_libs"
  LTLIB[]NAME="$ac_cv_lib[]Name[]_ltlibs"
  INC[]NAME="$ac_cv_lib[]Name[]_cppflags"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INC]NAME)
  AC_SUBST([LIB]NAME)
  AC_SUBST([LTLIB]NAME)
  dnl Also set HAVE_LIB[]NAME so that AC_LIB_HAVE_LINKFLAGS can reuse the
  dnl results of this search when this library appears as a dependency.
  HAVE_LIB[]NAME=yes
  undefine([Name])
  undefine([NAME])
])

dnl AC_LIB_HAVE_LINKFLAGS(name, dependencies, includes, testcode)
dnl searches for libname and the libraries corresponding to explicit and
dnl implicit dependencies, together with the specified include files and
dnl the ability to compile and link the specified testcode. If found, it
dnl sets and AC_SUBSTs HAVE_LIB${NAME}=yes and the LIB${NAME} and
dnl LTLIB${NAME} variables and augments the CPPFLAGS variable, and
dnl #defines HAVE_LIB${NAME} to 1. Otherwise, it sets and AC_SUBSTs
dnl HAVE_LIB${NAME}=no and LIB${NAME} and LTLIB${NAME} to empty.
AC_DEFUN([AC_LIB_HAVE_LINKFLAGS],
[
  AC_REQUIRE([AC_LIB_PREPARE_PREFIX])
  AC_REQUIRE([AC_LIB_RPATH])
  define([Name],[translit([$1],[./-], [___])])
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])

  dnl Search for lib[]Name and define LIB[]NAME, LTLIB[]NAME and INC[]NAME
  dnl accordingly.
  AC_LIB_LINKFLAGS_BODY([$1], [$2])

  dnl Add $INC[]NAME to CPPFLAGS before performing the following checks,
  dnl because if the user has installed lib[]Name and not disabled its use
  dnl via --without-lib[]Name-prefix, he wants to use it.
  ac_save_CPPFLAGS="$CPPFLAGS"
  AC_LIB_APPENDTOVAR([CPPFLAGS], [$INC]NAME)

  AC_CACHE_CHECK([for lib[]$1], [ac_cv_lib[]Name], [
    ac_save_LIBS="$LIBS"
    LIBS="$LIBS $LIB[]NAME"
    AC_TRY_LINK([$3], [$4], [ac_cv_lib[]Name=yes], [ac_cv_lib[]Name=no])
    LIBS="$ac_save_LIBS"
  ])
  if test "$ac_cv_lib[]Name" = yes; then
    HAVE_LIB[]NAME=yes
    AC_DEFINE([HAVE_LIB]NAME, 1, [Define if you have the $1 library.])
    AC_MSG_CHECKING([how to link with lib[]$1])
    AC_MSG_RESULT([$LIB[]NAME])
  else
    HAVE_LIB[]NAME=no
    dnl If $LIB[]NAME didn't lead to a usable library, we don't need
    dnl $INC[]NAME either.
    CPPFLAGS="$ac_save_CPPFLAGS"
    LIB[]NAME=
    LTLIB[]NAME=
  fi
  AC_SUBST([HAVE_LIB]NAME)
  AC_SUBST([LIB]NAME)
  AC_SUBST([LTLIB]NAME)
  undefine([Name])
  undefine([NAME])
])

dnl Determine the platform dependent parameters needed to use rpath:
dnl libext, shlibext, hardcode_libdir_flag_spec, hardcode_libdir_separator,
dnl hardcode_direct, hardcode_minus_L.
AC_DEFUN([AC_LIB_RPATH],
[
  AC_REQUIRE([AC_PROG_CC])                dnl we use $CC, $GCC, $LDFLAGS
  AC_REQUIRE([AC_LIB_PROG_LD])            dnl we use $LD, $with_gnu_ld
  AC_REQUIRE([AC_CANONICAL_HOST])         dnl we use $host
  AC_REQUIRE([AC_CONFIG_AUX_DIR_DEFAULT]) dnl we use $ac_aux_dir
  AC_CACHE_CHECK([for shared library run path origin], acl_cv_rpath, [
    CC="$CC" GCC="$GCC" LDFLAGS="$LDFLAGS" LD="$LD" with_gnu_ld="$with_gnu_ld" \
    ${CONFIG_SHELL-/bin/sh} "$ac_aux_dir/config.rpath" "$host" > conftest.sh
    . ./conftest.sh
    rm -f ./conftest.sh
    acl_cv_rpath=done
  ])
  wl="$acl_cv_wl"
  libext="$acl_cv_libext"
  shlibext="$acl_cv_shlibext"
  hardcode_libdir_flag_spec="$acl_cv_hardcode_libdir_flag_spec"
  hardcode_libdir_separator="$acl_cv_hardcode_libdir_separator"
  hardcode_direct="$acl_cv_hardcode_direct"
  hardcode_minus_L="$acl_cv_hardcode_minus_L"
  dnl Determine whether the user wants rpath handling at all.
  AC_ARG_ENABLE(rpath,
    [  --disable-rpath         do not hardcode runtime library paths],
    :, enable_rpath=yes)
])

dnl AC_LIB_LINKFLAGS_BODY(name [, dependencies]) searches for libname and
dnl the libraries corresponding to explicit and implicit dependencies.
dnl Sets the LIB${NAME}, LTLIB${NAME} and INC${NAME} variables.
AC_DEFUN([AC_LIB_LINKFLAGS_BODY],
[
  define([NAME],[translit([$1],[abcdefghijklmnopqrstuvwxyz./-],
                               [ABCDEFGHIJKLMNOPQRSTUVWXYZ___])])
  dnl By default, look in $includedir and $libdir.
  use_additional=yes
  AC_LIB_WITH_FINAL_PREFIX([
    eval additional_includedir=\"$includedir\"
    eval additional_libdir=\"$libdir\"
  ])
  AC_LIB_ARG_WITH([lib$1-prefix],
[  --with-lib$1-prefix[=DIR]  search for lib$1 in DIR/include and DIR/lib
  --without-lib$1-prefix     don't search for lib$1 in includedir and libdir],
[
    if test "X$withval" = "Xno"; then
      use_additional=no
    else
      if test "X$withval" = "X"; then
        AC_LIB_WITH_FINAL_PREFIX([
          eval additional_includedir=\"$includedir\"
          eval additional_libdir=\"$libdir\"
        ])
      else
        additional_includedir="$withval/include"
        additional_libdir="$withval/lib"
      fi
    fi
])
  dnl Search the library and its dependencies in $additional_libdir and
  dnl $LDFLAGS. Using breadth-first-seach.
  LIB[]NAME=
  LTLIB[]NAME=
  INC[]NAME=
  rpathdirs=
  ltrpathdirs=
  names_already_handled=
  names_next_round='$1 $2'
  while test -n "$names_next_round"; do
    names_this_round="$names_next_round"
    names_next_round=
    for name in $names_this_round; do
      already_handled=
      for n in $names_already_handled; do
        if test "$n" = "$name"; then
          already_handled=yes
          break
        fi
      done
      if test -z "$already_handled"; then
        names_already_handled="$names_already_handled $name"
        dnl See if it was already located by an earlier AC_LIB_LINKFLAGS
        dnl or AC_LIB_HAVE_LINKFLAGS call.
        uppername=`echo "$name" | sed -e 'y|abcdefghijklmnopqrstuvwxyz./-|ABCDEFGHIJKLMNOPQRSTUVWXYZ___|'`
        eval value=\"\$HAVE_LIB$uppername\"
        if test -n "$value"; then
          if test "$value" = yes; then
            eval value=\"\$LIB$uppername\"
            test -z "$value" || LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$value"
            eval value=\"\$LTLIB$uppername\"
            test -z "$value" || LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }$value"
          else
            dnl An earlier call to AC_LIB_HAVE_LINKFLAGS has determined
            dnl that this library doesn't exist. So just drop it.
            :
          fi
        else
          dnl Search the library lib$name in $additional_libdir and $LDFLAGS
          dnl and the already constructed $LIBNAME/$LTLIBNAME.
          found_dir=
          found_la=
          found_so=
          found_a=
          if test $use_additional = yes; then
            if test -n "$shlibext" && test -f "$additional_libdir/lib$name.$shlibext"; then
              found_dir="$additional_libdir"
              found_so="$additional_libdir/lib$name.$shlibext"
              if test -f "$additional_libdir/lib$name.la"; then
                found_la="$additional_libdir/lib$name.la"
              fi
            else
              if test -f "$additional_libdir/lib$name.$libext"; then
                found_dir="$additional_libdir"
                found_a="$additional_libdir/lib$name.$libext"
                if test -f "$additional_libdir/lib$name.la"; then
                  found_la="$additional_libdir/lib$name.la"
                fi
              fi
            fi
          fi
          if test "X$found_dir" = "X"; then
            for x in $LDFLAGS $LTLIB[]NAME; do
              AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
              case "$x" in
                -L*)
                  dir=`echo "X$x" | sed -e 's/^X-L//'`
                  if test -n "$shlibext" && test -f "$dir/lib$name.$shlibext"; then
                    found_dir="$dir"
                    found_so="$dir/lib$name.$shlibext"
                    if test -f "$dir/lib$name.la"; then
                      found_la="$dir/lib$name.la"
                    fi
                  else
                    if test -f "$dir/lib$name.$libext"; then
                      found_dir="$dir"
                      found_a="$dir/lib$name.$libext"
                      if test -f "$dir/lib$name.la"; then
                        found_la="$dir/lib$name.la"
                      fi
                    fi
                  fi
                  ;;
              esac
              if test "X$found_dir" != "X"; then
                break
              fi
            done
          fi
          if test "X$found_dir" != "X"; then
            dnl Found the library.
            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-L$found_dir -l$name"
            if test "X$found_so" != "X"; then
              dnl Linking with a shared library. We attempt to hardcode its
              dnl directory into the executable's runpath, unless it's the
              dnl standard /usr/lib.
              if test "$enable_rpath" = no || test "X$found_dir" = "X/usr/lib"; then
                dnl No hardcoding is needed.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
              else
                dnl Use an explicit option to hardcode DIR into the resulting
                dnl binary.
                dnl Potentially add DIR to ltrpathdirs.
                dnl The ltrpathdirs will be appended to $LTLIBNAME at the end.
                haveit=
                for x in $ltrpathdirs; do
                  if test "X$x" = "X$found_dir"; then
                    haveit=yes
                    break
                  fi
                done
                if test -z "$haveit"; then
                  ltrpathdirs="$ltrpathdirs $found_dir"
                fi
                dnl The hardcoding into $LIBNAME is system dependent.
                if test "$hardcode_direct" = yes; then
                  dnl Using DIR/libNAME.so during linking hardcodes DIR into the
                  dnl resulting binary.
                  LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                else
                  if test -n "$hardcode_libdir_flag_spec" && test "$hardcode_minus_L" = no; then
                    dnl Use an explicit option to hardcode DIR into the resulting
                    dnl binary.
                    LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                    dnl Potentially add DIR to rpathdirs.
                    dnl The rpathdirs will be appended to $LIBNAME at the end.
                    haveit=
                    for x in $rpathdirs; do
                      if test "X$x" = "X$found_dir"; then
                        haveit=yes
                        break
                      fi
                    done
                    if test -z "$haveit"; then
                      rpathdirs="$rpathdirs $found_dir"
                    fi
                  else
                    dnl Rely on "-L$found_dir".
                    dnl But don't add it if it's already contained in the LDFLAGS
                    dnl or the already constructed $LIBNAME
                    haveit=
                    for x in $LDFLAGS $LIB[]NAME; do
                      AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                      if test "X$x" = "X-L$found_dir"; then
                        haveit=yes
                        break
                      fi
                    done
                    if test -z "$haveit"; then
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$found_dir"
                    fi
                    if test "$hardcode_minus_L" != no; then
                      dnl FIXME: Not sure whether we should use
                      dnl "-L$found_dir -l$name" or "-L$found_dir $found_so"
                      dnl here.
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_so"
                    else
                      dnl We cannot use $hardcode_runpath_var and LD_RUN_PATH
                      dnl here, because this doesn't fit in flags passed to the
                      dnl compiler. So give up. No hardcoding. This affects only
                      dnl very old systems.
                      dnl FIXME: Not sure whether we should use
                      dnl "-L$found_dir -l$name" or "-L$found_dir $found_so"
                      dnl here.
                      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-l$name"
                    fi
                  fi
                fi
              fi
            else
              if test "X$found_a" != "X"; then
                dnl Linking with a static library.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$found_a"
              else
                dnl We shouldn't come here, but anyway it's good to have a
                dnl fallback.
                LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$found_dir -l$name"
              fi
            fi
            dnl Assume the include files are nearby.
            additional_includedir=
            case "$found_dir" in
              */lib | */lib/)
                basedir=`echo "X$found_dir" | sed -e 's,^X,,' -e 's,/lib/*$,,'`
                additional_includedir="$basedir/include"
                ;;
            esac
            if test "X$additional_includedir" != "X"; then
              dnl Potentially add $additional_includedir to $INCNAME.
              dnl But don't add it
              dnl   1. if it's the standard /usr/include,
              dnl   2. if it's /usr/local/include and we are using GCC on Linux,
              dnl   3. if it's already present in $CPPFLAGS or the already
              dnl      constructed $INCNAME,
              dnl   4. if it doesn't exist as a directory.
              if test "X$additional_includedir" != "X/usr/include"; then
                haveit=
                if test "X$additional_includedir" = "X/usr/local/include"; then
                  if test -n "$GCC"; then
                    case $host_os in
                      linux*) haveit=yes;;
                    esac
                  fi
                fi
                if test -z "$haveit"; then
                  for x in $CPPFLAGS $INC[]NAME; do
                    AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                    if test "X$x" = "X-I$additional_includedir"; then
                      haveit=yes
                      break
                    fi
                  done
                  if test -z "$haveit"; then
                    if test -d "$additional_includedir"; then
                      dnl Really add $additional_includedir to $INCNAME.
                      INC[]NAME="${INC[]NAME}${INC[]NAME:+ }-I$additional_includedir"
                    fi
                  fi
                fi
              fi
            fi
            dnl Look for dependencies.
            if test -n "$found_la"; then
              dnl Read the .la file. It defines the variables
              dnl dlname, library_names, old_library, dependency_libs, current,
              dnl age, revision, installed, dlopen, dlpreopen, libdir.
              save_libdir="$libdir"
              case "$found_la" in
                */* | *\\*) . "$found_la" ;;
                *) . "./$found_la" ;;
              esac
              libdir="$save_libdir"
              dnl We use only dependency_libs.
              for dep in $dependency_libs; do
                case "$dep" in
                  -L*)
                    additional_libdir=`echo "X$dep" | sed -e 's/^X-L//'`
                    dnl Potentially add $additional_libdir to $LIBNAME and $LTLIBNAME.
                    dnl But don't add it
                    dnl   1. if it's the standard /usr/lib,
                    dnl   2. if it's /usr/local/lib and we are using GCC on Linux,
                    dnl   3. if it's already present in $LDFLAGS or the already
                    dnl      constructed $LIBNAME,
                    dnl   4. if it doesn't exist as a directory.
                    if test "X$additional_libdir" != "X/usr/lib"; then
                      haveit=
                      if test "X$additional_libdir" = "X/usr/local/lib"; then
                        if test -n "$GCC"; then
                          case $host_os in
                            linux*) haveit=yes;;
                          esac
                        fi
                      fi
                      if test -z "$haveit"; then
                        haveit=
                        for x in $LDFLAGS $LIB[]NAME; do
                          AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                          if test "X$x" = "X-L$additional_libdir"; then
                            haveit=yes
                            break
                          fi
                        done
                        if test -z "$haveit"; then
                          if test -d "$additional_libdir"; then
                            dnl Really add $additional_libdir to $LIBNAME.
                            LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-L$additional_libdir"
                          fi
                        fi
                        haveit=
                        for x in $LDFLAGS $LTLIB[]NAME; do
                          AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
                          if test "X$x" = "X-L$additional_libdir"; then
                            haveit=yes
                            break
                          fi
                        done
                        if test -z "$haveit"; then
                          if test -d "$additional_libdir"; then
                            dnl Really add $additional_libdir to $LTLIBNAME.
                            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-L$additional_libdir"
                          fi
                        fi
                      fi
                    fi
                    ;;
                  -R*)
                    dir=`echo "X$dep" | sed -e 's/^X-R//'`
                    if test "$enable_rpath" != no; then
                      dnl Potentially add DIR to rpathdirs.
                      dnl The rpathdirs will be appended to $LIBNAME at the end.
                      haveit=
                      for x in $rpathdirs; do
                        if test "X$x" = "X$dir"; then
                          haveit=yes
                          break
                        fi
                      done
                      if test -z "$haveit"; then
                        rpathdirs="$rpathdirs $dir"
                      fi
                      dnl Potentially add DIR to ltrpathdirs.
                      dnl The ltrpathdirs will be appended to $LTLIBNAME at the end.
                      haveit=
                      for x in $ltrpathdirs; do
                        if test "X$x" = "X$dir"; then
                          haveit=yes
                          break
                        fi
                      done
                      if test -z "$haveit"; then
                        ltrpathdirs="$ltrpathdirs $dir"
                      fi
                    fi
                    ;;
                  -l*)
                    dnl Handle this in the next round.
                    names_next_round="$names_next_round "`echo "X$dep" | sed -e 's/^X-l//'`
                    ;;
                  *.la)
                    dnl Handle this in the next round. Throw away the .la's
                    dnl directory; it is already contained in a preceding -L
                    dnl option.
                    names_next_round="$names_next_round "`echo "X$dep" | sed -e 's,^X.*/,,' -e 's,^lib,,' -e 's,\.la$,,'`
                    ;;
                  *)
                    dnl Most likely an immediate library name.
                    LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$dep"
                    LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }$dep"
                    ;;
                esac
              done
            fi
          else
            dnl Didn't find the library; assume it is in the system directories
            dnl known to the linker and runtime loader. (All the system
            dnl directories known to the linker should also be known to the
            dnl runtime loader, otherwise the system is severely misconfigured.)
            LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }-l$name"
            LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-l$name"
          fi
        fi
      fi
    done
  done
  if test "X$rpathdirs" != "X"; then
    if test -n "$hardcode_libdir_separator"; then
      dnl Weird platform: only the last -rpath option counts, the user must
      dnl pass all path elements in one option. We can arrange that for a
      dnl single library, but not when more than one $LIBNAMEs are used.
      alldirs=
      for found_dir in $rpathdirs; do
        alldirs="${alldirs}${alldirs:+$hardcode_libdir_separator}$found_dir"
      done
      dnl Note: hardcode_libdir_flag_spec uses $libdir and $wl.
      acl_save_libdir="$libdir"
      libdir="$alldirs"
      eval flag=\"$hardcode_libdir_flag_spec\"
      libdir="$acl_save_libdir"
      LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$flag"
    else
      dnl The -rpath options are cumulative.
      for found_dir in $rpathdirs; do
        acl_save_libdir="$libdir"
        libdir="$found_dir"
        eval flag=\"$hardcode_libdir_flag_spec\"
        libdir="$acl_save_libdir"
        LIB[]NAME="${LIB[]NAME}${LIB[]NAME:+ }$flag"
      done
    fi
  fi
  if test "X$ltrpathdirs" != "X"; then
    dnl When using libtool, the option that works for both libraries and
    dnl executables is -R. The -R options are cumulative.
    for found_dir in $ltrpathdirs; do
      LTLIB[]NAME="${LTLIB[]NAME}${LTLIB[]NAME:+ }-R$found_dir"
    done
  fi
])

dnl AC_LIB_APPENDTOVAR(VAR, CONTENTS) appends the elements of CONTENTS to VAR,
dnl unless already present in VAR.
dnl Works only for CPPFLAGS, not for LIB* variables because that sometimes
dnl contains two or three consecutive elements that belong together.
AC_DEFUN([AC_LIB_APPENDTOVAR],
[
  for element in [$2]; do
    haveit=
    for x in $[$1]; do
      AC_LIB_WITH_FINAL_PREFIX([eval x=\"$x\"])
      if test "X$x" = "X$element"; then
        haveit=yes
        break
      fi
    done
    if test -z "$haveit"; then
      [$1]="${[$1]}${[$1]:+ }$element"
    fi
  done
])

# lib-ld.m4 serial 3 (gettext-0.13)
dnl Copyright (C) 1996-2003 Free Software Foundation, Inc.
dnl This file is free software, distributed under the terms of the GNU
dnl General Public License.  As a special exception to the GNU General
dnl Public License, this file may be distributed as part of a program
dnl that contains a configuration script generated by Autoconf, under
dnl the same distribution terms as the rest of that program.

dnl Subroutines of libtool.m4,
dnl with replacements s/AC_/AC_LIB/ and s/lt_cv/acl_cv/ to avoid collision
dnl with libtool.m4.

dnl From libtool-1.4. Sets the variable with_gnu_ld to yes or no.
AC_DEFUN([AC_LIB_PROG_LD_GNU],
[AC_CACHE_CHECK([if the linker ($LD) is GNU ld], acl_cv_prog_gnu_ld,
[# I'd rather use --version here, but apparently some GNU ld's only accept -v.
case `$LD -v 2>&1 </dev/null` in
*GNU* | *'with BFD'*)
  acl_cv_prog_gnu_ld=yes ;;
*)
  acl_cv_prog_gnu_ld=no ;;
esac])
with_gnu_ld=$acl_cv_prog_gnu_ld
])

dnl From libtool-1.4. Sets the variable LD.
AC_DEFUN([AC_LIB_PROG_LD],
[AC_ARG_WITH(gnu-ld,
[  --with-gnu-ld           assume the C compiler uses GNU ld [default=no]],
test "$withval" = no || with_gnu_ld=yes, with_gnu_ld=no)
AC_REQUIRE([AC_PROG_CC])dnl
AC_REQUIRE([AC_CANONICAL_HOST])dnl
# Prepare PATH_SEPARATOR.
# The user is always right.
if test "${PATH_SEPARATOR+set}" != set; then
  echo "#! /bin/sh" >conf$$.sh
  echo  "exit 0"   >>conf$$.sh
  chmod +x conf$$.sh
  if (PATH="/nonexistent;."; conf$$.sh) >/dev/null 2>&1; then
    PATH_SEPARATOR=';'
  else
    PATH_SEPARATOR=:
  fi
  rm -f conf$$.sh
fi
ac_prog=ld
if test "$GCC" = yes; then
  # Check if gcc -print-prog-name=ld gives a path.
  AC_MSG_CHECKING([for ld used by GCC])
  case $host in
  *-*-mingw*)
    # gcc leaves a trailing carriage return which upsets mingw
    ac_prog=`($CC -print-prog-name=ld) 2>&5 | tr -d '\015'` ;;
  *)
    ac_prog=`($CC -print-prog-name=ld) 2>&5` ;;
  esac
  case $ac_prog in
    # Accept absolute paths.
    [[\\/]* | [A-Za-z]:[\\/]*)]
      [re_direlt='/[^/][^/]*/\.\./']
      # Canonicalize the path of ld
      ac_prog=`echo $ac_prog| sed 's%\\\\%/%g'`
      while echo $ac_prog | grep "$re_direlt" > /dev/null 2>&1; do
	ac_prog=`echo $ac_prog| sed "s%$re_direlt%/%"`
      done
      test -z "$LD" && LD="$ac_prog"
      ;;
  "")
    # If it fails, then pretend we aren't using GCC.
    ac_prog=ld
    ;;
  *)
    # If it is relative, then search for the first ld in PATH.
    with_gnu_ld=unknown
    ;;
  esac
elif test "$with_gnu_ld" = yes; then
  AC_MSG_CHECKING([for GNU ld])
else
  AC_MSG_CHECKING([for non-GNU ld])
fi
AC_CACHE_VAL(acl_cv_path_LD,
[if test -z "$LD"; then
  IFS="${IFS= 	}"; ac_save_ifs="$IFS"; IFS="${IFS}${PATH_SEPARATOR-:}"
  for ac_dir in $PATH; do
    test -z "$ac_dir" && ac_dir=.
    if test -f "$ac_dir/$ac_prog" || test -f "$ac_dir/$ac_prog$ac_exeext"; then
      acl_cv_path_LD="$ac_dir/$ac_prog"
      # Check to see if the program is GNU ld.  I'd rather use --version,
      # but apparently some GNU ld's only accept -v.
      # Break only if it was the GNU/non-GNU ld that we prefer.
      case `"$acl_cv_path_LD" -v 2>&1 < /dev/null` in
      *GNU* | *'with BFD'*)
	test "$with_gnu_ld" != no && break ;;
      *)
	test "$with_gnu_ld" != yes && break ;;
      esac
    fi
  done
  IFS="$ac_save_ifs"
else
  acl_cv_path_LD="$LD" # Let the user override the test with a path.
fi])
LD="$acl_cv_path_LD"
if test -n "$LD"; then
  AC_MSG_RESULT($LD)
else
  AC_MSG_RESULT(no)
fi
test -z "$LD" && AC_MSG_ERROR([no acceptable ld found in \$PATH])
AC_LIB_PROG_LD_GNU
])

