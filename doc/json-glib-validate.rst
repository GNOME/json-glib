.. _json-glib-validate:

==================
json-glib-validate
==================

--------------------
JSON Validation Tool
--------------------

:Author: Emmanuele Bassi
:Manual section: 1
:Manual group: text processing

SYNOPSIS
--------
|   **json-glib-validate** [OPTIONS...] <FILE>

DESCRIPTION
-----------

``json-glib-validate`` offers a simple command line interface to validate JSON
data. It lets you list URIs that point to JSON data and checks that the data
conforms to the JSON syntax.

The resources to operate on are specified by the ``FILE`` argument.

If the JSON data is valid, ``json-glib-validate`` will terminate with an exit
code of 0; if the data is invalid, an error will be printed on ``stderr`` and
``json-glib-validate`` will terminate with a nonzero exit code.

OPTIONS
-------

``-h, --help``

  Print a help message and exit.


EXIT STATUS
-----------

- **0** Validation successful.

- **1** Validation error.

SEE ALSO
--------

* `JSON-GLib <https://gitlab.gnome.org/GNOME/json-glib>`__
* ``json-glib-format(1)``
