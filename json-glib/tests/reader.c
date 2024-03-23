// SPDX-FileCopyrightText: 2010 Intel Ltd.
// SPDX-FileCopyrightText: 2014 Bastien Nocera
// SPDX-FileCopyrightText: 2014 Philip Withnall
// SPDX-FileCopyrightText: 2017 Endless
// SPDX-FileCopyrightText: 2020 Jan-Michael Brummer
// SPDX-FileCopyrightText: 2022 Frederic Martinsons
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <string.h>
#include <json-glib/json-glib.h>

static const gchar *test_base_array_data =
"[ 0, true, null, \"foo\", 3.14, [ false ], { \"bar\" : 42 } ]";

static const gchar *test_base_object_data =
"{ \"text\" : \"hello, world!\", \"foo\" : null, \"blah\" : 47, \"double\" : 42.47 }";

static const gchar *test_reader_level_data =
" { \"list\": { \"181195771\": { \"given_url\": \"http://www.gnome.org/json-glib-test\" } } }";

static const gchar *test_reader_current_node_data =
" { \"object\": { \"subobject\": { \"key\": \"value\" } } }";

/* https://bugzilla.gnome.org/show_bug.cgi?id=758580 */
static const char *test_reader_null_value_data =
"{ \"v\": null }";

static const gchar *expected_member_name[] = {
  "text",
  "foo",
  "blah",
  "double",
};

static void
test_base_object (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  GError *error = NULL;
  gchar **members;
  gsize n_members, i;

  json_parser_load_from_data (parser, test_base_object_data, -1, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  g_assert_true (json_reader_is_object (reader));
  g_assert_cmpint (json_reader_count_members (reader), ==, 4);

  members = json_reader_list_members (reader);
  g_assert_nonnull (members);

  n_members = g_strv_length (members);
  g_assert_cmpint (n_members, ==, json_reader_count_members (reader));

  for (i = 0; i < n_members; i++)
    g_assert_cmpstr (members[i], ==, expected_member_name[i]);

  g_strfreev (members);

  g_assert_true (json_reader_read_member (reader, "text"));
  g_assert_true (json_reader_is_value (reader));
  g_assert_cmpstr (json_reader_get_string_value (reader), ==, "hello, world!");
  json_reader_end_member (reader);

  g_assert_true (json_reader_read_member (reader, "foo"));
  g_assert_true (json_reader_get_null_value (reader));
  json_reader_end_member (reader);

  g_assert_false (json_reader_read_member (reader, "bar"));
  g_assert_nonnull (json_reader_get_error (reader));
  g_assert_error ((GError *) json_reader_get_error (reader),
                  JSON_READER_ERROR,
                  JSON_READER_ERROR_INVALID_MEMBER);
  json_reader_end_member (reader);
  g_assert_no_error (json_reader_get_error (reader));

  g_assert_true (json_reader_read_element (reader, 2));
  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "blah");
  g_assert_true (json_reader_is_value (reader));
  g_assert_cmpint (json_reader_get_int_value (reader), ==, 47);
  json_reader_end_element (reader);
  g_assert_no_error (json_reader_get_error (reader));

  json_reader_read_member (reader, "double");
  g_assert_cmpfloat_with_epsilon (json_reader_get_double_value (reader), 42.47, 0.01);
  json_reader_end_element (reader);

  g_object_unref (reader);
  g_object_unref (parser);
}

