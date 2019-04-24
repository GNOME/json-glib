/* json-pointer.h - JSON Pointer implementation
 *
 * This file is part of JSON-GLib
 * Copyright © 2011  Intel Corp.
 * Copyright © 2019  Jakub Kaszycki
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author:
 *   Emmanuele Bassi  <ebassi@linux.intel.com>
 *   Jakub Kaszycki   <jakub@kaszycki.net.pl>
 */

/**
 * SECTION:json-pointer
 * @Title: JsonPointer
 * @short_description: JSON Pointer implementation
 *
 * #JsonPointer is a simple class implementing the JSON Pointer syntax for
 * extracting data out of a JSON tree. JSON Pointer is a standarized (RFC 6901)
 * format for addressing particular nodes in a JSON tree.
 *
 * Once a #JsonPointer instance has been created, it has to compile a JSON Pointer
 * expression using json_pointer_compile() before being able to match it to a
 * JSON tree; the same #JsonPointer instance can be used to match multiple JSON
 * trees. It it also possible to compile a new JSON Pointer expression using the
 * same #JsonPointer instance; the previous expression will be discarded only if
 * the compilation of the new expression is successful.
 *
 * The simple convenience function json_pointer_query() can be used for one-off
 * matching.
 *
 * ## Syntax of the JSON Pointer expressions ##
 *
 * A JSON Pointer expression is very simple; it consists of slash-separated
 * strings or numbers. Names match only object members and numbers match array
 * elements or object members. Simple examples are:
 *
 * |[<!-- language="plain" -->
 *   // Empty JSON Pointer string references the root node
 *
 *
 *   // Reference to a node in {"a":{"b":1}}
 *   /a/b
 *
 *   // Reference to a node in {"a":[{"": null}]}
 *   /a/0/
 * ]|
 *
 * More information about JSON Pointer is available in the
 * [RFC 6901](https://tools.ietf.org/html/rfc6901).
 *
 * ## Example of JSON Pointer matches
 * The following example shows some of the results of using #JsonPointer
 * on a JSON tree. We use the following JSON description of a bookstore:
 *
 * |[<!-- language="plain" -->
 *   { "store": {
 *       "book": [
 *         { "category": "reference", "author": "Nigel Rees",
 *           "title": "Sayings of the Century", "price": "8.95"  },
 *         { "category": "fiction", "author": "Evelyn Waugh",
 *           "title": "Sword of Honour", "price": "12.99" },
 *         { "category": "fiction", "author": "Herman Melville",
 *           "title": "Moby Dick", "isbn": "0-553-21311-3",
 *           "price": "8.99" },
 *         { "category": "fiction", "author": "J. R. R. Tolkien",
 *           "title": "The Lord of the Rings", "isbn": "0-395-19395-8",
 *           "price": "22.99" }
 *       ],
 *       "bicycle": { "color": "red", "price": "19.95" }
 *     }
 *   }
 * ]|
 *
 * We can parse the JSON using #JsonParser:
 *
 * |[<!-- language="C" -->
 *   JsonParser *parser = json_parser_new ();
 *   json_parser_load_from_data (parser, json_data, -1, NULL);
 * ]|
 *
 * If we run the following code:
 *
 * |[<!-- language="C" -->
 *   JsonNode *result;
 *   JsonPointer *pointer = json_pointer_new ();
 *   json_pointer_compile (pointer, "/store/book/3/author", NULL);
 *   result = json_pointer_match (pointer, json_parser_get_root (parser));
 * ]|
 *
 * The result #JsonNode will contain a string node containing the string
 * "J. R. R. Tolkien".
 *
 * |[<!-- language="C" -->
 *   JsonGenerator *generator = json_generator_new ();
 *   json_generator_set_root (generator, result);
 *   char *str = json_generator_to_data (generator, NULL);
 *   g_print ("Results: %s\n", str);
 * ]|
 *
 * The output will be:
 *
 * |[<!-- language="plain" -->
 *   "J. R. R. Tolkien"
 * ]|
 *
 * As you may have noted, JSON Pointer matches only single nodes. If you want
 * to match more than one node, use #JsonPath which is a slightly more
 * sophisticated format.
 *
 * #JsonPointer is available since JSON-GLib 1.6
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>

#include "json-pointer.h"

#include "json-debug.h"
#include "json-types-private.h"

typedef struct _PointerNode PointerNode;

struct _JsonPointer
{
  GObject parent_instance;

  /* the compiled pointer */
  GSList *nodes;

  gboolean is_compiled;
};

