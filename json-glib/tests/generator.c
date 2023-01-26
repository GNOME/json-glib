#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include <json-glib/json-glib.h>

#include <locale.h>

static const gchar *empty_array  = "[]";
static const gchar *empty_object = "{}";

static const gchar *simple_array = "[true,false,null,42,\"foo\"]";
static const gchar *nested_array = "[true,[false,null],42]";

static const gchar *simple_object = "{\"Bool1\":true,\"Bool2\":false,\"Null\":null,\"Int\":42,\"\":54,\"String\":\"foo\"}";
/* taken from the RFC 4627, Examples section */
static const gchar *nested_object =
"{"
  "\"Image\":{"
    "\"Width\":800,"
    "\"Height\":600,"
    "\"Title\":\"View from 15th Floor\","
    "\"Thumbnail\":{"
      "\"Url\":\"http://www.example.com/image/481989943\","
      "\"Height\":125,"
      "\"Width\":\"100\""
    "},"
    "\"IDs\":[116,943,234,38793]"
  "}"
"}";

static const char *pretty_examples[] = {
 "[]",

 "{}",

 "[\n"
 "\ttrue,\n"
 "\tfalse,\n"
 "\tnull,\n"
 "\t\"hello\"\n"
 "]",

 "{\n"
 "\t\"foo\" : 42,\n"
 "\t\"bar\" : true,\n"
 "\t\"baz\" : null\n"
 "}",
};

static const int n_pretty_examples = G_N_ELEMENTS (pretty_examples);

static const struct {
  const gchar *lang;
  const gchar *sep;
  guint matches : 1;
} decimal_separator[] = {
  { "C", ".",  TRUE },
  { "de", ",", FALSE },
  { "en", ".", TRUE },
  { "fr", ",", FALSE }
};

static void
test_empty_array (void)
{
  JsonGenerator *gen = json_generator_new ();
  JsonNode *root;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_ARRAY);
  json_node_take_array (root, json_array_new ());

  json_generator_set_root (gen, root);
  g_object_set (gen, "pretty", FALSE, "indent", 0, "indent-char", ' ', NULL);

  data = json_generator_to_data (gen, &len);

  g_assert_cmpint (len, ==, strlen (empty_array));
  g_assert_cmpstr (data, ==, empty_array);

  g_assert_false (json_generator_get_pretty (gen));
  g_assert_cmpint (json_generator_get_indent (gen), ==, 0);
  g_assert_cmpint (json_generator_get_indent_char (gen), ==, ' ');

  g_free (data);
  json_node_free (root);
  g_object_unref (gen);
}

static void
test_empty_object (void)
{
  JsonGenerator *gen = json_generator_new ();
  JsonNode *root;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_OBJECT);
  json_node_take_object (root, json_object_new ());

  json_generator_set_root (gen, root);
  g_object_set (gen, "pretty", FALSE, NULL);

  data = json_generator_to_data (gen, &len);

  g_assert_cmpint (len, ==, strlen (empty_object));
  g_assert_cmpstr (data, ==, empty_object);

  g_free (data);
  json_node_free (root);
  g_object_unref (gen);
}

static void
test_simple_array (void)
{
  JsonGenerator *generator = json_generator_new ();
  JsonNode *root;
  JsonArray *array;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_ARRAY);
  array = json_array_sized_new (5);

  json_array_add_boolean_element (array, TRUE);
  json_array_add_boolean_element (array, FALSE);
  json_array_add_null_element (array);
  json_array_add_int_element (array, 42);
  json_array_add_string_element (array, "foo");

  json_node_take_array (root, array);
  json_generator_set_root (generator, root);

  g_object_set (generator, "pretty", FALSE, NULL);
  data = json_generator_to_data (generator, &len);

  g_test_message ("checking simple array '%s' (expected: '%s')",
                  data,
                  simple_array);

  g_assert_cmpint (len, ==, strlen (simple_array));
  g_assert_cmpstr (data, ==, simple_array);

  g_free (data);
  json_node_free (root);
  g_object_unref (generator);
}

