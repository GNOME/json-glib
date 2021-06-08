Title: Serializing GVariants

# Serializing GVariants

`GVariant` is a serialization format mainly used as the wire data for D-Bus,
as well as storing values in GSettings. JSON-GLib provides utility functions
to serialize and deserialize `GVariant` data to and from JSON.

Use [`func@Json.gvariant_serialize`] and [`func@Json.gvariant_serialize_data`] to
convert from any `GVariant` value to a [struct@Json.Node] tree or its string
representation.

Use [`func@Json.gvariant_deserialize`] and [`func@Json.gvariant_deserialize_data`]
to obtain the `GVariant` value from a `JsonNode` tree or directly from a JSON
string.

Since many `GVariant` data types cannot be directly represented as
JSON, a `GVariant` type string (signature) should be provided to these
methods in order to obtain a correct, type-contrained result.
If no signature is provided, conversion can still be done, but the
resulting `GVariant` value will be "guessed" from the JSON data types
using the following rules:

Strings:
  A JSON string maps to a GVariant string `(s)`.

Integers:
  A JSON integer maps to a GVariant 64-bit integer `(x)`.

Booleans:
  A JSON boolean maps to a GVariant boolean `(b)`.

Numbers:
  A JSON number maps to a GVariant double `(d)`.

Arrays:
  A JSON array maps to a GVariant array of variants `(av)`.

Objects:
  A JSON object maps to a GVariant dictionary of string to variants `(a{sv})`.

`NULL` values:
  A JSON `null` value maps to a GVariant "maybe" variant `(mv)`.
