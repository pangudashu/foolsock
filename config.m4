dnl $Id$
dnl config.m4 for extension foolsock

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(foolsock, for foolsock support,
Make sure that the comment is aligned:
[  --with-foolsock             Include foolsock support])

dnl Otherwise use enable:

dnl PHP_ARG_ENABLE(foolsock, whether to enable foolsock support,
dnl Make sure that the comment is aligned:
dnl [  --enable-foolsock           Enable foolsock support])

if test "$PHP_FOOLSOCK" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-foolsock -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/foolsock.h"  # you most likely want to change this
  dnl if test -r $PHP_FOOLSOCK/$SEARCH_FOR; then # path given as parameter
  dnl   FOOLSOCK_DIR=$PHP_FOOLSOCK
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for foolsock files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       FOOLSOCK_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$FOOLSOCK_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the foolsock distribution])
  dnl fi

  dnl # --with-foolsock -> add include path
  dnl PHP_ADD_INCLUDE($FOOLSOCK_DIR/include)

  dnl # --with-foolsock -> check for lib and symbol presence
  dnl LIBNAME=foolsock # you may want to change this
  dnl LIBSYMBOL=foolsock # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $FOOLSOCK_DIR/lib, FOOLSOCK_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_FOOLSOCKLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong foolsock lib version or lib not found])
  dnl ],[
  dnl   -L$FOOLSOCK_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(FOOLSOCK_SHARED_LIBADD)

  PHP_NEW_EXTENSION(foolsock, foolsock.c, $ext_shared)
fi