static void
test_nested_array (void)
{
  JsonGenerator *generator = json_generator_new ();
  JsonNode *root;
  JsonArray *array, *nested;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_ARRAY);
  array = json_array_sized_new (3);

  json_array_add_boolean_element (array, TRUE);

  {
    nested = json_array_sized_new (2);

    json_array_add_boolean_element (nested, FALSE);
    json_array_add_null_element (nested);

    json_array_add_array_element (array, nested);
  }

  json_array_add_int_element (array, 42);

  json_node_take_array (root, array);
  json_generator_set_root (generator, root);

  g_object_set (generator, "pretty", FALSE, NULL);
  data = json_generator_to_data (generator, &len);

  g_assert_cmpint (len, ==, strlen (nested_array));
  g_assert_cmpstr (data, ==, nested_array);

  g_free (data);
  json_node_free (root);
  g_object_unref (generator);
}

static void
test_simple_object (void)
{
  JsonGenerator *generator = json_generator_new ();
  JsonNode *root;
  JsonObject *object;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_OBJECT);
  object = json_object_new ();

  json_object_set_boolean_member (object, "Bool1", TRUE);
  json_object_set_boolean_member (object, "Bool2", FALSE);
  json_object_set_null_member (object, "Null");
  json_object_set_int_member (object, "Int", 42);
  json_object_set_int_member (object, "", 54);
  json_object_set_string_member (object, "String", "foo");

  json_node_take_object (root, object);
  json_generator_set_root (generator, root);

  g_object_set (generator, "pretty", FALSE, NULL);
  data = json_generator_to_data (generator, &len);

  g_test_message ("checking simple object '%s' (expected: '%s')",
                  data,
                  simple_object);

  g_assert_cmpint (len, ==, strlen (simple_object));
  g_assert_cmpstr (data, ==, simple_object);

  g_free (data);
  json_node_free (root);
  g_object_unref (generator);
}

static void
test_nested_object (void)
{
  JsonGenerator *generator = json_generator_new ();
  JsonNode *root;
  JsonObject *object, *nested;
  JsonArray *array;
  gchar *data;
  gsize len;

  root = json_node_new (JSON_NODE_OBJECT);
  object = json_object_new ();

  json_object_set_int_member (object, "Width", 800);
  json_object_set_int_member (object, "Height", 600);
  json_object_set_string_member (object, "Title", "View from 15th Floor");

  {
    nested = json_object_new ();

    json_object_set_string_member (nested, "Url", "http://www.example.com/image/481989943");
    json_object_set_int_member (nested, "Height", 125);
    json_object_set_string_member (nested, "Width", "100");

    json_object_set_object_member (object, "Thumbnail", nested);
  }

  {
    array = json_array_new ();

    json_array_add_int_element (array, 116);
    json_array_add_int_element (array, 943);
    json_array_add_int_element (array, 234);
    json_array_add_int_element (array, 38793);

    json_object_set_array_member (object, "IDs", array);
  }

  nested = json_object_new ();
  json_object_set_object_member (nested, "Image", object);

  json_node_take_object (root, nested);
  json_generator_set_root (generator, root);

  g_object_set (generator, "pretty", FALSE, NULL);
  data = json_generator_to_data (generator, &len);

  g_test_message ("checking nested object '%s' (expected: '%s')",
                  data,
                  nested_object);

  g_assert_cmpint (len, ==, strlen (nested_object));
  g_assert_cmpstr (data, ==, nested_object);

  g_free (data);
  json_node_free (root);
  g_object_unref (generator);
}

static void
test_decimal_separator (void)
{
  JsonNode *node = json_node_new (JSON_NODE_VALUE);
  JsonGenerator *generator = json_generator_new ();
  gchar *old_locale;

  json_node_set_double (node, 3.14);

  json_generator_set_root (generator, node);

  old_locale = setlocale (LC_NUMERIC, NULL);

  for (guint i = 0; i < G_N_ELEMENTS (decimal_separator); i++)
    {
      gchar *str, *expected;

      setlocale (LC_NUMERIC, decimal_separator[i].lang);

      str = json_generator_to_data (generator, NULL);

      g_test_message ("%s: value: '%.2f' - string: '%s'",
                      G_STRFUNC,
                      json_node_get_double (node),
                      str);

      g_assert_nonnull (str);
      expected = strstr (str, decimal_separator[i].sep);
      if (decimal_separator[i].matches)
        g_assert_nonnull (expected);
      else
        g_assert_null (expected);

      g_free (str);
   }

  setlocale (LC_NUMERIC, old_locale);

  g_object_unref (generator);
  json_node_free (node);
}


