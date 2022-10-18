#include "json-test-utils.h"
#include <stdlib.h>

static const gchar *test_empty_string = "";
static const gchar *test_empty_array_string = "[ ]";
static const gchar *test_empty_object_string = "{ }";

static void
verify_int_value (JsonNode *node)
{
  g_assert_cmpint (42, ==, json_node_get_int (node));
}

static void
verify_negative_int_value (JsonNode *node)
{
  g_assert_cmpint (-1, ==, json_node_get_int (node));
}

static void
verify_boolean_value (JsonNode *node)
{
  g_assert_cmpint (TRUE, ==, json_node_get_boolean (node));
}

static void
verify_string_value (JsonNode *node)
{
  g_assert_cmpstr ("string", ==, json_node_get_string (node));
}

static void
verify_double_value (JsonNode *node)
{
  json_assert_fuzzy_equals (10.2e3, json_node_get_double (node), 0.1);
}

static void
verify_negative_double_value (JsonNode *node)
{
  json_assert_fuzzy_equals (-3.14, json_node_get_double (node), 0.01);
}

static const struct {
  const gchar *str;
  JsonNodeType type;
  GType gtype;
  void (* verify_value) (JsonNode *node);
} test_base_values[] = {
  { "null",       JSON_NODE_NULL,  G_TYPE_INVALID, NULL, },
  { "42",         JSON_NODE_VALUE, G_TYPE_INT64,   verify_int_value },
  { "true",       JSON_NODE_VALUE, G_TYPE_BOOLEAN, verify_boolean_value },
  { "\"string\"", JSON_NODE_VALUE, G_TYPE_STRING,  verify_string_value },
  { "10.2e3",     JSON_NODE_VALUE, G_TYPE_DOUBLE,  verify_double_value },
  { "-1",         JSON_NODE_VALUE, G_TYPE_INT64,   verify_negative_int_value },
  { "-3.14",      JSON_NODE_VALUE, G_TYPE_DOUBLE,  verify_negative_double_value },
};

static const struct {
  const gchar *str;
  gint len;
  gint element;
  JsonNodeType type;
  GType gtype;
} test_simple_arrays[] = {
  { "[ true ]",                 1, 0, JSON_NODE_VALUE, G_TYPE_BOOLEAN },
  { "[ true, false, null ]",    3, 2, JSON_NODE_NULL,  G_TYPE_INVALID },
  { "[ 1, 2, 3.14, \"test\" ]", 4, 3, JSON_NODE_VALUE, G_TYPE_STRING  }
};

static const gchar *test_nested_arrays[] = {
  "[ 42, [ ], null ]",
  "[ [ ], [ true, [ true ] ] ]",
  "[ [ false, true, 42 ], [ true, false, 3.14 ], \"test\" ]",
  "[ true, { } ]",
  "[ false, { \"test\" : 42 } ]",
  "[ { \"test\" : 42 }, null ]",
  "[ true, { \"test\" : 42 }, null ]",
  "[ { \"channel\" : \"/meta/connect\" } ]"
};

static const struct {
  const gchar *str;
  gint size;
  const gchar *member;
  JsonNodeType type;
  GType gtype;
} test_simple_objects[] = {
  { "{ \"test\" : 42 }", 1, "test", JSON_NODE_VALUE, G_TYPE_INT64 },
  { "{ \"name\" : \"\", \"state\" : 1 }", 2, "name", JSON_NODE_VALUE, G_TYPE_STRING },
  { "{ \"foo\" : \"bar\", \"baz\" : null }", 2, "baz", JSON_NODE_NULL, G_TYPE_INVALID },
  { "{ \"channel\" : \"/meta/connect\" }", 1, "channel", JSON_NODE_VALUE, G_TYPE_STRING },
  { "{ \"halign\":0.5, \"valign\":0.5 }", 2, "valign", JSON_NODE_VALUE, G_TYPE_DOUBLE },
  { "{ \"\" : \"emptiness\" }", 1, "", JSON_NODE_VALUE, G_TYPE_STRING }
};