struct _JsonPointerClass
{
  GObjectClass parent_class;
};

struct _PointerNode
{
  gchar *name;
  gboolean is_number;
  guint number;
};

G_DEFINE_QUARK (json-pointer-error-quark, json_pointer_error)

G_DEFINE_TYPE (JsonPointer, json_pointer, G_TYPE_OBJECT)

static void
pointer_node_free (gpointer data)
{
  if (data != NULL)
    {
      PointerNode *node = data;

      g_free (node->name);

      g_slice_free (PointerNode, node);
    }
}

static void
free_node_list (GSList *list)
{
  g_slist_free_full (list, pointer_node_free);
}

static void
json_pointer_finalize (GObject *gobject)
{
  JsonPointer *self = JSON_POINTER (gobject);

  free_node_list (self->nodes);

  G_OBJECT_CLASS (json_pointer_parent_class)->finalize (gobject);
}

static void
json_pointer_class_init (JsonPointerClass *klass)
{
  G_OBJECT_CLASS (klass)->finalize = json_pointer_finalize;
}

static void
json_pointer_init (JsonPointer *self)
{
  self->nodes = NULL;
  self->is_compiled = FALSE;
}

/**
 * json_pointer_new:
 *
 * Creates a new #JsonPointer instance.
 *
 * Once created, the #JsonPointer object should be used with json_pointer_compile()
 * and json_pointer_match().
 *
 * Return value: (transfer full): the newly created #JsonPointer instance. Use
 *   g_object_unref() to free the allocated resources when done
 *
 * Since: 1.6
 */
JsonPointer *
json_pointer_new (void)
{
  return g_object_new (JSON_TYPE_POINTER, NULL);
}

/**
 * json_pointer_compile:
 * @pointer: a #JsonPointer
 * @expression: a JSON Pointer expression
 * @error: return location for a #GError, or %NULL
 *
 * Validates and decomposes @expression.
 *
 * A JSON Pointer expression must be compiled before calling
 * json_pointer_match().
 *
 * Return value: %TRUE on success; on error, @error will be set with
 *   the %JSON_POINTER_ERROR domain and a code from the #JsonPointerError
 *   enumeration, and %FALSE will be returned
 *
 * Since: 1.6
 */
gboolean
json_pointer_compile (JsonPointer    *pointer,
                      const char     *expression,
                      GError        **error)
{
  GString *buf = g_string_new (NULL);
  gboolean in_escape = FALSE;
  PointerNode *node;
  GSList *nodes = NULL;
  guint64 number;
  const char *p = expression;

  g_return_val_if_fail (JSON_IS_POINTER (pointer), FALSE);
  g_return_val_if_fail (expression != NULL, FALSE);

  if (*p == '\0')
    {
      pointer->nodes = NULL;
      pointer->is_compiled = TRUE;
      return TRUE;
    }

  if (*p != '/')
    {
      g_set_error_literal (error,
                           JSON_POINTER_ERROR,
                           JSON_POINTER_ERROR_SYNTAX_ERROR,
                           "The first character of a non-empty JSON Pointer "
                           "must be a slash");
      return FALSE;
    }

  buf = g_string_new (NULL);
  p ++;

  while (*p)
    {
      if (in_escape)
        {
          switch (*p)
            {
            case '0':
              g_string_append_c (buf, '~');
              break;
            case '1':
              g_string_append_c (buf, '/');
              break;
            default:
              if (g_ascii_isprint (*p))
                g_set_error (error,
                             JSON_POINTER_ERROR,
                             JSON_POINTER_ERROR_SYNTAX_ERROR,
                             "Unknown escape: ~%c", *p);
              else
                g_set_error (error,
                             JSON_POINTER_ERROR,
                             JSON_POINTER_ERROR_SYNTAX_ERROR,
                             "Unknown escape: ~? (0x%02hhx)", *p);
              free_node_list (nodes);
              g_string_free (buf, TRUE);
              return FALSE;
            }

          in_escape = FALSE;
        }
      else
        switch (*p)
          {
          case '~':
            in_escape = TRUE;
            break;
          case '/':
            node = g_slice_new (PointerNode);
            node->name = g_steal_pointer (&buf->str);
            buf->allocated_len = buf->len = 0;
            if (g_ascii_string_to_unsigned (node->name,
                                            10,
                                            0,
                                            G_MAXUINT,
                                            &number,
                                            NULL))
              {
                node->is_number = TRUE;
                node->number = (guint) number;
              }
            else
              node->is_number = FALSE;
            nodes = g_slist_prepend (nodes, node);
            break;
          default:
            g_string_append_c (buf, *p);
          }

      p ++;
    }

  node = g_slice_new (PointerNode);
  node->name = g_string_free (buf, FALSE);
  if (g_ascii_string_to_unsigned (node->name,
                                  10,
                                  0,
                                  G_MAXUINT,
                                  &number,
                                  NULL))
    {
      node->is_number = TRUE;
      node->number = (guint) number;
    }
  else
    node->is_number = FALSE;

  nodes = g_slist_reverse (g_slist_prepend (nodes, node));

  free_node_list (pointer->nodes);
  pointer->nodes = nodes;
  pointer->is_compiled = TRUE;

  return TRUE;
}

