# - try to find the GLIB libraries
# Once done this will define
#
#  GLIB_FOUND - system has GLib
#  GLIB2_CFLAGS - the GLib CFlags
#  GLIB2_LIBRARIES - Link these to use GLib
#
# Copyright 2008-2010 Pino Toscano, <pino@kde.org>
# Copyright 2013 Michael Weiser, <michael@weiser.dinsnail.net>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB2 "glib-2.0" "gobject-2.0" "gio-2.0")

find_package_handle_standard_args(GLib DEFAULT_MSG GLIB2_LIBRARIES GLIB2_CFLAGS)