static const gchar *test_nested_objects[] = {
  "{ \"array\" : [ false, \"foo\" ], \"object\" : { \"foo\" : true } }",
  "{ "
    "\"type\" : \"ClutterGroup\", "
    "\"width\" : 1, "
    "\"children\" : [ "
      "{ "
        "\"type\" : \"ClutterRectangle\", "
        "\"children\" : [ "
          "{ \"type\" : \"ClutterText\", \"text\" : \"hello there\" }"
        "] "
      "}, "
      "{ "
        "\"type\" : \"ClutterGroup\", "
        "\"width\" : 1, "
        "\"children\" : [ "
          "{ \"type\" : \"ClutterText\", \"text\" : \"hello\" }"
        "] "
      "} "
    "] "
  "}"
};

static const struct {
  const gchar *str;
  const gchar *var;
  JsonNodeType type;
} test_assignments[] = {
  { "var foo = [ false, false, true ]", "foo", JSON_NODE_ARRAY },
  { "var bar = [ true, 42 ];", "bar", JSON_NODE_ARRAY },
  { "var baz = { \"foo\" : false }", "baz", JSON_NODE_OBJECT }
};

static const struct
{
  const gchar *str;
  const gchar *member;
  const gchar *match;
} test_unicode[] = {
  { "{ \"test\" : \"foo \\u00e8\" }", "test", "foo è" }
};

static guint n_test_base_values    = G_N_ELEMENTS (test_base_values);
static guint n_test_simple_arrays  = G_N_ELEMENTS (test_simple_arrays);
static guint n_test_nested_arrays  = G_N_ELEMENTS (test_nested_arrays);
static guint n_test_simple_objects = G_N_ELEMENTS (test_simple_objects);
static guint n_test_nested_objects = G_N_ELEMENTS (test_nested_objects);
static guint n_test_assignments    = G_N_ELEMENTS (test_assignments);
static guint n_test_unicode        = G_N_ELEMENTS (test_unicode);

static void
test_empty_with_parser (JsonParser *parser)
{
  GError *error = NULL;

  json_parser_load_from_data (parser, test_empty_string, -1, &error);
  g_assert_no_error (error);
  g_assert_null (json_parser_get_root (parser));
}

static void
test_empty (void)
{
  JsonParser *parser;

  /* Check with and without immutability enabled, as there have been bugs with
   * NULL root nodes on immutable parsers. */
  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));
  test_empty_with_parser (parser);
  g_object_unref (parser);

  parser = json_parser_new_immutable ();
  g_assert_true (JSON_IS_PARSER (parser));
  test_empty_with_parser (parser);
  g_object_unref (parser);
}

static void
test_base_value (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_base_values; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_base_values[i].str, -1, &error);
      g_assert_no_error (error);

      JsonNode *root = json_parser_get_root (parser);

      g_assert_nonnull (root);
      g_assert_null (json_node_get_parent (root));

      g_test_message ("Checking root node type '%s'...",
                      json_node_type_name (root));
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, test_base_values[i].type);
      g_assert_cmpint (json_node_get_value_type (root), ==, test_base_values[i].gtype);

      if (test_base_values[i].verify_value)
        test_base_values[i].verify_value (root);
    }

  g_object_unref (parser);
}

static void
test_empty_array (void)
{
  JsonParser *parser;
  GError *error = NULL;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  json_parser_load_from_data (parser, test_empty_array_string, -1, &error);

  g_assert_no_error (error);

  JsonNode *root = json_parser_get_root (parser);
  g_assert_nonnull (root);
  g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_ARRAY);
  g_assert_null (json_node_get_parent (root));

  JsonArray *array = json_node_get_array (root);
  g_assert_nonnull (array);

  g_assert_cmpint (json_array_get_length (array), ==, 0);

  g_object_unref (parser);
}