/**
 * json_pointer_match:
 * @pointer: a compiled #JsonPointer
 * @root: a #JsonNode
 *
 * Matches the JSON tree pointed by @root using the expression compiled
 * into the #JsonPointer.
 *
 * Return value: (nullable) (transfer full): the node which matched the query
 *     or %NULL if no node matched the query.
 *     Use json_node_unref() when done
 *
 * Since: 1.6
 */
JsonNode *
json_pointer_match (JsonPointer *pointer,
                    JsonNode *root)
{
  GSList *iter;
  JsonNode *node;

  g_return_val_if_fail (JSON_IS_POINTER (pointer), NULL);
  g_return_val_if_fail (pointer->is_compiled, NULL);
  g_return_val_if_fail (root != NULL, NULL);

  node = root;
  iter = pointer->nodes;

  while (iter != NULL)
    {
      PointerNode *pnode = iter->data;

      switch (node->type)
        {
        case JSON_NODE_NULL:
        case JSON_NODE_VALUE:
          return NULL;
        case JSON_NODE_ARRAY:
          if (pnode->is_number)
            {
              JsonArray *array = json_node_get_array (node);

              if (pnode->number >= array->elements->len)
                return NULL;

              node = g_ptr_array_index (array->elements, pnode->number);
            }
          else
            return NULL;
          break;
        case JSON_NODE_OBJECT:
          node = json_object_get_member (json_node_get_object (node),
                                         pnode->name);
          break;
        }

      if (node == NULL)
        return NULL;

      iter = iter->next;
    }

  return json_node_ref (node);
}

/**
 * json_pointer_query:
 * @expression: a JSON Pointer expression
 * @root: the root of a JSON tree
 * @error: return location for a #GError, or %NULL
 *
 * Queries a JSON tree using a JSON Pointer expression.
 *
 * This function is a simple wrapper around json_pointer_new(),
 * json_pointer_compile() and json_pointer_match(). It implicitly
 * creates a #JsonPointer instance, compiles @expression and
 * matches it against the JSON tree pointed by @root.
 *
 * Return value: (nullable) (transfer full): the node which matched the query
 *     or %NULL if no node matched the query.
 *     Use json_node_unref() when done
 *
 * Since: 1.6
 */
JsonNode *
json_pointer_query (const char  *expression,
                    JsonNode    *root,
                    GError     **error)
{
  JsonPointer *pointer = json_pointer_new ();
  JsonNode *retval;

  if (!json_pointer_compile (pointer, expression, error))
    {
      g_object_unref (pointer);
      return NULL;
    }

  retval = json_pointer_match (pointer, root);

  g_object_unref (pointer);

  return retval;
}
