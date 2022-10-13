#include "config.h"

#include <stdlib.h>
#include <stdio.h>

#include <glib.h>

#include <json-glib/json-glib.h>

static void
test_invalid_bareword (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);
  g_assert_false (res);
  g_assert_error (error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_INVALID_BAREWORD);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_invalid_assignment (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_nonnull (error);
  g_test_message ("error: %s", error->message);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_invalid_value (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_nonnull (error);
  g_test_message ("error: %s", error->message);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_invalid_array (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_nonnull (error);
  g_test_message ("error: %s", error->message);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_invalid_object (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_nonnull (error);
  g_test_message ("error: %s", error->message);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_missing_comma (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_error (error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_MISSING_COMMA);

  g_clear_error (&error);

  g_object_unref (parser);
}

static void
test_trailing_comma (gconstpointer user_data)
{
  const char *json = user_data;
  GError *error = NULL;
  JsonParser *parser;
  gboolean res;

  parser = json_parser_new ();
  g_assert_true (JSON_IS_PARSER (parser));

  g_test_message ("invalid data: '%s'...", json);

  res = json_parser_load_from_data (parser, json, -1, &error);

  g_assert_false (res);
  g_assert_error (error, JSON_PARSER_ERROR, JSON_PARSER_ERROR_TRAILING_COMMA);

  g_clear_error (&error);

  g_object_unref (parser);
}

static const struct
{
  const char *path;
  const char *json;
  gpointer func;
} test_invalid[] = {
  /* bareword */
  { "bareword-1", "rainbows", test_invalid_bareword },
  { "bareword-2", "[ unicorns ]", test_invalid_bareword },
  { "bareword-3", "{ \"foo\" : ponies }", test_invalid_bareword },
  { "bareword-4", "[ 3, 2, 1, lift_off ]", test_invalid_bareword },
  { "bareword-5", "{ foo : 42 }", test_invalid_bareword },
  { "bareword-6", "\"st\"ring\"", test_invalid_bareword },

  /* values */
  { "values-1", "[ -false ]", test_invalid_value },
  { "values-2", "[\"\\uD800\\uD800\"]", test_invalid_value },
  { "values-3", "[\"\\uDB00\\uD800\"]", test_invalid_value },
  { "values-4", "[\"\\uDB00\"]", test_invalid_value },

  /* assignment */
  { "assignment-1", "var foo", test_invalid_assignment },
  { "assignment-2", "var foo = no", test_invalid_assignment },
  { "assignment-3", "var = true", test_invalid_assignment },
  { "assignment-4", "var blah = 42:", test_invalid_assignment },
  { "assignment-5", "let foo = true;", test_invalid_assignment },

  /* arrays */
  { "array-1", "[ true, false", test_invalid_array },
  { "array-2", "[ true }", test_invalid_array },
  { "array-3", "[ \"foo\" : 42 ]", test_invalid_array },

  /* objects */
  { "object-1", "{ foo : 42 }", test_invalid_object },
  { "object-2", "{ 42 : \"foo\" }", test_invalid_object },
  { "object-3", "{ \"foo\", 42 }", test_invalid_object },
  { "object-4", "{ \"foo\" : 42 ]", test_invalid_object },
  { "object-5", "{ \"blah\" }", test_invalid_object },
  { "object-6", "{ \"a\" : 0 \"b\" : 1 }", test_invalid_object },
  { "object-7", "{ null: false }", test_invalid_object },

  /* missing commas */
  { "missing-comma-1", "[ true false ]", test_missing_comma },
  { "missing-comma-2", "{ \"foo\" : 42 \"bar\": null }", test_missing_comma },

  /* trailing commas */
  { "trailing-comma-1", "[ true, ]", test_trailing_comma },
  { "trailing-comma-2", "{ \"foo\" : 42, }", test_trailing_comma },
};

static guint n_test_invalid = G_N_ELEMENTS (test_invalid);

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  for (guint i = 0; i < n_test_invalid; i++)
    {
      char *test_path = g_strconcat ("/invalid/json/", test_invalid[i].path, NULL);

      g_test_add_data_func_full (test_path,
                                 (gpointer) test_invalid[i].json,
                                 test_invalid[i].func,
                                 NULL);

      g_free (test_path);
    }

  return g_test_run ();
}