static void
test_simple_array (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_simple_arrays; i++)
    {
      GError *error = NULL;

      g_test_message ("Parsing: '%s'", test_simple_arrays[i].str);

      json_parser_load_from_data (parser, test_simple_arrays[i].str, -1, &error);

      JsonNode *root = json_parser_get_root (parser);
      g_assert_nonnull (root);

      g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_ARRAY);
      g_assert_null (json_node_get_parent (root));

      JsonArray *array = json_node_get_array (root);
      g_assert_nonnull (array);

      g_assert_cmpint (json_array_get_length (array), ==, test_simple_arrays[i].len);

      g_test_message ("checking element %d is of the desired type %s...",
                      test_simple_arrays[i].element,
                      g_type_name (test_simple_arrays[i].gtype));

      JsonNode *node = json_array_get_element (array, test_simple_arrays[i].element);
      g_assert_nonnull (node);
      g_assert_true (json_node_get_parent (node) == root);
      g_assert_cmpint (JSON_NODE_TYPE (node), ==, test_simple_arrays[i].type);
      g_assert_cmpint (json_node_get_value_type (node), ==, test_simple_arrays[i].gtype);
    }

  g_object_unref (parser);
}

static void
test_nested_array (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_nested_arrays; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_nested_arrays[i], -1, &error);
      g_assert_no_error (error);

      JsonNode *root = json_parser_get_root (parser);
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_ARRAY);
      g_assert_null (json_node_get_parent (root));

      JsonArray *array = json_node_get_array (root);
      g_assert_nonnull (array);

      g_assert_cmpint (json_array_get_length (array), >, 0);
    }

  g_object_unref (parser);
}

static void
test_empty_object (void)
{
  JsonParser *parser;
  GError *error = NULL;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  json_parser_load_from_data (parser, test_empty_object_string, -1, &error);
  g_assert_no_error (error);

  JsonNode *root = json_parser_get_root (parser);
  g_assert_nonnull (root);
  g_assert_null (json_node_get_parent (root));
  g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_OBJECT);

  JsonObject *object = json_node_get_object (root);
  g_assert_nonnull (object);
  g_assert_cmpint (json_object_get_size (object), ==, 0);

  g_object_unref (parser);
}

static void
test_simple_object (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_simple_objects; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_simple_objects[i].str, -1, &error);
      g_assert_no_error (error);

      JsonNode *root = json_parser_get_root (parser);
      g_assert_nonnull (root);

      g_test_message ("checking root node is an object...");
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_OBJECT);
      g_assert_null (json_node_get_parent (root));

      JsonObject *object = json_node_get_object (root);
      g_assert_nonnull (object);

      g_test_message ("checking object is of the desired size '%d'...",
                      test_simple_objects[i].size);
      g_assert_cmpint (json_object_get_size (object), ==, test_simple_objects[i].size);

      g_test_message ("checking member '%s' exists and is of the desired type '%s'...",
                      test_simple_objects[i].member,
                      g_type_name (test_simple_objects[i].gtype));
      JsonNode *node = json_object_get_member (object, test_simple_objects[i].member);
      g_assert_nonnull (node);
      g_assert_true (json_node_get_parent (node) == root);
      g_assert_cmpint (JSON_NODE_TYPE (node), ==, test_simple_objects[i].type);
      g_assert_cmpint (json_node_get_value_type (node), ==, test_simple_objects[i].gtype);
    }

  g_object_unref (parser);
}

static void
test_nested_object (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_nested_objects; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_nested_objects[i], -1, &error);
      g_assert_no_error (error);

      JsonNode *root = json_parser_get_root (parser);
      g_assert_nonnull (root);

      g_test_message ("checking root node is an object...");
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_OBJECT);
      g_assert_null (json_node_get_parent (root));

      JsonObject *object = json_node_get_object (root);
      g_assert_nonnull (object);

      g_test_message ("checking object is not empty...");
      g_assert_cmpint (json_object_get_size (object), >, 0);
    }

  g_object_unref (parser);
}

