JSON-GLib
===============================================================================

JSON-GLib implements a full suite of JSON-related tools using GLib and GObject.

Use JSON-GLib it is possible to parse and generate valid JSON data
structures using a DOM-like API. JSON-GLib also integrates with GObject to
provide the ability to serialize and deserialize GObject instances to and from
JSON data types.

JSON is the JavaScript Object Notation; it can be used to represent objects and
object hierarchies while retaining human-readability.

GLib is a C library providing common and efficient data types for the C
developers.

GObject is a library providing a run-time Object Oriented type system for C
developers. GLib and GObject are extensively used by the GTK+ toolkit and by the
[GNOME][gnome] project.

For more information, see:

 * [JSON][json]
 * [GLib and GObject][glib]
 * [JSON-GLib][json-glib]

Requirements
--------------------------------------------------------------------------------
In order to build JSON-GLib you will need:

 * python3
 * [ninja](http://ninja-build.org)
 * [meson](http://mesonbuild.com)
 * pkg-config
 * gtk-doc ≥ 1.13 (optional)
 * GLib, GIO ≥ 2.38
 * GObject-Introspection ≥ 1.38 (optional)

Build and installation
--------------------------------------------------------------------------------
To build JSON-GLib just run:

```sh
  $ meson _build .
  $ ninja -C _build
  $ mesontest -C _build
  $ sudo ninja -C _build install
```

See the [Meson documentation](http://mesonbuild.com) for more information.

Contributing
--------------------------------------------------------------------------------
If you find a bug in JSON-GLib, please file an issue on the
[Issues page][gitlab-issues].

Required information:

 * the version of JSON-GLib
  * if it is a development version, the branch of the git repository
 * the JSON data that produced the bug (if any)
 * a small, self-contained test case, if none of the test units exhibit the
   buggy behaviour
 * in case of a segmentation fault, a full stack trace with debugging
   symbols obtained through gdb is greatly appreaciated

JSON-GLib is developed mainly inside a GIT repository available at:

    https://gitlab.gnome.org/GNOME/json-glib/

You can clone the GIT repository with:

    git clone https://gitlab.gnome.org/GNOME/json-glib.git

If you want to contribute functionality or bug fixes to JSON-GLib you should
fork the json-glib repository, work on a separate branch, and then open a
merge request on Gitlab:

    https://gitlab.gnome.org/GNOME/json-glib/merge_requests/new

Please, try to conform to the coding style used by JSON-GLib, which is the same
used by projects like GLib, [GTK+][gtk-coding-style], and Clutter. Coding style
conformance is a requirement for upstream acceptance.

Make sure you always run the test suite when you are fixing bugs. New features
should come with a test unit. Patches that regress the test suite will be
rejected.

Release notes
--------------------------------------------------------------------------------
 * Prior to JSON-GLib 0.10, a JsonSerializable implementation could
   automatically fall back to the default serialization code by simply
   returning NULL from an overridden JsonSerializable::serialize-property
   virtual function. Since JSON-GLib 0.10 this is not possible any more. A
   JsonSerializable is always expected to serialize and deserialize all
   properties. JSON-GLib provides public API for the default implementation
   in case the serialization code wants to fall back to that.

Copyright and licensing
--------------------------------------------------------------------------------
JSON-GLib has been written by Emmanuele Bassi

JSON-GLib is released under the terms of the GNU Lesser General Public License,
either version 2.1 or (at your option) any later version.

See the file COPYING for details.

Copyright 2007, 2008  OpenedHand Ltd
Copyright 2009, 2010, 2011, 2012  Intel Corp.
Copyright 2013  Emmanuele Bassi

[json]: http://www.json.org "JSON"
[glib]: http://www.gtk.org "GTK+"
[json-glib]: https://wiki.gnome.org/Projects/JsonGlib "JSON-GLib wiki"
[gnome]: https://www.gnome.org "GNOME"
[gitlab-issues]: https://gitlab.gnome.org/GNOME/json-glib/issues
[gtk-coding-style]: https://git.gnome.org/browse/gtk+/tree/docs/CODING-STYLE
