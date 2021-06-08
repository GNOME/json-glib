Title: Serializing GObject Types

# Serializing GObject Types

JSON-GLib provides API for serializing and deserializing `GObject` instances
to and from JSON data streams.

Simple `GObject` classes can be (de)serialized into JSON objects, if the
properties have compatible types with the native JSON types (integers,
booleans, strings, string vectors). If the class to be (de)serialized has
complex data types for properties (like boxed types or other objects) then
the class should implement the provided [iface@Json.Serializable] interface
and its virtual functions.