static void
test_assignment (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_assignments; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_assignments[i].str, -1, &error);
      g_assert_no_error (error);

      g_test_message ("checking variable '%s' is assigned...",
                      test_assignments[i].var);

      char *var = NULL;
      g_assert_true (json_parser_has_assignment (parser, &var));
      g_assert_nonnull (var);
      g_assert_cmpstr (var, ==, test_assignments[i].var);

      g_test_message ("checking for a root of the desired type...");
      JsonNode *root = json_parser_get_root (parser);
      g_assert_nonnull (root);
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, test_assignments[i].type);
    }

  g_object_unref (parser);
}

static void
test_unicode_escape (void)
{
  JsonParser *parser;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  for (guint i = 0; i < n_test_unicode; i++)
    {
      GError *error = NULL;

      json_parser_load_from_data (parser, test_unicode[i].str, -1, &error);
      g_assert_no_error (error);

      g_test_message ("checking root node is an object...");
      JsonNode *root = json_parser_get_root (parser);
      g_assert_nonnull (root);
      g_assert_cmpint (JSON_NODE_TYPE (root), ==, JSON_NODE_OBJECT);

      JsonObject *object = json_node_get_object (root);
      g_assert_nonnull (object);

      g_test_message ("checking object is not empty...");
      g_assert_cmpint (json_object_get_size (object), >, 0);

      g_test_message ("checking for object member '%s'...", test_unicode[i].member);
      JsonNode *node = json_object_get_member (object, test_unicode[i].member);
      g_assert_cmpint (JSON_NODE_TYPE (node), ==, JSON_NODE_VALUE);

      g_test_message ("checking simple string equality...");
      g_assert_cmpstr (json_node_get_string (node), ==, test_unicode[i].match);

      g_test_message ("checking for valid UTF-8...");
      g_assert_true (g_utf8_validate (json_node_get_string (node), -1, NULL));
    }

  g_object_unref (parser);
}

static void
test_stream_sync (void)
{
  JsonParser *parser;
  GFile *file;
  GFileInputStream *stream;
  GError *error = NULL;
  JsonNode *root;
  JsonArray *array;
  char *path;

  parser = json_parser_new ();

  path = g_test_build_filename (G_TEST_DIST, "stream-load.json", NULL);
  file = g_file_new_for_path (path);
  stream = g_file_read (file, NULL, &error);
  g_assert_no_error (error);
  g_assert_nonnull (stream);

  json_parser_load_from_stream (parser, G_INPUT_STREAM (stream), NULL, &error);
  g_assert_no_error (error);

  root = json_parser_get_root (parser);
  g_assert_nonnull (root);
  g_assert_true (JSON_NODE_HOLDS_ARRAY (root));

  array = json_node_get_array (root);
  g_assert_cmpint (json_array_get_length (array), ==, 1);
  g_assert_true (JSON_NODE_HOLDS_OBJECT (json_array_get_element (array, 0)));
  g_assert_true (json_object_has_member (json_array_get_object_element (array, 0), "hello"));

  g_object_unref (stream);
  g_object_unref (file);
  g_object_unref (parser);
  g_free (path);
}

/* Assert that the JSON in @parser was correctly loaded from stream-load.json. */
static void
assert_stream_load_json_correct (JsonParser *parser)
{
  JsonNode *root;
  JsonArray *array;

  root = json_parser_get_root (parser);
  g_assert_nonnull (root);
  g_assert_true (JSON_NODE_HOLDS_ARRAY (root));

  array = json_node_get_array (root);
  g_assert_cmpint (json_array_get_length (array), ==, 1);
  g_assert_true (JSON_NODE_HOLDS_OBJECT (json_array_get_element (array, 0)));
  g_assert_true (json_object_has_member (json_array_get_object_element (array, 0), "hello"));
}

