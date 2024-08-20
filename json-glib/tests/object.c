// SPDX-FileCopyrightText: 2008 OpenedHand Ltd.
// SPDX-FileCopyrightText: 2009 Intel Ltd.
// SPDX-FileCopyrightText: 2015 Collabora Ltd.
// SPDX-FileCopyrightText: 2022 Frederic Martinsons
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <json-glib/json-glib.h>

static void
test_empty_object (void)
{
  JsonObject *object = json_object_new ();

  g_assert_cmpint (json_object_get_size (object), ==, 0);
  g_assert_null (json_object_get_members (object));

  json_object_unref (object);
}

static void
test_add_member (void)
{
  JsonObject *object = json_object_new ();
  JsonNode *node = json_node_new (JSON_NODE_NULL);

  g_assert_cmpint (json_object_get_size (object), ==, 0);

  json_object_set_member (object, "Null", node);
  g_assert_cmpint (json_object_get_size (object), ==, 1);

  node = json_object_get_member (object, "Null");
  g_assert_cmpint (JSON_NODE_TYPE (node), ==, JSON_NODE_NULL);

  json_object_unref (object);
}

static void
test_set_member (void)
{
  JsonNode *node = json_node_new (JSON_NODE_VALUE);
  JsonObject *object = json_object_new ();

  g_assert_cmpint (json_object_get_size (object), ==, 0);

  json_node_set_string (node, "Hello");

  json_object_set_member (object, "String", node);
  g_assert_cmpint (json_object_get_size (object), ==, 1);

  node = json_object_get_member (object, "String");
  g_assert_cmpint (JSON_NODE_TYPE (node), ==, JSON_NODE_VALUE);
  g_assert_cmpstr (json_node_get_string (node), ==, "Hello");

  json_object_set_string_member (object, "String", "World");
  node = json_object_get_member (object, "String");
  g_assert_cmpint (JSON_NODE_TYPE (node), ==, JSON_NODE_VALUE);
  g_assert_cmpstr (json_node_get_string (node), ==, "World");

  json_object_set_string_member (object, "String", "Goodbye");
  g_assert_cmpstr (json_object_get_string_member (object, "String"), ==, "Goodbye");

  json_object_set_array_member (object, "Array", NULL);
  g_assert_cmpint (JSON_NODE_TYPE (json_object_get_member (object, "Array")), ==, JSON_NODE_NULL);

  json_object_set_object_member (object, "Object", NULL);
  g_assert_true (json_object_get_null_member (object, "Object"));

  json_object_set_object_member (object, "", NULL);
  g_assert_true (json_object_get_null_member (object, ""));

  json_object_unref (object);
}

static void
test_get_member_default (void)
{
  JsonObject *object = json_object_new ();
  JsonObject *empty = json_object_new ();

  json_object_set_int_member (object, "foo", 42);
  json_object_set_boolean_member (object, "bar", TRUE);
  json_object_set_string_member (object, "hello", "world");
  json_object_set_double_member (object, "pi", 3.); /* close enough */
  json_object_set_object_member (object, "empty", g_steal_pointer (&empty));

  /* Keys exists, with correctly-typed values */
  g_assert_cmpint (json_object_get_int_member_with_default (object, "foo", 47), ==, 42);
  g_assert_true (json_object_get_boolean_member_with_default (object, "bar", FALSE));
  g_assert_cmpstr (json_object_get_string_member_with_default (object, "hello", "wisconsin"), ==, "world");
  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "pi", 3.142), ==, 3.);

  /* Keys do not exist */
  g_assert_cmpint (json_object_get_int_member_with_default (object, "no", 4), ==, 4);
  g_assert_true (json_object_get_boolean_member_with_default (object, "no", TRUE));
  g_assert_cmpstr (json_object_get_string_member_with_default (object, "doesNotExist", "indeed"), ==, "indeed");
  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "doesNotExist", 4.), ==, 4.);

  /* Keys exist, but with wrongly-typed scalar values. */
  g_assert_cmpint (json_object_get_int_member_with_default (object, "hello", 42), ==, 0);
  g_assert_cmpint (json_object_get_int_member_with_default (object, "pi", 42), ==, 3);
  g_assert_cmpint (json_object_get_int_member_with_default (object, "bar", 42), ==, 1);

  g_assert_false (json_object_get_boolean_member_with_default (object, "hello", TRUE));
  g_assert_true (json_object_get_boolean_member_with_default (object, "foo", FALSE));

  g_assert_cmpstr (json_object_get_string_member_with_default (object, "foo", "wisconsin"), ==, NULL);

  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "foo", 1.), ==, 42);
  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "bar", 3.), ==, 1);
  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "hello", 3.), ==, 0);

  /* Key exists, with non-scalar value */
  g_assert_cmpint (json_object_get_int_member_with_default (object, "empty", 1), ==, 1);
  g_assert_true (json_object_get_boolean_member_with_default (object, "empty", TRUE));
  g_assert_cmpstr (json_object_get_string_member_with_default (object, "empty", "x"), ==, "x");
  g_assert_cmpfloat (json_object_get_double_member_with_default (object, "empty", 1.), ==, 1.);

  json_object_unref (object);
}

static void
test_remove_member (void)
{
  JsonObject *object = json_object_new ();
  JsonNode *node = json_node_new (JSON_NODE_NULL);

  json_object_set_member (object, "Null", node);

  json_object_remove_member (object, "Null");
  g_assert_cmpint (json_object_get_size (object), ==, 0);

  json_object_unref (object);
}

