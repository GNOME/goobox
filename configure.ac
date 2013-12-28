AC_PREREQ(2.61)
AC_INIT([goobox], [3.3.0], [http://bugzilla.gnome.org/enter_bug.cgi?product=goobox])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz tar-ustar])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

GNOME_COMMON_INIT
AC_PROG_CXX
AM_PROG_LIBTOOL
YELP_HELP_INIT
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES
GLIB_GSETTINGS

GLIB_REQUIRED=2.36
GTK_REQUIRED=3.8.0
GSTREAMER_REQUIRED=1.0.0
LIBNOTIFY_REQUIRED=0.4.3
LIBMUSICBRAINZ5_REQUIRED=5.0.0
LIBCOVERART_REQUIRED=1.0.0

dnl ===========================================================================

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

PKG_CHECK_MODULES(GOO, [
	glib-2.0 >= $GLIB_REQUIRED
	gthread-2.0
	gtk+-3.0 >= $GTK_REQUIRED
	gstreamer-1.0 >= $GSTREAMER_REQUIRED
	libbrasero-media3
	libmusicbrainz5 >= $LIBMUSICBRAINZ5_REQUIRED
	libdiscid
])
AC_SUBST(GOO_CFLAGS)
AC_SUBST(GOO_LIBS)

dnl ===========================================================================

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums, glib-mkenums)
AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources, glib-compile-resources)

dnl ===========================================================================

AC_ARG_ENABLE(debug,
	      AS_HELP_STRING([--enable-debug], [enable compilation of debugging messages]),
              [case "${enableval}" in
              yes) ENABLE_DEBUG=yes ;;
	      no) ENABLE_DEBUG=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
	      esac],
	      [ENABLE_DEBUG=no])
if test x$ENABLE_DEBUG = xyes; then
	AC_DEFINE(DEBUG, 1, [enable compilation of debugging messages])
	CFLAGS="$CFLAGS -g -O0 -DDEBUG"
fi

dnl ===========================================================================

if test x$ENABLE_DEBUG = xyes; then
	WARN_CFLAGS="-Wall -Wcast-align -Wtype-limits -Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-prototypes -Wnested-externs -Wpointer-arith -Wno-sign-compare"
else
	WARN_CFLAGS="-Wall"
fi

for option in $WARN_CFLAGS; do
	SAVE_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $option"
	AC_MSG_CHECKING([whether gcc understands $option])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[]], [[]])],[has_option=yes],[has_option=no])
	if test x$has_option = xyes; then
		WARNINGS="$WARNINGS $option"
	fi
	AC_MSG_RESULT($has_option)
	CFLAGS="$SAVE_CFLAGS"
	unset has_option
	unset SAVE_CFLAGS
done
unset option
CFLAGS="$CFLAGS $WARNINGS"

dnl ===========================================================================

AC_ARG_ENABLE(run_in_place,
	      AS_HELP_STRING([--enable-run-in-place],[load ui data and extensions from the source tree]),
	      [case "${enableval}" in
	      yes) enable_run_in_place=yes ;;
	      no) enable_run_in_place=no ;;
	      *) AC_MSG_ERROR([bad value ${enableval} for --enable-run-in-place]) ;;
	      esac],
	      [enable_run_in_place=no])
AM_CONDITIONAL(RUN_IN_PLACE, test "x$enable_run_in_place" != xno)

if test x$enable_run_in_place = xyes; then
	AC_DEFINE(RUN_IN_PLACE, 1, [load ui data and extensions from the source tree])
fi

dnl ===========================================================================

SAVE_CFLAGS="$CFLAGS"
CFLAGS=""
AC_CHECK_LIB(m, floor,, AC_MSG_ERROR([math library not found]))
M_LIBS="-lm"
CFLAGS="$SAVE_CFLAGS"
unset SAVE_CFLAGS
AC_SUBST(M_LIBS)

dnl ===========================================================================
dnl
dnl libcoverart
dnl
AC_ARG_ENABLE(libcoverart,[AC_HELP_STRING([--enable-libcoverart],[enable use of libcoverart to fetch CD covers [default=yes]])],, [enable_libcoverart="yes"])

LIBCOVERART_LIBS=""
LIBCOVERART_CFLAGS=""
if test x"$enable_libcoverart" = xyes; then
   AC_MSG_CHECKING(for libcoverart support)

   if pkg-config --atleast-version=$LIBCOVERART_REQUIRED libcoverart; then
      LIBCOVERART_LIBS=`pkg-config --libs libcoverart`
      LIBCOVERART_CFLAGS=`pkg-config --cflags libcoverart`
      AC_DEFINE(HAVE_LIBCOVERART, 1, [Have libcoverart])
   else
      enable_libcoverart=no
   fi
fi
AC_MSG_RESULT($enable_libcoverart)
AC_SUBST(LIBCOVERART_LIBS)
AC_SUBST(LIBCOVERART_CFLAGS)

dnl ===========================================================================
dnl
dnl libnotify
dnl
AC_ARG_ENABLE(notification,[AC_HELP_STRING([--enable-notification],[enable current track notification if libnotify is available [default=yes]])],, [enable_notification="yes"])
AM_CONDITIONAL(ENABLE_NOTIFICATION, test "x$enable_notification" = xyes)

