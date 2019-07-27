#include <string.h>
#include <glib.h>
#include <json-glib/json-glib.h>

static const char *test_json =
"{ \"store\": {"
"    \"book\": [ "
"      { \"category\": \"reference\","
"        \"author\": \"Nigel Rees\","
"        \"title\": \"Sayings of the Century\","
"        \"price\": \"8.95\""
"      },"
"      { \"category\": \"fiction\","
"        \"author\": \"Evelyn Waugh\","
"        \"title\": \"Sword of Honour\","
"        \"price\": \"12.99\""
"      },"
"      { \"category\": \"fiction\","
"        \"author\": \"Herman Melville\","
"        \"title\": \"Moby Dick\","
"        \"isbn\": \"0-553-21311-3\","
"        \"price\": \"8.99\""
"      },"
"      { \"category\": \"fiction\","
"        \"author\": \"J. R. R. Tolkien\","
"        \"title\": \"The Lord of the Rings\","
"        \"isbn\": \"0-395-19395-8\","
"        \"price\": \"22.99\""
"      }"
"    ],"
"    \"bicycle\": {"
"      \"color\": \"red\","
"      \"price\": \"19.95\""
"    }"
"  }"
"}";

static const struct {
  const char *desc;
  const char *expr;
  const char *res;

  guint is_valid : 1;

  JsonPointerError error_code;
} test_expressions[] = {
  {
    "INVALID: invalid escape sequence (~2)",
    "/foo~2bar",
    NULL,
    FALSE,
    JSON_POINTER_ERROR_SYNTAX_ERROR,
  },
  {
    "INVALID: invalid escape sequence (~?) (0x02)",
    "/foo~\002bar",
    NULL,
    FALSE,
    JSON_POINTER_ERROR_SYNTAX_ERROR,
  },
  {
    "INVALID: invalid first character",
    "foo/bar",
    NULL,
    FALSE,
    JSON_POINTER_ERROR_SYNTAX_ERROR,
  },
  {
    "Title of the first book in the store",
    "/store/book/0/title",
    "\"Sayings of the Century\"",
    TRUE,
    0
  },
  {
    "The third book.",
    "/store/book/2",
    "{\"category\":\"fiction\",\"author\":\"Herman Melville\",\"title\":\"Moby Dick\",\"isbn\":\"0-553-21311-3\",\"price\":\"8.99\"}",
    TRUE,
    0
  },
  {
    "The last book.",
    "/store/book/3",
    "{\"category\":\"fiction\",\"author\":\"J. R. R. Tolkien\",\"title\":\"The Lord of the Rings\",\"isbn\":\"0-395-19395-8\",\"price\":\"22.99\"}",
    TRUE,
    0
  },
  {
    "The root node.",
    "",
    "{\"store\":{\"book\":[{\"category\":\"reference\",\"author\":\"Nigel Rees\",\"title\":\"Sayings of the Century\",\"price\":\"8.95\"},"
                           "{\"category\":\"fiction\",\"author\":\"Evelyn Waugh\",\"title\":\"Sword of Honour\",\"price\":\"12.99\"},"
                           "{\"category\":\"fiction\",\"author\":\"Herman Melville\",\"title\":\"Moby Dick\",\"isbn\":\"0-553-21311-3\",\"price\":\"8.99\"},"
                           "{\"category\":\"fiction\",\"author\":\"J. R. R. Tolkien\",\"title\":\"The Lord of the Rings\",\"isbn\":\"0-395-19395-8\",\"price\":\"22.99\"}],"
                 "\"bicycle\":{\"color\":\"red\",\"price\":\"19.95\"}}}",
    TRUE,
    0
  },
  {
    "Not found",
    "/nonexistent",
    "",
    TRUE,
    0
  }
};

static void
pointer_expressions_valid (gconstpointer data)
{
  const int index_ = GPOINTER_TO_INT (data);
  const char *expr = test_expressions[index_].expr;
  const char *desc = test_expressions[index_].desc;

  JsonPointer *pointer = json_pointer_new ();
  GError *error = NULL;

  if (g_test_verbose ())
    g_print ("* %s ('%s')\n", desc, expr);

  g_assert (json_pointer_compile (pointer, expr, &error));
  g_assert_no_error (error);

  g_object_unref (pointer);
}

static void
pointer_expressions_invalid (gconstpointer data)
{
  const int index_ = GPOINTER_TO_INT (data);
  const char *expr = test_expressions[index_].expr;
  const char *desc = test_expressions[index_].desc;
  const JsonPointerError code = test_expressions[index_].error_code;

  JsonPointer *pointer = json_pointer_new ();
  GError *error = NULL;

  if (g_test_verbose ())
    g_print ("* %s ('%s')\n", desc, expr);


  g_assert (!json_pointer_compile (pointer, expr, &error));
  g_assert_error (error, JSON_POINTER_ERROR, (gint) code);

  g_object_unref (pointer);
  g_clear_error (&error);
}

static void
pointer_match (gconstpointer data)
{
  const int index_ = GPOINTER_TO_INT (data);
  const char *desc = test_expressions[index_].desc;
  const char *expr = test_expressions[index_].expr;
  const char *res  = test_expressions[index_].res;

  JsonParser *parser = json_parser_new ();
  JsonGenerator *gen = json_generator_new ();
  JsonPointer *pointer = json_pointer_new ();
  JsonNode *root;
  JsonNode *matches;
  char *str;

  json_parser_load_from_data (parser, test_json, -1, NULL);
  root = json_parser_get_root (parser);

  g_assert (json_pointer_compile (pointer, expr, NULL));

  matches = json_pointer_match (pointer, root);

  json_generator_set_root (gen, matches);
  str = json_generator_to_data (gen, NULL);

  if (g_test_verbose ())
    {
      g_print ("* %s ('%s') =>\n"
               "- result:   %s\n"
               "- expected: %s\n",
               desc, expr,
               str,
               res);
    }

  g_assert_cmpstr (str, ==, res);

  g_free (str);
  if (matches != NULL)
    json_node_unref (matches);

  g_object_unref (parser);
  g_object_unref (pointer);
  g_object_unref (gen);
}

int
main (int   argc,
      char *argv[])
{
  guint i, j;

  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

  for (i = 0, j = 1; i < G_N_ELEMENTS (test_expressions); i++)
    {
      char *pointer;

      if (!test_expressions[i].is_valid)
        continue;

      pointer = g_strdup_printf ("/pointer/expressions/valid/%d", j++);

      g_test_add_data_func (pointer, GINT_TO_POINTER (i), pointer_expressions_valid);

      g_free (pointer);
    }

  for (i = 0, j = 1; i < G_N_ELEMENTS (test_expressions); i++)
    {
      char *pointer;

      if (test_expressions[i].is_valid)
        continue;

      pointer = g_strdup_printf ("/pointer/expressions/invalid/%d", j++);

      g_test_add_data_func (pointer, GINT_TO_POINTER (i), pointer_expressions_invalid);

      g_free (pointer);
    }

  for (i = 0, j = 1; i < G_N_ELEMENTS (test_expressions); i++)
    {
      char *pointer;

      if (!test_expressions[i].is_valid || test_expressions[i].res == NULL)
        continue;

      pointer = g_strdup_printf ("/pointer/match/%d", j++);

      g_test_add_data_func (pointer, GINT_TO_POINTER (i), pointer_match);

      g_free (pointer);
    }

  return g_test_run ();
}