static void
test_base_array (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  GError *error = NULL;

  json_parser_load_from_data (parser, test_base_array_data, -1, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  g_assert_true (json_reader_is_array (reader));
  g_assert_cmpint (json_reader_count_elements (reader), ==, 7);

  json_reader_read_element (reader, 0);
  g_assert_true (json_reader_is_value (reader));
  g_assert_cmpint (json_reader_get_int_value (reader), ==, 0);
  json_reader_end_element (reader);

  json_reader_read_element (reader, 1);
  g_assert_true (json_reader_is_value (reader));
  g_assert_true (json_reader_get_boolean_value (reader));
  json_reader_end_element (reader);

  json_reader_read_element (reader, 3);
  g_assert_true (json_reader_is_value (reader));
  g_assert_cmpstr (json_reader_get_string_value (reader), ==, "foo");
  json_reader_end_element (reader);

  json_reader_read_element (reader, 5);
  g_assert_false (json_reader_is_value (reader));
  g_assert_true (json_reader_is_array (reader));
  json_reader_end_element (reader);

  json_reader_read_element (reader, 6);
  g_assert_true (json_reader_is_object (reader));

  json_reader_read_member (reader, "bar");
  g_assert_true (json_reader_is_value (reader));
  g_assert_cmpint (json_reader_get_int_value (reader), ==, 42);
  json_reader_end_member (reader);

  json_reader_end_element (reader);

  g_assert_false (json_reader_read_element (reader, 7));
  g_assert_nonnull (json_reader_get_error (reader));
  g_assert_error ((GError *) json_reader_get_error (reader),
                  JSON_READER_ERROR,
                  JSON_READER_ERROR_INVALID_INDEX);
  json_reader_end_element (reader);
  g_assert_no_error (json_reader_get_error (reader));

  g_object_unref (reader);
  g_object_unref (parser);
  g_clear_error (&error);
}

static void
test_reader_level (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  GError *error = NULL;
  char **members;

  json_parser_load_from_data (parser, test_reader_level_data, -1, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  g_assert_cmpint (json_reader_count_members (reader), >, 0);

  /* Grab the list */
  g_assert_true (json_reader_read_member (reader, "list"));
  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "list");

  members = json_reader_list_members (reader);
  g_assert_nonnull (members);
  g_strfreev (members);

  g_assert_true (json_reader_read_member (reader, "181195771"));
  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "181195771");

  g_assert_false (json_reader_read_member (reader, "resolved_url"));
  g_assert_cmpstr (json_reader_get_member_name (reader), ==, NULL);
  g_assert_nonnull (json_reader_get_error (reader));
  json_reader_end_member (reader);

  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "181195771");

  g_assert_true (json_reader_read_member (reader, "given_url"));
  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "given_url");
  g_assert_cmpstr (json_reader_get_string_value (reader), ==, "http://www.gnome.org/json-glib-test");
  json_reader_end_member (reader);

  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "181195771");

  json_reader_end_member (reader);

  g_assert_cmpstr (json_reader_get_member_name (reader), ==, "list");

  json_reader_end_member (reader);

  g_assert_cmpstr (json_reader_get_member_name (reader), ==, NULL);

  g_clear_object (&reader);
  g_clear_object (&parser);
}

static void
test_reader_null_value (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  GError *error = NULL;

  g_test_bug ("758580");

  json_parser_load_from_data (parser, test_reader_null_value_data, -1, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  json_reader_read_member (reader, "v");
  g_assert_true (json_reader_is_value (reader));
  g_assert_no_error (json_reader_get_error (reader));
  g_assert_nonnull (json_reader_get_value (reader));

  g_object_unref (reader);
  g_object_unref (parser);
}

/* test_reader_skip_bom: Ensure that a BOM Unicode character is skipped when parsing */
static void
test_reader_skip_bom (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  GError *error = NULL;
  char *path;

  path = g_test_build_filename (G_TEST_DIST, "skip-bom.json", NULL);

  json_parser_load_from_mapped_file (parser, path, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  json_reader_read_member (reader, "appName");
  g_assert_true (json_reader_is_value (reader));
  g_assert_no_error (json_reader_get_error (reader));
  g_assert_cmpstr (json_reader_get_string_value (reader), ==, "String starts with BOM");

  g_free (path);
  g_object_unref (reader);
  g_object_unref (parser);
}

static void
test_reader_current_node (void)
{
  JsonParser *parser = json_parser_new ();
  JsonReader *reader = json_reader_new (NULL);
  JsonNode *current = NULL;
  gchar* node_string = NULL;
  const gchar* expected_data = "{\"subobject\":{\"key\":\"value\"}}";
  GError *error = NULL;

  g_assert_null (json_reader_get_current_node (reader));

  json_parser_load_from_data (parser, test_reader_current_node_data, -1, &error);
  g_assert_no_error (error);

  json_reader_set_root (reader, json_parser_get_root (parser));

  /* Grab the list */
  g_assert_true (json_reader_read_member (reader, "object"));

  /* Get the node at the current position */
  current = json_reader_get_current_node (reader);
  g_assert_nonnull (current);

  /* Check that it contains the data waited */
  node_string = json_to_string (current, FALSE);
  g_assert_cmpstr (node_string, ==, expected_data);
  g_free (node_string);

  json_reader_end_member (reader);

  g_clear_object (&reader);
  g_clear_object (&parser);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);
  g_test_bug_base ("http://bugzilla.gnome.org/show_bug.cgi?id=");

  g_test_add_func ("/reader/base-array", test_base_array);
  g_test_add_func ("/reader/base-object", test_base_object);
  g_test_add_func ("/reader/level", test_reader_level);
  g_test_add_func ("/reader/null-value", test_reader_null_value);
  g_test_add_func ("/reader/bom", test_reader_skip_bom);
  g_test_add_func ("/reader/currrent-node", test_reader_current_node);

  return g_test_run ();
}