LIBNOTIFY_LIBS=""
LIBNOTIFY_CFLAGS=""
if test x"$enable_notification" = xyes; then
   AC_MSG_CHECKING(for notification support)

   if pkg-config --atleast-version=$LIBNOTIFY_REQUIRED libnotify; then
      LIBNOTIFY_LIBS=`pkg-config --libs libnotify`
      LIBNOTIFY_CFLAGS=`pkg-config --cflags libnotify`
      AC_DEFINE(ENABLE_NOTIFICATION, 1, [Have libnotify])
   else
      enable_notification=no
   fi
fi
AC_MSG_RESULT($enable_notification)
AC_SUBST(LIBNOTIFY_LIBS)
AC_SUBST(LIBNOTIFY_CFLAGS)

dnl ===========================================================================

AC_ARG_ENABLE(media-keys,[AC_HELP_STRING([--enable-media-keys],[enable use of the keyboard media keys [default=yes]])],, [enable_media_keys="yes"])
AM_CONDITIONAL(ENABLE_MEDIA_KEYS, test "x$enable_media_keys" = xyes)

DBUS_LIBS=""
DBUS_CFLAGS=""
if test x"$enable_media_keys" = xyes; then
   AC_DEFINE(ENABLE_MEDIA_KEYS, 1, [enable media keys support])
fi
AC_MSG_RESULT($enable_media_keys)

dnl ===========================================================================

GDK_TARGET="$($PKG_CONFIG --variable targets gdk-3.0)"

AC_MSG_CHECKING([which smclient backend to use])
AC_ARG_WITH([smclient],
  [AS_HELP_STRING([--with-smclient=no|auto|xsmp],
                  [Setting smclient backend (default:no)])],,
                  [with_smclient=no])

if test "$with_smclient" == "auto"; then
  case "$GDK_TARGET" in
    *x11*) with_smclient=xsmp ;;
    *) with_smclient=no ;;
  esac
fi

AC_MSG_RESULT([$with_smclient])

if test "$with_smclient" != "no"; then
  AC_DEFINE([WITH_SMCLIENT],[1],[Define if smclient is enabled])

  case "$with_smclient" in
    xsmp) SMCLIENT_PKGS="sm >= 1.0.0 ice" ;;
    *) SMCLIENT_PKGS="" ;;
  esac

  PKG_CHECK_MODULES([SMCLIENT],[$SMCLIENT_PKGS])
  AC_SUBST([SMCLIENT_CFLAGS])
  AC_SUBST([SMCLIENT_LIBS])
  AC_DEFINE(USE_SMCLIENT, 1, [Use a session menager])
fi

AM_CONDITIONAL([WITH_SMCLIENT],[test "$with_smclient" != "no"])
AM_CONDITIONAL([WITH_SMCLIENT_XSMP],[test "$with_smclient" = "xsmp"])
AM_CONDITIONAL([WITH_SMCLIENT_WIN32],[test "$with_smclient" = "win32"])

dnl ===========================================================================

SYSTEM_LIBS=""
host=`uname -s`
case "$host" in
*FreeBSD*)
	DEFAULT_DEVICE="/dev/acd0c"
	AC_DEFINE(HAVE_BSD, 1, [Define to 1 if this is a BSD system])
	SYSTEM_LIBS="-lcam"
	;;
*NetBSD*)
	DEFAULT_DEVICE="/dev/rcd0d"
	AC_DEFINE(HAVE_BSD, 1, [Define to 1 if this is a BSD system])
	;;
*Linux*)
	DEFAULT_DEVICE="/dev/cdrom"
	AC_DEFINE(HAVE_LINUX, 1, [Define to 1 if this is a Linux system])
	;;
*SunOS*)
	DEFAULT_DEVICE="/vol/dev/aliases/cdrom0"
	AC_DEFINE(HAVE_SOLARIS, 1, [Define to 1 if this is a Solaris system])
	;;
esac

AC_DEFINE_UNQUOTED(DEFAULT_DEVICE, "$DEFAULT_DEVICE", [Default CD device])
AC_SUBST(SYSTEM_LIBS)

dnl ===========================================================================

IT_PROG_INTLTOOL([0.35.0])
GETTEXT_PACKAGE=goobox
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GetText Package])
AM_GLIB_GNU_GETTEXT

if test "x${prefix}" = "xNONE"; then
	AC_DEFINE_UNQUOTED(LOCALEDIR, "${ac_default_prefix}/${DATADIRNAME}/locale", [Locale directory])
else
	AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Locale directory])
fi

dnl ===========================================================================

AC_ARG_ENABLE(deprecated,
	      AS_HELP_STRING([--disable-deprecated], [do not use deprecated API]),
	      disable_deprecated=yes,
	      disable_deprecated=no)
if test x$disable_deprecated = xyes; then
	CFLAGS="$CFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
	CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
	CFLAGS="$CFLAGS -DGSEAL_ENABLE -DDISABLE_DEPRECATED -DGSTREAMER_DISABLE_DEPRECATED"
fi

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
copy-n-paste/Makefile
data/Makefile
data/goobox.desktop.in
data/appdata/Makefile
data/icons/Makefile
data/icons/256x256/Makefile
data/icons/256x256/apps/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
help/Makefile
src/Makefile
src/icons/Makefile
src/ui/Makefile
po/Makefile.in
])

AC_OUTPUT

echo "

Configuration:

	Source code location : ${srcdir}
	Compiler             : ${CC}
	Prefix               : ${prefix}
	CFLAGS		     : ${CFLAGS}
	Run in place         : ${enable_run_in_place}
	Enable notification  : ${enable_notification}
	Enable media keys    : ${enable_media_keys}
	Use libcoverart      : ${enable_libcoverart}
	SM client support    : ${with_smclient}
"