static void
test_double_stays_double (void)
{
  gchar *str;
  JsonNode *node = json_node_new (JSON_NODE_VALUE);
  JsonGenerator *generator = json_generator_new ();

  json_node_set_double (node, 1.0);

  json_generator_set_root (generator, node);

  str = json_generator_to_data (generator, NULL);
  g_assert_cmpstr (str, ==, "1.0");

  g_free (str);
  g_object_unref (generator);
  json_node_free (node);
}

static void
test_double_valid (void)
{
  gchar *str;
  JsonNode *node = json_node_new (JSON_NODE_VALUE);
  JsonGenerator *generator = json_generator_new ();

  json_node_set_double (node, 1e-8);
  json_generator_set_root (generator, node);

  str = json_generator_to_data (generator, NULL);
  g_test_message ("%s: value: '%.2f' - string: '%s'",
                G_STRFUNC,
                json_node_get_double (node),
                str);

  /* should be valid double
   * in particular; no trailing .0 for exponential notation */
  gchar *end = NULL;
  g_ascii_strtod(str, &end);
  g_assert_cmpint (0, ==, *end);

  g_free (str);
  g_object_unref (generator);
  json_node_free (node);
}


static void
test_pretty (void)
{
  JsonParser *parser = json_parser_new ();
  JsonGenerator *generator = json_generator_new ();
  int i;

  json_generator_set_pretty (generator, TRUE);
  json_generator_set_indent (generator, 1);
  json_generator_set_indent_char (generator, '\t');

  for (i = 0; i < n_pretty_examples; i++)
    {
      JsonNode *root;
      char *data;
      gsize len;

      g_assert_true (json_parser_load_from_data (parser, pretty_examples[i], -1, NULL));

      root = json_parser_get_root (parser);
      g_assert_nonnull (root);

      json_generator_set_root (generator, root);

      data = json_generator_to_data (generator, &len);

      g_test_message ("checking pretty printing: %s\texpected: %s",
                      data,
                      pretty_examples[i]);

      g_assert_cmpint (len, ==, strlen (pretty_examples[i]));
      g_assert_cmpstr (data, ==, pretty_examples[i]);

      g_free (data);
    }

  g_object_unref (generator);
  g_object_unref (parser);
}

typedef struct {
    const gchar *str;
    const gchar *expect;
} FixtureString;

static const FixtureString string_fixtures[] = {
  { "abc", "\"abc\"" },
  { "a\x7fxc", "\"a\\u007fxc\"" },
  { "a\033xc", "\"a\\u001bxc\"" },
  { "a\nxc", "\"a\\nxc\"" },
  { "a\\xc", "\"a\\\\xc\"" },
  { "Barney B\303\244r", "\"Barney B\303\244r\"" },
};

static void
test_string_encode (gconstpointer data)
{
  const FixtureString *fixture = data;
  JsonGenerator *generator = json_generator_new ();
  JsonNode *node;
  gsize length;
  gchar *output;

  node = json_node_init_string (json_node_alloc (), fixture->str);\
  json_generator_set_root (generator, node);

  output = json_generator_to_data (generator, &length);
  g_assert_cmpstr (output, ==, fixture->expect);
  g_assert_cmpuint (length, ==, strlen (fixture->expect));
  g_free (output);
  json_node_free (node);

  g_object_unref (generator);
}
int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/generator/empty-array", test_empty_array);
  g_test_add_func ("/generator/empty-object", test_empty_object);
  g_test_add_func ("/generator/simple-array", test_simple_array);
  g_test_add_func ("/generator/nested-array", test_nested_array);
  g_test_add_func ("/generator/simple-object", test_simple_object);
  g_test_add_func ("/generator/nested-object", test_nested_object);
  g_test_add_func ("/generator/decimal-separator", test_decimal_separator);
  g_test_add_func ("/generator/double-stays-double", test_double_stays_double);
  g_test_add_func ("/generator/double-valid", test_double_valid);
  g_test_add_func ("/generator/pretty", test_pretty);

  for (guint i = 0; i < G_N_ELEMENTS (string_fixtures); i++)
    {
      char *escaped = g_strescape (string_fixtures[i].str, NULL);
      char *name = g_strdup_printf ("/generator/string/%s", escaped);
      g_test_add_data_func (name, string_fixtures + i, test_string_encode);
      g_free (escaped);
      g_free (name);
    }

  return g_test_run ();
}
