/* json-parser.c - JSON streams parser
 * 
 * This file is part of JSON-GLib
 *
 * Copyright © 2007, 2008, 2009 OpenedHand Ltd
 * Copyright © 2009, 2010 Intel Corp.
 * Copyright © 2015 Collabora Ltd.
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
 *   Philip Withnall  <philip.withnall@collabora.co.uk>
 */

/**
 * SECTION:json-parser
 * @short_description: Parse JSON data streams
 *
 * #JsonParser provides an object for parsing a JSON data stream, either
 * inside a file or inside a static buffer.
 */

#include "config.h"

#include <string.h>

#include <glib/gi18n-lib.h>

#include "json-types-private.h"

#include "json-debug.h"
#include "json-parser.h"
#include "json-scanner.h"

struct _JsonParserPrivate
{
  JsonNode *root;
  JsonNode *current_node;

  JsonScanner *scanner;

  JsonParserError error_code;
  GError *last_error;

  gchar *variable_name;
  gchar *filename;

  guint has_assignment : 1;
  guint is_filename    : 1;
  guint is_immutable   : 1;
};

static const gchar symbol_names[] =
  "true\0"
  "false\0"
  "null\0"
  "var\0";

static const struct
{
  guint name_offset;
  guint token;
} symbols[] = {
  {  0, JSON_TOKEN_TRUE  },
  {  5, JSON_TOKEN_FALSE },
  { 11, JSON_TOKEN_NULL  },
  { 16, JSON_TOKEN_VAR   }
};

static const guint n_symbols = G_N_ELEMENTS (symbols);

enum
{
  PARSE_START,
  OBJECT_START,
  OBJECT_MEMBER,
  OBJECT_END,
  ARRAY_START,
  ARRAY_ELEMENT,
  ARRAY_END,
  PARSE_END,
  ERROR,

  LAST_SIGNAL
};

static guint parser_signals[LAST_SIGNAL] = { 0, };

enum
{
  PROP_IMMUTABLE = 1,
  PROP_LAST
};

static GParamSpec *parser_props[PROP_LAST] = { NULL, };

G_DEFINE_QUARK (json-parser-error-quark, json_parser_error)

G_DEFINE_TYPE_WITH_PRIVATE (JsonParser, json_parser, G_TYPE_OBJECT)

static guint json_parse_array  (JsonParser   *parser,
                                JsonScanner  *scanner,
                                JsonNode    **node);
static guint json_parse_object (JsonParser   *parser,
                                JsonScanner  *scanner,
                                JsonNode    **node);

static inline void
json_parser_clear (JsonParser *parser)
{
  JsonParserPrivate *priv = parser->priv;

  g_free (priv->variable_name);
  priv->variable_name = NULL;

  if (priv->last_error)
    {
      g_error_free (priv->last_error);
      priv->last_error = NULL;
    }

  if (priv->root)
    {
      json_node_unref (priv->root);
      priv->root = NULL;
    }
}

static void
json_parser_dispose (GObject *gobject)
{
  json_parser_clear (JSON_PARSER (gobject));

  G_OBJECT_CLASS (json_parser_parent_class)->dispose (gobject);
}

static void
json_parser_finalize (GObject *gobject)
{
  JsonParserPrivate *priv = JSON_PARSER (gobject)->priv;

  g_free (priv->variable_name);
  g_free (priv->filename);

  G_OBJECT_CLASS (json_parser_parent_class)->finalize (gobject);
}

