AC_INIT([goobox], [2.2.0], [http://bugzilla.gnome.org/enter_bug.cgi?product=goobox])

GNOME_COMMON_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([configure.ac])

AM_INIT_AUTOMAKE([1.9 foreign])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES])

AC_PROG_CXX
AM_PROG_LIBTOOL
GNOME_DOC_INIT
GNOME_COMPILE_WARNINGS([maximum])
GNOME_MAINTAINER_MODE_DEFINES

GLIB_REQUIRED=2.22
GTK_REQUIRED=2.18
GSTREAMER_REQUIRED=0.10.12
LIBNOTIFY_REQUIRED=0.4.3
LIBMUSICBRAINZ_REQUIRED=2.1.0

dnl ===========================================================================

PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= $GTK_REQUIRED])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

PKG_CHECK_MODULES(GOO, [
	glib-2.0 >= $GLIB_REQUIRED
	gthread-2.0				
	gtk+-2.0 >= $GTK_REQUIRED
	gstreamer-0.10 >= $GSTREAMER_REQUIRED
	libbrasero-media	
	libmusicbrainz >= $LIBMUSICBRAINZ_REQUIRED 
	gconf-2.0
	unique-1.0
])
AC_SUBST(GOO_CFLAGS)
AC_SUBST(GOO_LIBS)

dnl ===========================================================================

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

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
   AC_MSG_CHECKING(for media keys support)

   if pkg-config dbus-glib-1; then
      DBUS_LIBS=`pkg-config --libs dbus-glib-1`
      DBUS_CFLAGS=`pkg-config --cflags dbus-glib-1`
      AC_DEFINE(ENABLE_MEDIA_KEYS, 1, [enable media keys support])
   else
      enable_media_keys=no
   fi
fi
AC_MSG_RESULT($enable_media_keys)
AC_SUBST(DBUS_LIBS)
AC_SUBST(DBUS_CFLAGS)

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

AC_ARG_ENABLE(deprecations,
	      [AS_HELP_STRING([--enable-deprecations],[warn about deprecated usages])])
if test "x$enable_deprecations" = "xyes"; then
	DISABLE_DEPRECATED="-DGSEAL_ENABLE -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED"
fi
AC_SUBST(DISABLE_DEPRECATED)

dnl ===========================================================================

AC_ARG_ENABLE(schemas-install,[AC_HELP_STRING([--disable-schemas-install],
					      [Disable installation of the gconf schemas])])
AM_CONDITIONAL(SCHEMAS_INSTALL, test x$enable_schemas_install != xno)

AC_PATH_PROG(GCONFTOOL, gconftool-2, no)
if test x"$GCONFTOOL" = xno; then
	AC_MSG_ERROR([gconftool-2 executable not found in your path - should be installed with GConf])
fi
AM_GCONF_SOURCE_2

dnl ===========================================================================

AC_CONFIG_FILES([
Makefile
copy-n-paste/Makefile
data/Makefile
data/goobox.desktop.in
data/ui/Makefile
data/icons/Makefile
data/icons/48x48/Makefile
data/icons/48x48/apps/Makefile
help/Makefile
src/Makefile
src/icons/Makefile
po/Makefile.in
])

AC_OUTPUT

echo "

Configuration:

	Source code location:   ${srcdir}
	Compiler:               ${CC}
	Run in place:           ${enable_run_in_place}
	Enable notification:    ${enable_notification}
	Enable media keys:      ${enable_media_keys}
"