typedef struct _TestForeachFixture
{
  gint n_members;
  gboolean ordered;
} TestForeachFixture;

static const struct {
  const gchar *member_name;
  JsonNodeType member_type;
  GType member_gtype;
} type_verify[] = {
  { "integer", JSON_NODE_VALUE, G_TYPE_INT64 },
  { "boolean", JSON_NODE_VALUE, G_TYPE_BOOLEAN },
  { "string", JSON_NODE_VALUE, G_TYPE_STRING },
  { "double", JSON_NODE_VALUE, G_TYPE_DOUBLE },
  { "null", JSON_NODE_NULL, G_TYPE_INVALID },
  { "", JSON_NODE_VALUE, G_TYPE_INT64 }
};

static void
verify_foreach (JsonObject  *object G_GNUC_UNUSED,
                const gchar *member_name,
                JsonNode    *member_node,
                gpointer     user_data)
{
  TestForeachFixture *fixture = user_data;

  if (fixture->ordered)
    {
      int idx = fixture->n_members;

      g_assert_cmpstr (member_name, ==, type_verify[idx].member_name);
      g_assert_true (json_node_get_node_type (member_node) == type_verify[idx].member_type);
      g_assert_true (json_node_get_value_type (member_node) == type_verify[idx].member_gtype);
    }
  else
    {
      for (guint i = 0; i < G_N_ELEMENTS (type_verify); i++)
        {
          if (strcmp (member_name, type_verify[i].member_name) == 0)
            {
              g_assert_true (json_node_get_node_type (member_node) == type_verify[i].member_type);
              g_assert_true (json_node_get_value_type (member_node) == type_verify[i].member_gtype);
              break;
            }
        }
    }

  fixture->n_members += 1;
}

static void
test_foreach_member (void)
{
  JsonObject *object = json_object_new ();
  TestForeachFixture fixture = { 0, TRUE, };

  json_object_set_int_member (object, "integer", 42);
  json_object_set_boolean_member (object, "boolean", TRUE);
  json_object_set_string_member (object, "string", "hello");
  json_object_set_double_member (object, "double", 3.14159);
  json_object_set_null_member (object, "null");
  json_object_set_int_member (object, "", 0);

  json_object_foreach_member (object, verify_foreach, &fixture);

  g_assert_cmpint (fixture.n_members, ==, json_object_get_size (object));

  json_object_unref (object);
}

static void
test_iter (void)
{
  JsonObject *object = NULL;
  TestForeachFixture fixture = { 0, FALSE, };
  JsonObjectIter iter;
  const gchar *member_name;
  JsonNode *member_node;

  object = json_object_new ();

  json_object_set_int_member (object, "integer", 42);
  json_object_set_boolean_member (object, "boolean", TRUE);
  json_object_set_string_member (object, "string", "hello");
  json_object_set_double_member (object, "double", 3.14159);
  json_object_set_null_member (object, "null");
  json_object_set_int_member (object, "", 0);

  json_object_iter_init (&iter, object);

  while (json_object_iter_next (&iter, &member_name, &member_node))
    verify_foreach (object, member_name, member_node, &fixture);

  g_assert_cmpint (fixture.n_members, ==, json_object_get_size (object));

  json_object_unref (object);
}

static void
test_ordered_iter (void)
{
  JsonObject *object = NULL;
  TestForeachFixture fixture = { 0, TRUE, };
  JsonObjectIter iter;
  const gchar *member_name;
  JsonNode *member_node;

  object = json_object_new ();

  json_object_set_int_member (object, "integer", 42);
  json_object_set_boolean_member (object, "boolean", TRUE);
  json_object_set_string_member (object, "string", "hello");
  json_object_set_double_member (object, "double", 3.14159);
  json_object_set_null_member (object, "null");
  json_object_set_int_member (object, "", 0);

  json_object_iter_init_ordered (&iter, object);

  while (json_object_iter_next_ordered (&iter, &member_name, &member_node))
    verify_foreach (object, member_name, member_node, &fixture);

  g_assert_cmpint (fixture.n_members, ==, json_object_get_size (object));

  json_object_unref (object);
}

static void
test_empty_member (void)
{
  JsonObject *object = json_object_new ();

  json_object_set_string_member (object, "string", "");
  g_assert_true (json_object_has_member (object, "string"));
  g_assert_cmpstr (json_object_get_string_member (object, "string"), ==, "");

  json_object_set_string_member (object, "null", NULL);
  g_assert_true (json_object_has_member (object, "null"));
  g_assert_null (json_object_get_string_member (object, "null"));

  json_object_set_null_member (object, "array");
  g_assert_null (json_object_get_array_member (object, "array"));

  json_object_set_object_member (object, "object", NULL);
  g_assert_nonnull (json_object_get_member (object, "object"));
  g_assert_null (json_object_get_object_member (object, "object"));

  json_object_unref (object);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/object/empty-object", test_empty_object);
  g_test_add_func ("/object/add-member", test_add_member);
  g_test_add_func ("/object/set-member", test_set_member);
  g_test_add_func ("/object/get-member-default", test_get_member_default);
  g_test_add_func ("/object/remove-member", test_remove_member);
  g_test_add_func ("/object/foreach-member", test_foreach_member);
  g_test_add_func ("/object/iter", test_iter);
  g_test_add_func ("/object/ordered-iter", test_ordered_iter);
  g_test_add_func ("/object/empty-member", test_empty_member);

  return g_test_run ();
}