static void
on_load_complete (GObject      *gobject,
                  GAsyncResult *result,
                  gpointer      user_data)
{
  JsonParser *parser = JSON_PARSER (gobject);
  GMainLoop *main_loop = user_data;
  GError *error = NULL;

  json_parser_load_from_stream_finish (parser, result, &error);
  g_assert_no_error (error);

  assert_stream_load_json_correct (parser);

  g_main_loop_quit (main_loop);
}

static void
test_stream_async (void)
{
  GMainLoop *main_loop;
  GError *error = NULL;
  JsonParser *parser = json_parser_new ();
  GFile *file;
  GFileInputStream *stream;
  char *path;

  path = g_test_build_filename (G_TEST_DIST, "stream-load.json", NULL);
  file = g_file_new_for_path (path);
  stream = g_file_read (file, NULL, &error);
  g_assert_null (error);
  g_assert_nonnull (stream);

  main_loop = g_main_loop_new (NULL, FALSE);

  json_parser_load_from_stream_async (parser, G_INPUT_STREAM (stream), NULL,
                                      on_load_complete,
                                      main_loop);

  g_main_loop_run (main_loop);

  g_main_loop_unref (main_loop);
  g_object_unref (stream);
  g_object_unref (file);
  g_object_unref (parser);
  g_free (path);
}

/* Test json_parser_load_from_mapped_file() succeeds. */
static void
test_mapped (void)
{
  GError *error = NULL;
  JsonParser *parser = json_parser_new ();
  char *path;

  path = g_test_build_filename (G_TEST_DIST, "stream-load.json", NULL);

  json_parser_load_from_mapped_file (parser, path, &error);
  g_assert_no_error (error);

  assert_stream_load_json_correct (parser);

  g_object_unref (parser);
  g_free (path);
}

/* Test json_parser_load_from_mapped_file() error handling for file I/O. */
static void
test_mapped_file_error (void)
{
  GError *error = NULL;
  JsonParser *parser = json_parser_new ();

  json_parser_load_from_mapped_file (parser, "nope.json", &error);
  g_assert_error (error, G_FILE_ERROR, G_FILE_ERROR_NOENT);

  g_assert_null (json_parser_get_root (parser));

  g_object_unref (parser);
}

/* Test json_parser_load_from_mapped_file() error handling for JSON parsing. */
static void
test_mapped_json_error (void)
{
  GError *error = NULL;
  JsonParser *parser = json_parser_new ();
  char *path;

  path = g_test_build_filename (G_TEST_DIST, "invalid.json", NULL);

  json_parser_load_from_mapped_file (parser, path, &error);
  g_assert_error (error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_INVALID_BAREWORD);

  g_assert_null (json_parser_get_root (parser));

  g_object_unref (parser);
  g_free (path);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/parser/empty-string", test_empty);
  g_test_add_func ("/parser/base-value", test_base_value);
  g_test_add_func ("/parser/empty-array", test_empty_array);
  g_test_add_func ("/parser/simple-array", test_simple_array);
  g_test_add_func ("/parser/nested-array", test_nested_array);
  g_test_add_func ("/parser/empty-object", test_empty_object);
  g_test_add_func ("/parser/simple-object", test_simple_object);
  g_test_add_func ("/parser/nested-object", test_nested_object);
  g_test_add_func ("/parser/assignment", test_assignment);
  g_test_add_func ("/parser/unicode-escape", test_unicode_escape);
  g_test_add_func ("/parser/stream-sync", test_stream_sync);
  g_test_add_func ("/parser/stream-async", test_stream_async);
  g_test_add_func ("/parser/mapped", test_mapped);
  g_test_add_func ("/parser/mapped/file-error", test_mapped_file_error);
  g_test_add_func ("/parser/mapped/json-error", test_mapped_json_error);

  return g_test_run ();
}