static void
json_parser_set_property (GObject      *gobject,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  JsonParserPrivate *priv = JSON_PARSER (gobject)->priv;

  switch (prop_id)
    {
    case PROP_IMMUTABLE:
      /* Construct-only. */
      priv->is_immutable = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
json_parser_get_property (GObject    *gobject,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  JsonParserPrivate *priv = JSON_PARSER (gobject)->priv;

  switch (prop_id)
    {
    case PROP_IMMUTABLE:
      g_value_set_boolean (value, priv->is_immutable);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
      break;
    }
}

static void
json_parser_class_init (JsonParserClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = json_parser_set_property;
  gobject_class->get_property = json_parser_get_property;
  gobject_class->dispose = json_parser_dispose;
  gobject_class->finalize = json_parser_finalize;

  /**
   * JsonParser:immutable:
   *
   * Whether the #JsonNode tree built by the #JsonParser should be immutable
   * when created. Making the output immutable on creation avoids the expense
   * of traversing it to make it immutable later.
   *
   * Since: 1.2
   */
  parser_props[PROP_IMMUTABLE] =
    g_param_spec_boolean ("immutable",
                          "Immutable Output",
                          "Whether the parser output is immutable.",
                          FALSE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, PROP_LAST, parser_props);

  /**
   * JsonParser::parse-start:
   * @parser: the #JsonParser that received the signal
   * 
   * The ::parse-start signal is emitted when the parser began parsing
   * a JSON data stream.
   */
  parser_signals[PARSE_START] =
    g_signal_new ("parse-start",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, parse_start),
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
  /**
   * JsonParser::parse-end:
   * @parser: the #JsonParser that received the signal
   *
   * The ::parse-end signal is emitted when the parser successfully
   * finished parsing a JSON data stream
   */
  parser_signals[PARSE_END] =
    g_signal_new ("parse-end",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, parse_end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  /**
   * JsonParser::object-start:
   * @parser: the #JsonParser that received the signal
   *
   * The ::object-start signal is emitted each time the #JsonParser
   * starts parsing a #JsonObject.
   */
  parser_signals[OBJECT_START] =
    g_signal_new ("object-start",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, object_start),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  /**
   * JsonParser::object-member:
   * @parser: the #JsonParser that received the signal
   * @object: a #JsonObject
   * @member_name: the name of the newly parsed member
   *
   * The ::object-member signal is emitted each time the #JsonParser
   * has successfully parsed a single member of a #JsonObject. The
   * object and member are passed to the signal handlers.
   */
  parser_signals[OBJECT_MEMBER] =
    g_signal_new ("object-member",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, object_member),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  JSON_TYPE_OBJECT,
                  G_TYPE_STRING);
  /**
   * JsonParser::object-end:
   * @parser: the #JsonParser that received the signal
   * @object: the parsed #JsonObject
   *
   * The ::object-end signal is emitted each time the #JsonParser
   * has successfully parsed an entire #JsonObject.
   */
  parser_signals[OBJECT_END] =
    g_signal_new ("object-end",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, object_end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  JSON_TYPE_OBJECT);
  /**
   * JsonParser::array-start:
   * @parser: the #JsonParser that received the signal
   *
   * The ::array-start signal is emitted each time the #JsonParser
   * starts parsing a #JsonArray
   */
  parser_signals[ARRAY_START] =
    g_signal_new ("array-start",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, array_start),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  /**
   * JsonParser::array-element:
   * @parser: the #JsonParser that received the signal
   * @array: a #JsonArray
   * @index_: the index of the newly parsed element
   *
   * The ::array-element signal is emitted each time the #JsonParser
   * has successfully parsed a single element of a #JsonArray. The
   * array and element index are passed to the signal handlers.
   */
  parser_signals[ARRAY_ELEMENT] =
    g_signal_new ("array-element",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, array_element),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  JSON_TYPE_ARRAY,
                  G_TYPE_INT);
  /**
   * JsonParser::array-end:
   * @parser: the #JsonParser that received the signal
   * @array: the parsed #JsonArray
   *
   * The ::array-end signal is emitted each time the #JsonParser
   * has successfully parsed an entire #JsonArray
   */
  parser_signals[ARRAY_END] =
    g_signal_new ("array-end",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, array_end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  JSON_TYPE_ARRAY);
  /**
   * JsonParser::error:
   * @parser: the parser instance that received the signal
   * @error: a pointer to the #GError
   *
   * The ::error signal is emitted each time a #JsonParser encounters
   * an error in a JSON stream.
   */
  parser_signals[ERROR] =
    g_signal_new ("error",
                  G_OBJECT_CLASS_TYPE (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (JsonParserClass, error),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_POINTER);
}

static void
json_parser_init (JsonParser *parser)
{
  JsonParserPrivate *priv = json_parser_get_instance_private (parser);

  parser->priv = priv;

  priv->root = NULL;
  priv->current_node = NULL;

  priv->error_code = JSON_PARSER_ERROR_PARSE;
  priv->last_error = NULL;

  priv->has_assignment = FALSE;
  priv->variable_name = NULL;

  priv->is_filename = FALSE;
  priv->filename = FALSE;
}

static guint
json_parse_value (JsonParser   *parser,
                  JsonScanner  *scanner,
                  guint         token,
                  JsonNode    **node)
{
  JsonParserPrivate *priv = parser->priv;
  JsonNode *current_node = priv->current_node;
  gboolean is_negative = FALSE;

  if (token == '-')
    {
      guint next_token = json_scanner_peek_next_token (scanner);

      if (next_token == G_TOKEN_INT ||
          next_token == G_TOKEN_FLOAT)
        {
           is_negative = TRUE;
           token = json_scanner_get_next_token (scanner);
        }
      else
        return G_TOKEN_INT;
    }

  switch (token)
    {
    case G_TOKEN_INT:
      JSON_NOTE (PARSER, "abs(node): %" G_GINT64_FORMAT " (sign: %s)",
                 scanner->value.v_int64,
                 is_negative ? "negative" : "positive");
      *node = json_node_init_int (json_node_alloc (),
                                  is_negative ? scanner->value.v_int64 * -1
                                              : scanner->value.v_int64);
      break;

    case G_TOKEN_FLOAT:
      JSON_NOTE (PARSER, "abs(node): %.6f (sign: %s)",
                 scanner->value.v_float,
                 is_negative ? "negative" : "positive");
      *node = json_node_init_double (json_node_alloc (),
                                     is_negative ? scanner->value.v_float * -1.0
                                                 : scanner->value.v_float);
      break;

    case G_TOKEN_STRING:
      JSON_NOTE (PARSER, "node: '%s'",
                 scanner->value.v_string);
      *node = json_node_init_string (json_node_alloc (), scanner->value.v_string);
      break;

    case JSON_TOKEN_TRUE:
    case JSON_TOKEN_FALSE:
      JSON_NOTE (PARSER, "node: '%s'",
                 JSON_TOKEN_TRUE ? "<true>" : "<false>");
      *node = json_node_init_boolean (json_node_alloc (), token == JSON_TOKEN_TRUE ? TRUE : FALSE);
      break;

    case JSON_TOKEN_NULL:
      JSON_NOTE (PARSER, "node: <null>");
      *node = json_node_init_null (json_node_alloc ());
      break;

    case G_TOKEN_IDENTIFIER:
      JSON_NOTE (PARSER, "node: identifier '%s'", scanner->value.v_identifier);
      priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;
      *node = NULL;
      return G_TOKEN_SYMBOL;

    default:
      {
        JsonNodeType cur_type;

        *node = NULL;

        JSON_NOTE (PARSER, "node: invalid token");

        cur_type = json_node_get_node_type (current_node);
        if (cur_type == JSON_NODE_ARRAY)
          {
            priv->error_code = JSON_PARSER_ERROR_PARSE;
            return G_TOKEN_RIGHT_BRACE;
          }
        else if (cur_type == JSON_NODE_OBJECT)
          {
            priv->error_code = JSON_PARSER_ERROR_PARSE;
            return G_TOKEN_RIGHT_CURLY;
          }
        else
          {
            priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;
            return G_TOKEN_SYMBOL;
          }
      }
      break;
    }

  if (priv->is_immutable && *node != NULL)
    json_node_seal (*node);

  return G_TOKEN_NONE;
}

static guint
json_parse_array (JsonParser   *parser,
                  JsonScanner  *scanner,
                  JsonNode    **node)
{
  JsonParserPrivate *priv = parser->priv;
  JsonNode *old_current;
  JsonArray *array;
  guint token;
  gint idx;

  old_current = priv->current_node;
  priv->current_node = json_node_init_array (json_node_alloc (), NULL);

  array = json_array_new ();

  token = json_scanner_get_next_token (scanner);
  g_assert (token == G_TOKEN_LEFT_BRACE);

  g_signal_emit (parser, parser_signals[ARRAY_START], 0);

  idx = 0;
  while (token != G_TOKEN_RIGHT_BRACE)
    {
      guint next_token = json_scanner_peek_next_token (scanner);
      JsonNode *element = NULL;

      /* parse the element */
      switch (next_token)
        {
        case G_TOKEN_LEFT_BRACE:
          JSON_NOTE (PARSER, "Nested array at index %d", idx);
          token = json_parse_array (parser, scanner, &element);
          break;

        case G_TOKEN_LEFT_CURLY:
          JSON_NOTE (PARSER, "Nested object at index %d", idx);
          token = json_parse_object (parser, scanner, &element);
          break;

        case G_TOKEN_RIGHT_BRACE:
          goto array_done;

        default:
          token = json_scanner_get_next_token (scanner);
          token = json_parse_value (parser, scanner, token, &element);
          break;
        }

      if (token != G_TOKEN_NONE || element == NULL)
        {
          /* the json_parse_* functions will have set the error code */
          json_array_unref (array);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return token;
        }

      next_token = json_scanner_peek_next_token (scanner);

      /* look for missing commas */
      if (next_token != G_TOKEN_COMMA && next_token != G_TOKEN_RIGHT_BRACE)
        {
          priv->error_code = JSON_PARSER_ERROR_MISSING_COMMA;

          json_array_unref (array);
          json_node_free (priv->current_node);
          json_node_free (element);
          priv->current_node = old_current;

          return G_TOKEN_COMMA;
        }

      /* look for trailing commas */
      if (next_token == G_TOKEN_COMMA)
        {
          token = json_scanner_get_next_token (scanner);
          next_token = json_scanner_peek_next_token (scanner);

          if (next_token == G_TOKEN_RIGHT_BRACE)
            {
              priv->error_code = JSON_PARSER_ERROR_TRAILING_COMMA;

              json_array_unref (array);
              json_node_unref (priv->current_node);
              json_node_unref (element);
              priv->current_node = old_current;

              return G_TOKEN_RIGHT_BRACE;
            }
        }

      JSON_NOTE (PARSER, "Array element %d completed", idx);
      json_node_set_parent (element, priv->current_node);
      if (priv->is_immutable)
        json_node_seal (element);
      json_array_add_element (array, element);

      g_signal_emit (parser, parser_signals[ARRAY_ELEMENT], 0,
                     array,
                     idx);

      idx += 1;
      token = next_token;
    }

array_done:
  json_scanner_get_next_token (scanner);

  if (priv->is_immutable)
    json_array_seal (array);

  json_node_take_array (priv->current_node, array);
  if (priv->is_immutable)
    json_node_seal (priv->current_node);
  json_node_set_parent (priv->current_node, old_current);

  g_signal_emit (parser, parser_signals[ARRAY_END], 0, array);

  if (node != NULL && *node == NULL)
    *node = priv->current_node;

  priv->current_node = old_current;

  return G_TOKEN_NONE;
}

static guint
json_parse_object (JsonParser   *parser,
                   JsonScanner  *scanner,
                   JsonNode    **node)
{
  JsonParserPrivate *priv = parser->priv;
  JsonObject *object;
  JsonNode *old_current;
  guint token;

  old_current = priv->current_node;
  priv->current_node = json_node_init_object (json_node_alloc (), NULL);

  object = json_object_new ();

  token = json_scanner_get_next_token (scanner);
  g_assert (token == G_TOKEN_LEFT_CURLY);

  g_signal_emit (parser, parser_signals[OBJECT_START], 0);

  while (token != G_TOKEN_RIGHT_CURLY)
    {
      guint next_token = json_scanner_peek_next_token (scanner);
      JsonNode *member = NULL;
      gchar *name;

      /* we need to abort here because empty objects do not
       * have member names
       */
      if (next_token == G_TOKEN_RIGHT_CURLY)
        break;

      /* parse the member's name */
      if (next_token != G_TOKEN_STRING)
        {
          JSON_NOTE (PARSER, "Missing object member name");

          priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;

          json_object_unref (object);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return G_TOKEN_STRING;
        }

      /* member name */
      token = json_scanner_get_next_token (scanner);
      name = g_strdup (scanner->value.v_string);
      if (name == NULL)
        {
          JSON_NOTE (PARSER, "Empty object member name");

          priv->error_code = JSON_PARSER_ERROR_EMPTY_MEMBER_NAME;

          json_object_unref (object);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return G_TOKEN_STRING;
        }

      JSON_NOTE (PARSER, "Object member '%s'", name);

      /* a colon separates names from values */
      next_token = json_scanner_peek_next_token (scanner);
      if (next_token != ':')
        {
          JSON_NOTE (PARSER, "Missing object member name separator");

          priv->error_code = JSON_PARSER_ERROR_MISSING_COLON;

          g_free (name);
          json_object_unref (object);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return ':';
        }

      /* we swallow the ':' */
      token = json_scanner_get_next_token (scanner);
      g_assert (token == ':');
      next_token = json_scanner_peek_next_token (scanner);

      /* parse the member's value */
      switch (next_token)
        {
        case G_TOKEN_LEFT_BRACE:
          JSON_NOTE (PARSER, "Nested array at member %s", name);
          token = json_parse_array (parser, scanner, &member);
          break;

        case G_TOKEN_LEFT_CURLY:
          JSON_NOTE (PARSER, "Nested object at member %s", name);
          token = json_parse_object (parser, scanner, &member);
          break;

        default:
          /* once a member name is defined we need a value */
          token = json_scanner_get_next_token (scanner);
          token = json_parse_value (parser, scanner, token, &member);
          break;
        }

      if (token != G_TOKEN_NONE || member == NULL)
        {
          /* the json_parse_* functions will have set the error code */
          g_free (name);
          json_object_unref (object);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return token;
        }

      next_token = json_scanner_peek_next_token (scanner);
      if (next_token == G_TOKEN_COMMA)
        {
          token = json_scanner_get_next_token (scanner);
          next_token = json_scanner_peek_next_token (scanner);

          /* look for trailing commas */
          if (next_token == G_TOKEN_RIGHT_CURLY)
            {
              priv->error_code = JSON_PARSER_ERROR_TRAILING_COMMA;

              g_free (name);
              json_object_unref (object);
              json_node_unref (member);
              json_node_unref (priv->current_node);
              priv->current_node = old_current;

              return G_TOKEN_RIGHT_BRACE;
            }
        }
      else if (next_token == G_TOKEN_STRING)
        {
          priv->error_code = JSON_PARSER_ERROR_MISSING_COMMA;

          g_free (name);
          json_object_unref (object);
          json_node_unref (member);
          json_node_unref (priv->current_node);
          priv->current_node = old_current;

          return G_TOKEN_COMMA;
        }

      JSON_NOTE (PARSER, "Object member '%s' completed", name);
      json_node_set_parent (member, priv->current_node);
      if (priv->is_immutable)
        json_node_seal (member);
      json_object_set_member (object, name, member);

      g_signal_emit (parser, parser_signals[OBJECT_MEMBER], 0,
                     object,
                     name);

      g_free (name);

      token = next_token;
    }

  json_scanner_get_next_token (scanner);

  if (priv->is_immutable)
    json_object_seal (object);

  json_node_take_object (priv->current_node, object);
  if (priv->is_immutable)
    json_node_seal (priv->current_node);
  json_node_set_parent (priv->current_node, old_current);

  g_signal_emit (parser, parser_signals[OBJECT_END], 0, object);

  if (node != NULL && *node == NULL)
    *node = priv->current_node;

  priv->current_node = old_current;

  return G_TOKEN_NONE;
}

static guint
json_parse_statement (JsonParser  *parser,
                      JsonScanner *scanner)
{
  JsonParserPrivate *priv = parser->priv;
  guint token;

  token = json_scanner_peek_next_token (scanner);
  switch (token)
    {
    case G_TOKEN_LEFT_CURLY:
      JSON_NOTE (PARSER, "Statement is object declaration");
      return json_parse_object (parser, scanner, &priv->root);

    case G_TOKEN_LEFT_BRACE:
      JSON_NOTE (PARSER, "Statement is array declaration");
      return json_parse_array (parser, scanner, &priv->root);

    /* some web APIs are not only passing the data structures: they are
     * also passing an assigment, which makes parsing horribly complicated
     * only because web developers are lazy, and writing "var foo = " is
     * evidently too much to request from them.
     */
    case JSON_TOKEN_VAR:
      {
        guint next_token;
        gchar *name;

        JSON_NOTE (PARSER, "Statement is an assignment");

        /* swallow the 'var' token... */
        token = json_scanner_get_next_token (scanner);

        /* ... swallow the variable name... */
        next_token = json_scanner_get_next_token (scanner);
        if (next_token != G_TOKEN_IDENTIFIER)
          {
            priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;
            return G_TOKEN_IDENTIFIER;
          }

        name = g_strdup (scanner->value.v_identifier);

        /* ... and finally swallow the '=' */
        next_token = json_scanner_get_next_token (scanner);
        if (next_token != '=')
          {
            priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;
            g_free (name);
            return '=';
          }

        priv->has_assignment = TRUE;
        priv->variable_name = name;

        token = json_parse_statement (parser, scanner);

        /* remove the trailing semi-colon */
        next_token = json_scanner_peek_next_token (scanner);
        if (next_token == ';')
          {
            token = json_scanner_get_next_token (scanner);
            return G_TOKEN_NONE;
          }

        return token;
      }
      break;

    case JSON_TOKEN_NULL:
    case JSON_TOKEN_TRUE:
    case JSON_TOKEN_FALSE:
    case '-':
    case G_TOKEN_INT:
    case G_TOKEN_FLOAT:
    case G_TOKEN_STRING:
    case G_TOKEN_IDENTIFIER:
      JSON_NOTE (PARSER, "Statement is a value");
      token = json_scanner_get_next_token (scanner);
      return json_parse_value (parser, scanner, token, &priv->root);

    default:
      JSON_NOTE (PARSER, "Unknown statement");
      json_scanner_get_next_token (scanner);
      priv->error_code = JSON_PARSER_ERROR_INVALID_BAREWORD;
      return G_TOKEN_SYMBOL;
    }
}

static void
json_scanner_msg_handler (JsonScanner *scanner,
                          gchar       *message)
{
  JsonParser *parser = scanner->user_data;
  JsonParserPrivate *priv = parser->priv;
  GError *error = NULL;

  g_set_error (&error, JSON_PARSER_ERROR,
               priv->error_code,
               /* translators: %s: is the file name, the first %d is the line
                * number, the second %d is the position on the line, and %s is
                * the error message
                */
               _("%s:%d:%d: Parse error: %s"),
               priv->is_filename ? priv->filename : "<data>",
               scanner->line,
               scanner->position,
               message);
      
  parser->priv->last_error = error;
  g_signal_emit (parser, parser_signals[ERROR], 0, error);
}

static JsonScanner *
json_scanner_create (JsonParser *parser)
{
  JsonScanner *scanner;
  gint i;

  scanner = json_scanner_new ();
  scanner->msg_handler = json_scanner_msg_handler;
  scanner->user_data = parser;

  /* XXX: this is eminently stupid, but we use the symbols later on, so
   * we cannot move them into JsonScanner without moving a bunch of code
   * as well
   */
  for (i = 0; i < n_symbols; i++)
    {
      json_scanner_scope_add_symbol (scanner, 0,
                                     symbol_names + symbols[i].name_offset,
                                     GINT_TO_POINTER (symbols[i].token));
    }

  return scanner;
}

/**
 * json_parser_new:
 * 
 * Creates a new #JsonParser instance. You can use the #JsonParser to
 * load a JSON stream from either a file or a buffer and then walk the
 * hierarchy using the data types API.
 *
 * Return value: the newly created #JsonParser. Use g_object_unref()
 *   to release all the memory it allocates.
 */
JsonParser *
json_parser_new (void)
{
  return g_object_new (JSON_TYPE_PARSER, NULL);
}

/**
 * json_parser_new_immutable:
 *
 * Creates a new #JsonParser instance with its #JsonParser:immutable property
 * set to %TRUE to create immutable output trees.
 *
 * Since: 1.2
 * Returns: (transfer full): a new #JsonParser
 */
JsonParser *
json_parser_new_immutable (void)
{
  return g_object_new (JSON_TYPE_PARSER, "immutable", TRUE, NULL);
}

static gboolean
json_parser_load (JsonParser   *parser,
                  const gchar  *data,
                  gsize         length,
                  GError      **error)
{
  JsonParserPrivate *priv = parser->priv;
  JsonScanner *scanner;
  gboolean done;
  gboolean retval = TRUE;
  gint i;

  json_parser_clear (parser);

  if (!g_utf8_validate (data, length, NULL))
    {
      g_set_error_literal (error, JSON_PARSER_ERROR,
                           JSON_PARSER_ERROR_INVALID_DATA,
                           _("JSON data must be UTF-8 encoded"));
      g_signal_emit (parser, parser_signals[ERROR], 0, *error);
      return FALSE;
    }

  scanner = json_scanner_create (parser);
  json_scanner_input_text (scanner, data, length);

  priv->scanner = scanner;

  g_signal_emit (parser, parser_signals[PARSE_START], 0);

  done = FALSE;
  while (!done)
    {
      if (json_scanner_peek_next_token (scanner) == G_TOKEN_EOF)
        done = TRUE;
      else
        {
          guint expected_token;
          gint cur_token;

          /* we try to show the expected token, if possible */
          expected_token = json_parse_statement (parser, scanner);
          if (expected_token != G_TOKEN_NONE)
            {
              const gchar *symbol_name;
              gchar *msg;

              cur_token = scanner->token;
              msg = NULL;
              symbol_name = NULL;

              if (scanner->scope_id == 0)
                {
                  if (expected_token > JSON_TOKEN_INVALID &&
                      expected_token < JSON_TOKEN_LAST)
                    {
                      for (i = 0; i < n_symbols; i++)
                        if (symbols[i].token == expected_token)
                          symbol_name = symbol_names + symbols[i].name_offset;

                      if (!msg)
                        msg = g_strconcat ("e.g. '", symbol_name, "'", NULL);
                    }

                  if (cur_token > JSON_TOKEN_INVALID &&
                      cur_token < JSON_TOKEN_LAST)
                    {
                      symbol_name = "???";

                      for (i = 0; i < n_symbols; i++)
                        if (symbols[i].token == cur_token)
                          symbol_name = symbol_names + symbols[i].name_offset;
                    }
                }

              /* this will emit the ::error signal via the custom
               * message handler we install
               */
              json_scanner_unexp_token (scanner, expected_token,
                                        NULL, "value",
                                        symbol_name, msg);

              /* and this will propagate the error we create in the
               * same message handler
               */
              if (priv->last_error)
                {
                  g_propagate_error (error, priv->last_error);
                  priv->last_error = NULL;
                }

              retval = FALSE;

              g_free (msg);
              done = TRUE;
            }
        }
    }

  g_signal_emit (parser, parser_signals[PARSE_END], 0);

  /* remove the scanner */
  json_scanner_destroy (scanner);
  priv->scanner = NULL;
  priv->current_node = NULL;

  return retval;
}

/**
 * json_parser_load_from_file:
 * @parser: a #JsonParser
 * @filename: the path for the file to parse
 * @error: return location for a #GError, or %NULL
 *
 * Loads a JSON stream from the content of @filename and parses it. See
 * json_parser_load_from_data().
 *
 * Return value: %TRUE if the file was successfully loaded and parsed.
 *   In case of error, @error is set accordingly and %FALSE is returned
 */
gboolean
json_parser_load_from_file (JsonParser   *parser,
                            const gchar  *filename,
                            GError      **error)
{
  JsonParserPrivate *priv;
  GError *internal_error;
  gchar *data;
  gsize length;
  gboolean retval = TRUE;

  g_return_val_if_fail (JSON_IS_PARSER (parser), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);

  priv = parser->priv;

  internal_error = NULL;
  if (!g_file_get_contents (filename, &data, &length, &internal_error))
    {
      g_propagate_error (error, internal_error);
      return FALSE;
    }

  g_free (priv->filename);

  priv->is_filename = TRUE;
  priv->filename = g_strdup (filename);

  if (!json_parser_load (parser, data, length, &internal_error))
    {
      g_propagate_error (error, internal_error);
      retval = FALSE;
    }

  g_free (data);

  return retval;
}

/**
 * json_parser_load_from_data:
 * @parser: a #JsonParser
 * @data: the buffer to parse
 * @length: the length of the buffer, or -1
 * @error: return location for a #GError, or %NULL
 *
 * Loads a JSON stream from a buffer and parses it. You can call this function
 * multiple times with the same #JsonParser object, but the contents of the
 * parser will be destroyed each time.
 *
 * Return value: %TRUE if the buffer was succesfully parser. In case
 *   of error, @error is set accordingly and %FALSE is returned
 */
gboolean
json_parser_load_from_data (JsonParser   *parser,
                            const gchar  *data,
                            gssize        length,
                            GError      **error)
{
  JsonParserPrivate *priv;
  GError *internal_error;
  gboolean retval = TRUE;

  g_return_val_if_fail (JSON_IS_PARSER (parser), FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  priv = parser->priv;

  if (length < 0)
    length = strlen (data);

  priv->is_filename = FALSE;
  g_free (priv->filename);
  priv->filename = NULL;

  internal_error = NULL;
  if (!json_parser_load (parser, data, length, &internal_error))
    {
      g_propagate_error (error, internal_error);
      retval = FALSE;
    }

  return retval;
}

/**
 * json_parser_get_root:
 * @parser: a #JsonParser
 *
 * Retrieves the top level node from the parsed JSON stream.
 *
 * Return value: (transfer none): the root #JsonNode . The returned
 *   node is owned by the #JsonParser and should never be modified
 *   or freed.
 */
JsonNode *
json_parser_get_root (JsonParser *parser)
{
  g_return_val_if_fail (JSON_IS_PARSER (parser), NULL);

  /* Sanity check. */
  g_return_val_if_fail (!parser->priv->is_immutable ||
                        json_node_is_immutable (parser->priv->root), NULL);

  return parser->priv->root;
}

/**
 * json_parser_steal_root:
 * @parser: a #JsonParser
 *
 * Steals the top level node from the parsed JSON stream.
 *
 * Returns: (transfer full): the top level #JsonNode
 *
 * Since: 1.4
 */
JsonNode *
json_parser_steal_root (JsonParser *parser)
{
  JsonParserPrivate *priv = json_parser_get_instance_private (parser);

  g_return_val_if_fail (JSON_IS_PARSER (parser), NULL);

  return g_steal_pointer (&priv->root);
}

/**
 * json_parser_get_current_line:
 * @parser: a #JsonParser
 *
 * Retrieves the line currently parsed, starting from 1.
 *
 * This function has defined behaviour only while parsing; calling this
 * function from outside the signal handlers emitted by #JsonParser will
 * yield 0.
 *
 * Return value: the currently parsed line, or 0.
 */
guint
json_parser_get_current_line (JsonParser *parser)
{
  g_return_val_if_fail (JSON_IS_PARSER (parser), 0);

  if (parser->priv->scanner != NULL)
    return parser->priv->scanner->line;

  return 0;
}

/**
 * json_parser_get_current_pos:
 * @parser: a #JsonParser
 *
 * Retrieves the current position inside the current line, starting
 * from 0.
 *
 * This function has defined behaviour only while parsing; calling this
 * function from outside the signal handlers emitted by #JsonParser will
 * yield 0.
 *
 * Return value: the position in the current line, or 0.
 */
guint
json_parser_get_current_pos (JsonParser *parser)
{
  g_return_val_if_fail (JSON_IS_PARSER (parser), 0);

  if (parser->priv->scanner != NULL)
    return parser->priv->scanner->position;

  return 0;
}

/**
 * json_parser_has_assignment:
 * @parser: a #JsonParser
 * @variable_name: (out) (allow-none) (transfer none): Return location for the variable
 *   name, or %NULL
 *
 * A JSON data stream might sometimes contain an assignment, like:
 *
 * |[
 *   var _json_data = { "member_name" : [ ...
 * ]|
 *
 * even though it would technically constitute a violation of the RFC.
 *
 * #JsonParser will ignore the left hand identifier and parse the right
 * hand value of the assignment. #JsonParser will record, though, the
 * existence of the assignment in the data stream and the variable name
 * used.
 *
 * Return value: %TRUE if there was an assignment, %FALSE otherwise. If
 *   @variable_name is not %NULL it will be set to the name of the variable
 *   used in the assignment. The string is owned by #JsonParser and should
 *   never be modified or freed.
 *
 * Since: 0.4
 */
gboolean
json_parser_has_assignment (JsonParser  *parser,
                            gchar      **variable_name)
{
  JsonParserPrivate *priv;

  g_return_val_if_fail (JSON_IS_PARSER (parser), FALSE);

  priv = parser->priv;

  if (priv->has_assignment && variable_name)
    *variable_name = priv->variable_name;

  return priv->has_assignment;
}

#define GET_DATA_BLOCK_SIZE     8192

/**
 * json_parser_load_from_stream:
 * @parser: a #JsonParser
 * @stream: an open #GInputStream
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: the return location for a #GError, or %NULL
 *
 * Loads the contents of an input stream and parses them.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by
 * triggering the @cancellable object from another thread. If the
 * operation was cancelled, the error %G_IO_ERROR_CANCELLED will be set
 * on the passed @error.
 *
 * Return value: %TRUE if the data stream was successfully read and
 *   parsed, and %FALSE otherwise
 *
 * Since: 0.12
 */
gboolean
json_parser_load_from_stream (JsonParser    *parser,
                              GInputStream  *stream,
                              GCancellable  *cancellable,
                              GError       **error)
{
  GByteArray *content;
  gsize pos;
  gssize res;
  gboolean retval = FALSE;
  GError *internal_error;

  g_return_val_if_fail (JSON_IS_PARSER (parser), FALSE);
  g_return_val_if_fail (G_IS_INPUT_STREAM (stream), FALSE);
  g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);

  if (g_cancellable_set_error_if_cancelled (cancellable, error))
    return FALSE;

  content = g_byte_array_new ();
  pos = 0;

  g_byte_array_set_size (content, pos + GET_DATA_BLOCK_SIZE + 1);
  while ((res = g_input_stream_read (stream, content->data + pos,
                                     GET_DATA_BLOCK_SIZE,
                                     cancellable, error)) > 0)
    {
      pos += res;
      g_byte_array_set_size (content, pos + GET_DATA_BLOCK_SIZE + 1);
    }

  if (res < 0)
    {
      /* error has already been set */
      retval = FALSE;
      goto out;
    }

  /* zero-terminate the content; we allocated an extra byte for this */
  content->data[pos] = 0;

  internal_error = NULL;
  retval = json_parser_load (parser, (const gchar *) content->data, pos, &internal_error);

  if (internal_error != NULL)
    g_propagate_error (error, internal_error);

out:
  g_byte_array_free (content, TRUE);

  return retval;
}

typedef struct {
  GInputStream *stream;
  GByteArray *content;
  gsize pos;
} LoadData;

static void
load_data_free (gpointer data_)
{
  if (data_ != NULL)
    {
      LoadData *data = data_;

      g_object_unref (data->stream);
      g_byte_array_unref (data->content);
      g_free (data);
    }
}

/**
 * json_parser_load_from_stream_finish:
 * @parser: a #JsonParser
 * @result: a #GAsyncResult
 * @error: the return location for a #GError or %NULL
 *
 * Finishes an asynchronous stream loading started with
 * json_parser_load_from_stream_async().
 *
 * Return value: %TRUE if the content of the stream was successfully retrieves
 *   and parsed, and %FALSE otherwise. In case of error, the #GError will be
 *   filled accordingly.
 *
 * Since: 0.12
 */
gboolean
json_parser_load_from_stream_finish (JsonParser    *parser,
                                     GAsyncResult  *result,
                                     GError       **error)
{
  gboolean res;

  g_return_val_if_fail (JSON_IS_PARSER (parser), FALSE);
  g_return_val_if_fail (g_task_is_valid (result, parser), FALSE);

  res = g_task_propagate_boolean (G_TASK (result), error);
  if (res)
    {
      LoadData *data = g_task_get_task_data (G_TASK (result));
      GError *internal_error = NULL;

      /* We need to do this inside the finis() function because JsonParser will emit
       * signals, and we need to ensure that the signals are emitted in the right
       * context; it's easier to do that if we just rely on the async callback being
       * called in the right context, even if it means making the finish() function
       * necessary to complete the async operation.
       */
      res = json_parser_load (parser, (const gchar *) data->content->data, data->pos, &internal_error);
      if (internal_error != NULL)
        g_propagate_error (error, internal_error);
    }

  return res;
}

static void
read_from_stream (GTask *task,
                  gpointer source_obj,
                  gpointer task_data,
                  GCancellable *cancellable)
{
  LoadData *data = task_data;
  GError *error = NULL;
  gssize res;

  data->pos = 0;
  g_byte_array_set_size (data->content, data->pos + GET_DATA_BLOCK_SIZE + 1);
  while ((res = g_input_stream_read (data->stream,
                                     data->content->data + data->pos,
                                     GET_DATA_BLOCK_SIZE,
                                     cancellable, &error)) > 0)
    {
      data->pos += res;
      g_byte_array_set_size (data->content, data->pos + GET_DATA_BLOCK_SIZE + 1);
    }

  if (res < 0)
    {
      g_task_return_error (task, error);
      return;
    }

  /* zero-terminate the content; we allocated an extra byte for this */
  data->content->data[data->pos] = 0;
  g_task_return_boolean (task, TRUE);
}

/**
 * json_parser_load_from_stream_async:
 * @parser: a #JsonParser
 * @stream: a #GInputStream
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: the data to pass to @callback
 *
 * Asynchronously reads the contents of @stream.
 *
 * For more details, see json_parser_load_from_stream() which is the
 * synchronous version of this call.
 *
 * When the operation is finished, @callback will be called. You should
 * then call json_parser_load_from_stream_finish() to get the result
 * of the operation.
 *
 * Since: 0.12
 */
void
json_parser_load_from_stream_async (JsonParser          *parser,
                                    GInputStream        *stream,
                                    GCancellable        *cancellable,
                                    GAsyncReadyCallback  callback,
                                    gpointer             user_data)
{
  LoadData *data;
  GTask *task;

  g_return_if_fail (JSON_IS_PARSER (parser));
  g_return_if_fail (G_IS_INPUT_STREAM (stream));
  g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

  data = g_new (LoadData, 1);
  data->stream = g_object_ref (stream);
  data->content = g_byte_array_new ();
  data->pos = 0;

  task = g_task_new (parser, cancellable, callback, user_data);
  g_task_set_task_data (task, data, load_data_free);

  g_task_run_in_thread (task, read_from_stream);
  g_object_unref (task);
}
