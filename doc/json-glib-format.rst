.. _json-glib-format:

================
json-glib-format
================

--------------------
JSON Formatting Tool
--------------------

:Author: Emmanuele Bassi
:Manual section: 1
:Manual group: text processing

SYNOPSIS
--------
|   **json-glib-format** [OPTIONS...] <FILE> [<FILE>...]

DESCRIPTION
-----------

``json-glib-format`` offers a simple command line interface to format JSON data.
It reads a list or URIs, applies the spacified formatting rules on the JSON
data, and outputs the formatted JSON to the standard output or to a file.

The resources to operate on are specified by the ``FILE`` argument.

OPTIONS
-------

``-h, --help``

  Print a help message and exit.

``--output=FILE``

  Redirects the output to ``FILE`` instead of using the standard output.

``-p, --prettify``

  Prettifies the output, by adding spaces and indentation. This argument is
  useful to improve the readability of JSON data, at the expense of its size.

``--indent-spaces=SPACES``

  Changes the number of spaces using to indent the JSON data from the default
  of 2. This argument is only considered if ``--prettify`` is used.

SEE ALSO
--------

* `JSON-GLib <https://gitlab.gnome.org/GNOME/json-glib>`__
* ``json-glib-validate(1)``
