#include "json-test-utils.h"

static void
test_empty_array (void)
{
  JsonArray *array = json_array_new ();

  g_assert_cmpint (json_array_get_length (array), ==, 0);
  g_assert_null (json_array_get_elements (array));

  json_array_unref (array);
}

static void
test_add_element (void)
{
  JsonArray *array = json_array_new ();
  JsonNode *node = json_node_new (JSON_NODE_NULL);

  g_assert_cmpint (json_array_get_length (array), ==, 0);

  json_array_add_element (array, node);
  g_assert_cmpint (json_array_get_length (array), ==, 1);

  node = json_array_get_element (array, 0);
  g_assert_cmpint (JSON_NODE_TYPE (node), ==, JSON_NODE_NULL);
  g_assert_true (json_array_get_null_element (array, 0));

  json_array_add_int_element (array, 42);
  g_assert_cmpint (json_array_get_length (array), ==, 2);
  g_assert_cmpint (json_array_get_int_element (array, 1), ==, 42);

  json_array_add_double_element (array, 3.14);
  g_assert_cmpint (json_array_get_length (array), ==, 3);
  json_assert_fuzzy_equals (json_array_get_double_element (array, 2), 3.14, 0.001);

  json_array_add_boolean_element (array, TRUE);
  g_assert_cmpint (json_array_get_length (array), ==, 4);
  g_assert_true (json_array_get_boolean_element (array, 3));

  json_array_add_string_element (array, "Hello");
  g_assert_cmpint (json_array_get_length (array), ==, 5);
  g_assert_cmpstr (json_array_get_string_element (array, 4), ==, "Hello");

  json_array_add_string_element (array, NULL);
  g_assert_cmpint (json_array_get_length (array), ==, 6);
  g_assert_null (json_array_get_string_element (array, 5));
  g_assert_nonnull (json_array_get_element (array, 5));
  g_assert_true (json_array_get_null_element (array, 5));

  json_array_add_array_element (array, NULL);
  g_assert_null (json_array_get_array_element (array, 6));
  g_assert_true (json_array_get_null_element (array, 6));

  json_array_add_object_element (array, json_object_new ());
  g_assert_nonnull (json_array_get_object_element (array, 7));

  json_array_add_object_element (array, NULL);
  g_assert_null (json_array_get_object_element (array, 8));
  g_assert_true (json_array_get_null_element (array, 8));

  json_array_unref (array);
}

static void
test_remove_element (void)
{
  JsonArray *array = json_array_new ();
  JsonNode *node = json_node_new (JSON_NODE_NULL);

  json_array_add_element (array, node);

  json_array_remove_element (array, 0);
  g_assert_cmpint (json_array_get_length (array), ==, 0);

  json_array_unref (array);
}

typedef struct _TestForeachFixture
{
  GList *elements;
  gint n_elements;
  gint iterations;
} TestForeachFixture;

static const struct {
  JsonNodeType element_type;
  GType element_gtype;
} type_verify[] = {
  { JSON_NODE_VALUE, G_TYPE_INT64   },
  { JSON_NODE_VALUE, G_TYPE_BOOLEAN },
  { JSON_NODE_VALUE, G_TYPE_STRING  },
  { JSON_NODE_NULL,  G_TYPE_INVALID }
};

static void
verify_foreach (JsonArray *array G_GNUC_UNUSED,
                guint      index_,
                JsonNode  *element_node,
                gpointer   user_data)
{
  TestForeachFixture *fixture = user_data;

  g_assert_nonnull (g_list_find (fixture->elements, element_node));
  g_assert_true (json_node_get_node_type (element_node) == type_verify[index_].element_type);
  g_assert_true (json_node_get_value_type (element_node) == type_verify[index_].element_gtype);

  fixture->iterations += 1;
}

static void
test_foreach_element (void)
{
  JsonArray *array = json_array_new ();
  TestForeachFixture fixture = { 0, };

  json_array_add_int_element (array, 42);
  json_array_add_boolean_element (array, TRUE);
  json_array_add_string_element (array, "hello");
  json_array_add_null_element (array);

  fixture.elements = json_array_get_elements (array);
  g_assert_nonnull (fixture.elements);

  fixture.n_elements = json_array_get_length (array);
  g_assert_cmpint (fixture.n_elements, ==, g_list_length (fixture.elements));

  fixture.iterations = 0;

  json_array_foreach_element (array, verify_foreach, &fixture);

  g_assert_cmpint (fixture.iterations, ==, fixture.n_elements);

  g_list_free (fixture.elements);
  json_array_unref (array);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/array/empty-array", test_empty_array);
  g_test_add_func ("/array/add-element", test_add_element);
  g_test_add_func ("/array/remove-element", test_remove_element);
  g_test_add_func ("/array/foreach-element", test_foreach_element);

  return g_test_run ();
}
