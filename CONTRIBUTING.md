# Contributing

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
used by projects like GLib, [GTK][gtk-coding-style], and Clutter. Coding style
conformance is a requirement for upstream acceptance.

Make sure you always run the test suite when you are fixing bugs. New features
should come with a test unit. Functionality that regress the test suite will be
rejected.
