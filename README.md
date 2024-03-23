<!--
SPDX-FileCopyrightText: 2007 OpenedHand
SPDX-FileCopyrightText: 2009 Intel Corp
SPDX-FileCopyrightText: 2012 Emmanuele Bassi

SPDX-License-Identifier: LGPL-2.1-or-later
-->

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
developers. GLib and GObject are extensively used by the GTK toolkit and by the
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
 * [GLib](https://gitlab.gnome.org/GNOME/glib)
 * [GObject-Introspection](https://gitlab.gnome.org/GNOME/gobject-introspection)

Build and installation
--------------------------------------------------------------------------------

To build JSON-GLib, run:

```sh
$ meson setup _build .
$ meson compile -C _build
$ meson test -C _build
$ meson install -C _build
```

See the [Meson documentation](http://mesonbuild.com) for more information.

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

[json]: https://www.json.org "JSON"
[glib]: https://gitlab.gnome.org/GNOME/glib "GLib"
[json-glib]: https://gnome.pages.gitlab.gnome.org/json-glib/
[gnome]: https://www.gnome.org "GNOME"
