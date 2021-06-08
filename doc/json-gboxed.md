Title: Serializing GBoxed Types

# Serializing GBoxed Types

GLib's `GBoxed` type is a generic wrapper for arbitrary C structures.

JSON-GLib allows serialization and deserialization of a boxed type
by registering functions mapping a [enum@Json.NodeType] to a specific
`GType`.

When registering a boxed type you should also register the
corresponding transformation functions, e.g.:

```c
GType
my_struct_get_type (void)
{
  static GType boxed_type = 0;

  if (boxed_type == 0)
    {
      boxed_type =
        g_boxed_type_register_static (g_intern_static_string ("MyStruct"),
                                      (GBoxedCopyFunc) my_struct_copy,
                                      (GBoxedFreeFunc) my_struct_free);

      json_boxed_register_serialize_func (boxed_type, JSON_NODE_OBJECT,
                                          my_struct_serialize);
      json_boxed_register_deserialize_func (boxed_type, JSON_NODE_OBJECT,
                                            my_struct_deserialize);
    }

  return boxed_type;
}
```

The serialization function will be invoked by [func@Json.boxed_serialize]:
it will be passed a pointer to the C structure and it must return a
[struct@Json.Node]. The deserialization function will be invoked by
[func@Json.boxed_deserialize]: it will be passed a `JsonNode` for the
declared type and it must return a newly allocated C structure.

It is possible to check whether a boxed type can be deserialized
from a specific `JsonNodeType`, and whether a boxed can be serialized
and to which specific `JsonNodeType`.
