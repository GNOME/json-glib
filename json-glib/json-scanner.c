/* json-scanner.c: Tokenizer for JSON
 * Copyright (C) 2008 OpenedHand
 *
 * Based on JsonScanner: Flexible lexical scanner for general purpose.
 * Copyright (C) 1997, 1998 Tim Janik
 *
 * Modified by Emmanuele Bassi <ebassi@openedhand.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gprintf.h>

#include "json-scanner.h"

#ifdef G_OS_WIN32
#include <io.h> /* For _read() */
#endif

enum {
  JSON_ERR_MALFORMED_SURROGATE_PAIR = G_TOKEN_LAST + 1,
};

typedef struct _JsonScannerConfig JsonScannerConfig;

struct _JsonScannerConfig
{
  /* Character sets */
  const char *cset_skip_characters; /* default: " \t\n" */
  const char *cset_identifier_first;
  const char *cset_identifier_nth;
  const char *cpair_comment_single; /* default: "#\n" */
  
  /* Should symbol lookup work case sensitive? */
  bool case_sensitive;
  
  /* Boolean values to be adjusted "on the fly"
   * to configure scanning behaviour.
   */
  bool skip_comment_multi;  /* C like comment */
  bool skip_comment_single; /* single line comment */
  bool scan_comment_multi;  /* scan multi line comments? */
  bool scan_identifier;
  bool scan_identifier_1char;
  bool scan_symbols;
  bool scan_binary;
  bool scan_octal;
  bool scan_float;
  bool scan_hex;            /* `0x0ff0' */
  bool scan_hex_dollar;     /* `$0ff0' */
  bool scan_string_sq;      /* string: 'anything' */
  bool scan_string_dq;      /* string: "\\-escapes!\n" */
  bool numbers_2_int;       /* bin, octal, hex => int */
  bool identifier_2_string;
  bool char_2_token;        /* return G_TOKEN_CHAR? */
  bool symbol_2_token;
  bool scope_0_fallback;    /* try scope 0 on lookups? */
  bool store_int64;         /* use value.v_int64 rather than v_int */
};

/*< private >
 * JsonScanner:
 *
 * Tokenizer scanner for JSON. See #GScanner
 *
 * Since: 0.6
 */
struct _JsonScanner
{
  /* unused fields */
  gpointer user_data;
  guint max_parse_errors;

  /* json_scanner_error() increments this field */
  guint parse_errors;

  /* name of input stream, featured by the default message handler */
  const char *input_name;

  /* link into the scanner configuration */
  JsonScannerConfig config;

  /* fields filled in after json_scanner_get_next_token() */
  GTokenType token;
  GTokenValue value;
  guint line;
  guint position;

  /* fields filled in after json_scanner_peek_next_token() */
  GTokenType next_token;
  GTokenValue next_value;
  guint next_line;
  guint next_position;

  /* to be considered private */
  GHashTable *symbol_table;
  const char *text;
  const char *text_end;
  char *buffer;
  guint scope_id;

  /* handler function for _warn and _error */
  JsonScannerMsgFunc msg_handler;
};

/* --- defines --- */
#define	to_lower(c)				( \
	(guchar) (							\
	  ( (((guchar)(c))>='A' && ((guchar)(c))<='Z') * ('a'-'A') ) |	\
	  ( (((guchar)(c))>=192 && ((guchar)(c))<=214) * (224-192) ) |	\
	  ( (((guchar)(c))>=216 && ((guchar)(c))<=222) * (248-216) ) |	\
	  ((guchar)(c))							\
	)								\
)

#define	READ_BUFFER_SIZE	(4000)

/* --- typedefs --- */
typedef	struct	_JsonScannerKey JsonScannerKey;

struct	_JsonScannerKey
{
  guint scope_id;
  char *symbol;
  gpointer value;
};

/* --- prototypes --- */
static gboolean json_scanner_key_equal (gconstpointer v1,
                                        gconstpointer v2);
static guint    json_scanner_key_hash  (gconstpointer v);

static inline
JsonScannerKey *json_scanner_lookup_internal (JsonScanner *scanner,
                                              guint        scope_id,
                                              const char  *symbol);
static void     json_scanner_get_token_ll    (JsonScanner *scanner,
                                              GTokenType  *token_p,
                                              GTokenValue *value_p,
                                              guint       *line_p,
                                              guint       *position_p);
static void	json_scanner_get_token_i     (JsonScanner *scanner,
                                              GTokenType  *token_p,
                                              GTokenValue *value_p,
                                              guint       *line_p,
                                              guint       *position_p);

static guchar   json_scanner_peek_next_char  (JsonScanner *scanner);
static guchar   json_scanner_get_char        (JsonScanner *scanner,
                                              guint       *line_p,
                                              guint       *position_p);
static gunichar json_scanner_get_unichar     (JsonScanner *scanner,
                                              guint       *line_p,
                                              guint       *position_p);

/* --- functions --- */

static void
json_scanner_key_free (gpointer _key)
{
  JsonScannerKey *key = _key;

  g_free (key->symbol);
  g_free (key);
}

static inline gint
json_scanner_char_2_num (guchar c,
                         guchar base)
{
  if (c >= '0' && c <= '9')
    c -= '0';
  else if (c >= 'A' && c <= 'Z')
    c -= 'A' - 10;
  else if (c >= 'a' && c <= 'z')
    c -= 'a' - 10;
  else
    return -1;
  
  if (c < base)
    return c;
  
  return -1;
}

JsonScanner *
json_scanner_new (void)
{
  JsonScanner *scanner;
  
  scanner = g_new0 (JsonScanner, 1);
  
  scanner->user_data = NULL;
  scanner->max_parse_errors = 1;
  scanner->parse_errors	= 0;
  scanner->input_name = NULL;
  
  scanner->config = (JsonScannerConfig) {
    .cset_skip_characters = ( " \t\r\n" ),
    .cset_identifier_first = (
      "_"
      G_CSET_a_2_z
      G_CSET_A_2_Z
    ),
    .cset_identifier_nth = (
      G_CSET_DIGITS
      "-_"
      G_CSET_a_2_z
      G_CSET_A_2_Z
    ),
    .cpair_comment_single = ( "//\n" ),
    .case_sensitive = true,
    .skip_comment_multi = true,
    .skip_comment_single = true,
    .scan_comment_multi = false,
    .scan_identifier = true,
    .scan_identifier_1char = true,
    .scan_symbols = true,
    .scan_binary = true,
    .scan_octal = true,
    .scan_float = true,
    .scan_hex = true,
    .scan_hex_dollar = true,
    .scan_string_sq = true,
    .scan_string_dq = true,
    .numbers_2_int = true,
    .identifier_2_string = false,
    .char_2_token = true,
    .symbol_2_token = true,
    .scope_0_fallback = false,
    .store_int64 = true,
  };

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;

  scanner->next_token = G_TOKEN_NONE;
  scanner->next_value.v_int64 = 0;
  scanner->next_line = 1;
  scanner->next_position = 0;

  scanner->symbol_table = g_hash_table_new_full (json_scanner_key_hash,
                                                 json_scanner_key_equal,
                                                 json_scanner_key_free,
                                                 NULL);
  scanner->text = NULL;
  scanner->text_end = NULL;
  scanner->buffer = NULL;
  scanner->scope_id = 0;

  return scanner;
}

static inline void
json_scanner_free_value (GTokenType  *token_p,
                         GTokenValue *value_p)
{
  switch (*token_p)
    {
    case G_TOKEN_STRING:
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      g_free (value_p->v_string);
      break;
      
    default:
      break;
    }
  
  *token_p = G_TOKEN_NONE;
}

void
json_scanner_destroy (JsonScanner *scanner)
{
  g_return_if_fail (scanner != NULL);
  
  g_hash_table_unref (scanner->symbol_table);

  json_scanner_free_value (&scanner->token, &scanner->value);
  json_scanner_free_value (&scanner->next_token, &scanner->next_value);

  g_free (scanner->buffer);
  g_free (scanner);
}

void
json_scanner_set_msg_handler (JsonScanner        *scanner,
                              JsonScannerMsgFunc  msg_handler,
                              gpointer            user_data)
{
  g_return_if_fail (scanner != NULL);

  scanner->msg_handler = msg_handler;
  scanner->user_data = user_data;
}

void
json_scanner_error (JsonScanner *scanner,
                    const char  *format,
                    ...)
{
  g_return_if_fail (scanner != NULL);
  g_return_if_fail (format != NULL);
  
  scanner->parse_errors++;
  
  if (scanner->msg_handler)
    {
      va_list args;
      char *string;
      
      va_start (args, format);
      string = g_strdup_vprintf (format, args);
      va_end (args);
      
      scanner->msg_handler (scanner, string, scanner->user_data);
      
      g_free (string);
    }
}

static gboolean
json_scanner_key_equal (gconstpointer v1,
                        gconstpointer v2)
{
  const JsonScannerKey *key1 = v1;
  const JsonScannerKey *key2 = v2;
  
  return (key1->scope_id == key2->scope_id) &&
         (strcmp (key1->symbol, key2->symbol) == 0);
}

static guint
json_scanner_key_hash (gconstpointer v)
{
  const JsonScannerKey *key = v;
  char *c;
  guint h;
  
  h = key->scope_id;
  for (c = key->symbol; *c; c++)
    h = (h << 5) - h + *c;
  
  return h;
}

static inline JsonScannerKey *
json_scanner_lookup_internal (JsonScanner *scanner,
                              guint        scope_id,
                              const char  *symbol)
{
  JsonScannerKey *key_p;
  JsonScannerKey key;
  
  key.scope_id = scope_id;
  
  if (!scanner->config.case_sensitive)
    {
      char *d;
      const char *c;
      
      key.symbol = g_new (char, strlen (symbol) + 1);
      for (d = key.symbol, c = symbol; *c; c++, d++)
	*d = to_lower (*c);
      *d = 0;
      key_p = g_hash_table_lookup (scanner->symbol_table, &key);
      g_free (key.symbol);
    }
  else
    {
      key.symbol = (char *) symbol;
      key_p = g_hash_table_lookup (scanner->symbol_table, &key);
    }
  
  return key_p;
}

void
json_scanner_scope_add_symbol (JsonScanner *scanner,
                               guint        scope_id,
                               const char  *symbol,
                               gpointer     value)
{
  JsonScannerKey *key;

  g_return_if_fail (scanner != NULL);
  g_return_if_fail (symbol != NULL);

  key = json_scanner_lookup_internal (scanner, scope_id, symbol);
  if (!key)
    {
      key = g_new (JsonScannerKey, 1);
      key->scope_id = scope_id;
      key->symbol = g_strdup (symbol);
      key->value = value;
      if (!scanner->config.case_sensitive)
	{
	  char *c;

	  c = key->symbol;
	  while (*c != 0)
	    {
	      *c = to_lower (*c);
	      c++;
	    }
	}

      g_hash_table_add (scanner->symbol_table, key);
    }
  else
    key->value = value;
}

GTokenType
json_scanner_peek_next_token (JsonScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_EOF);

  if (scanner->next_token == G_TOKEN_NONE)
    {
      scanner->next_line = scanner->line;
      scanner->next_position = scanner->position;
      json_scanner_get_token_i (scanner,
                                &scanner->next_token,
                                &scanner->next_value,
                                &scanner->next_line,
                                &scanner->next_position);
    }

  return scanner->next_token;
}

GTokenType
json_scanner_get_next_token (JsonScanner *scanner)
{
  g_return_val_if_fail (scanner != NULL, G_TOKEN_EOF);

  if (scanner->next_token != G_TOKEN_NONE)
    {
      json_scanner_free_value (&scanner->token, &scanner->value);

      scanner->token = scanner->next_token;
      scanner->value = scanner->next_value;
      scanner->line = scanner->next_line;
      scanner->position = scanner->next_position;
      scanner->next_token = G_TOKEN_NONE;
    }
  else
    json_scanner_get_token_i (scanner,
                              &scanner->token,
                              &scanner->value,
                              &scanner->line,
                              &scanner->position);

  return scanner->token;
}

void
json_scanner_input_text (JsonScanner *scanner,
                         const char  *text,
                         guint        text_len)
{
  g_return_if_fail (scanner != NULL);
  if (text_len)
    g_return_if_fail (text != NULL);
  else
    text = NULL;

  scanner->token = G_TOKEN_NONE;
  scanner->value.v_int64 = 0;
  scanner->line = 1;
  scanner->position = 0;
  scanner->next_token = G_TOKEN_NONE;

  scanner->text = text;
  scanner->text_end = text + text_len;

  if (scanner->buffer)
    {
      g_free (scanner->buffer);
      scanner->buffer = NULL;
    }
}

static guchar
json_scanner_peek_next_char (JsonScanner *scanner)
{
  if (scanner->text < scanner->text_end)
    return *scanner->text;
  else
    return 0;
}

static guchar
json_scanner_get_char (JsonScanner *scanner,
                       guint       *line_p,
                       guint       *position_p)
{
  guchar fchar;

  if (scanner->text < scanner->text_end)
    fchar = *(scanner->text++);
  else
    fchar = 0;
  
  if (fchar == '\n')
    {
      (*position_p) = 0;
      (*line_p)++;
    }
  else if (fchar)
    {
      (*position_p)++;
    }
  
  return fchar;
}

#define is_hex_digit(c)         (((c) >= '0' && (c) <= '9') || \
                                 ((c) >= 'a' && (c) <= 'f') || \
                                 ((c) >= 'A' && (c) <= 'F'))
#define to_hex_digit(c)         (((c) <= '9') ? (c) - '0' : ((c) & 7) + 9)

static gunichar
json_scanner_get_unichar (JsonScanner *scanner,
                          guint       *line_p,
                          guint       *position_p)
{
  gunichar uchar;

  uchar = 0;
  for (int i = 0; i < 4; i++)
    {
      char ch = json_scanner_get_char (scanner, line_p, position_p);

      if (is_hex_digit (ch))
        uchar += ((gunichar) to_hex_digit (ch) << ((3 - i) * 4));
      else
        break;
    }

  g_assert (g_unichar_validate (uchar) || g_unichar_type (uchar) == G_UNICODE_SURROGATE);

  return uchar;
}

/*
 * decode_utf16_surrogate_pair:
 * @units: (array length=2): a pair of UTF-16 code points
 *
 * Decodes a surrogate pair of UTF-16 code points into the equivalent
 * Unicode code point.
 *
 * Returns: the Unicode code point equivalent to the surrogate pair
 */
static inline gunichar
decode_utf16_surrogate_pair (const gunichar units[2])
{
  gunichar ucs;

  g_assert (0xd800 <= units[0] && units[0] <= 0xdbff);
  g_assert (0xdc00 <= units[1] && units[1] <= 0xdfff);

  ucs = 0x10000;
  ucs += (units[0] & 0x3ff) << 10;
  ucs += (units[1] & 0x3ff);

  return ucs;
}

void
json_scanner_unexp_token (JsonScanner *scanner,
                          GTokenType   expected_token,
                          const char  *identifier_spec,
                          const char  *symbol_spec,
                          const char  *symbol_name,
                          const char  *message)
{
  char *token_string;
  gsize token_string_len;
  char *expected_string;
  gsize expected_string_len;
  const char *message_prefix;
  bool print_unexp;
  
  g_return_if_fail (scanner != NULL);
  
  if (identifier_spec == NULL)
    identifier_spec = "identifier";
  if (symbol_spec == NULL)
    symbol_spec = "symbol";
  
  token_string_len = 56;
  token_string = g_new (char, token_string_len + 1);
  expected_string_len = 64;
  expected_string = g_new (char, expected_string_len + 1);
  print_unexp = true;
  
  switch (scanner->token)
    {
    case G_TOKEN_EOF:
      g_snprintf (token_string, token_string_len, "end of file");
      break;
      
    default:
      if (scanner->token >= 1 && scanner->token <= 255)
	{
	  if ((scanner->token >= ' ' && scanner->token <= '~') ||
	      strchr (scanner->config.cset_identifier_first, scanner->token) ||
	      strchr (scanner->config.cset_identifier_nth, scanner->token))
	    g_snprintf (token_string, token_string_len, "character `%c'", scanner->token);
	  else
	    g_snprintf (token_string, token_string_len, "character `\\%o'", scanner->token);
	  break;
	}
      else if (!scanner->config.symbol_2_token)
	{
	  g_snprintf (token_string, token_string_len, "(unknown) token <%d>", scanner->token);
	  break;
	}
      /* fall through */
    case G_TOKEN_SYMBOL:
      if (expected_token == G_TOKEN_SYMBOL ||
	  (scanner->config.symbol_2_token &&
	   expected_token > G_TOKEN_LAST))
	print_unexp = false;
      if (symbol_name)
	g_snprintf (token_string, token_string_len,
                    "%s%s `%s'",
                    print_unexp ? "" : "invalid ",
                    symbol_spec,
                    symbol_name);
      else
	g_snprintf (token_string, token_string_len,
                    "%s%s",
                    print_unexp ? "" : "invalid ",
                    symbol_spec);
      break;
 
    case G_TOKEN_ERROR:
      print_unexp = false;
      expected_token = G_TOKEN_NONE;
      switch (scanner->value.v_error)
	{
	case G_ERR_UNEXP_EOF:
	  g_snprintf (token_string, token_string_len, "scanner: unexpected end of file");
	  break;
	  
	case G_ERR_UNEXP_EOF_IN_STRING:
	  g_snprintf (token_string, token_string_len, "scanner: unterminated string constant");
	  break;
	  
	case G_ERR_UNEXP_EOF_IN_COMMENT:
	  g_snprintf (token_string, token_string_len, "scanner: unterminated comment");
	  break;
	  
	case G_ERR_NON_DIGIT_IN_CONST:
	  g_snprintf (token_string, token_string_len, "scanner: non digit in constant");
	  break;
	  
	case G_ERR_FLOAT_RADIX:
	  g_snprintf (token_string, token_string_len, "scanner: invalid radix for floating constant");
	  break;
	  
	case G_ERR_FLOAT_MALFORMED:
	  g_snprintf (token_string, token_string_len, "scanner: malformed floating constant");
	  break;
	  
	case G_ERR_DIGIT_RADIX:
	  g_snprintf (token_string, token_string_len, "scanner: digit is beyond radix");
	  break;

	case JSON_ERR_MALFORMED_SURROGATE_PAIR:
	  g_snprintf (token_string, token_string_len, "scanner: malformed surrogate pair");
	  break;

	case G_ERR_UNKNOWN:
	default:
	  g_snprintf (token_string, token_string_len, "scanner: unknown error");
	  break;
	}
      break;
      
    case G_TOKEN_CHAR:
      g_snprintf (token_string, token_string_len, "character `%c'", scanner->value.v_char);
      break;
      
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      if (expected_token == G_TOKEN_IDENTIFIER ||
	  expected_token == G_TOKEN_IDENTIFIER_NULL)
	print_unexp = false;
      g_snprintf (token_string, token_string_len,
                  "%s%s `%s'",
                  print_unexp ? "" : "invalid ",
                  identifier_spec,
                  scanner->token == G_TOKEN_IDENTIFIER ? scanner->value.v_string : "null");
      break;
      
    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_INT:
    case G_TOKEN_HEX:
      if (scanner->config.store_int64)
	g_snprintf (token_string, token_string_len, "number `%" G_GUINT64_FORMAT "'", scanner->value.v_int64);
      else
	g_snprintf (token_string, token_string_len, "number `%lu'", scanner->value.v_int);
      break;
      
    case G_TOKEN_FLOAT:
      g_snprintf (token_string, token_string_len, "number `%.3f'", scanner->value.v_float);
      break;
      
    case G_TOKEN_STRING:
      if (expected_token == G_TOKEN_STRING)
	print_unexp = false;
      g_snprintf (token_string, token_string_len,
                  "%s%sstring constant \"%s\"",
                  print_unexp ? "" : "invalid ",
                  scanner->value.v_string[0] == 0 ? "empty " : "",
                  scanner->value.v_string);
      token_string[token_string_len - 2] = '"';
      token_string[token_string_len - 1] = 0;
      break;
      
    case G_TOKEN_COMMENT_SINGLE:
    case G_TOKEN_COMMENT_MULTI:
      g_snprintf (token_string, token_string_len, "comment");
      break;
      
    case G_TOKEN_NONE:
      /* somehow the user's parsing code is screwed, there isn't much
       * we can do about it.
       * Note, a common case to trigger this is
       * json_scanner_peek_next_token(); json_scanner_unexp_token();
       * without an intermediate json_scanner_get_next_token().
       */
      g_assert_not_reached ();
      break;
    }
  
  
  switch (expected_token)
    {
      const char *tstring;
      bool need_valid;
    case G_TOKEN_EOF:
      g_snprintf (expected_string, expected_string_len, "end of file");
      break;
    default:
      if (expected_token >= 1 && expected_token <= 255)
	{
	  if ((expected_token >= ' ' && expected_token <= '~') ||
	      strchr (scanner->config.cset_identifier_first, expected_token) ||
	      strchr (scanner->config.cset_identifier_nth, expected_token))
	    g_snprintf (expected_string, expected_string_len, "character `%c'", expected_token);
	  else
	    g_snprintf (expected_string, expected_string_len, "character `\\%o'", expected_token);
	  break;
	}
      else if (!scanner->config.symbol_2_token)
	{
	  g_snprintf (expected_string, expected_string_len, "(unknown) token <%d>", expected_token);
	  break;
	}
      /* fall through */
    case G_TOKEN_SYMBOL:
      need_valid = (scanner->token == G_TOKEN_SYMBOL ||
		    (scanner->config.symbol_2_token &&
		     scanner->token > G_TOKEN_LAST));
      g_snprintf (expected_string, expected_string_len,
                  "%s%s",
                  need_valid ? "valid " : "",
                  symbol_spec);
      /* FIXME: should we attempt to lookup the symbol_name for symbol_2_token? */
      break;
    case G_TOKEN_CHAR:
      g_snprintf (expected_string, expected_string_len, "%scharacter",
		  scanner->token == G_TOKEN_CHAR ? "valid " : "");
      break;
    case G_TOKEN_BINARY:
      tstring = "binary";
      g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_OCTAL:
      tstring = "octal";
      g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_INT:
      tstring = "integer";
      g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_HEX:
      tstring = "hexadecimal";
      g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_FLOAT:
      tstring = "float";
      g_snprintf (expected_string, expected_string_len, "%snumber (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_STRING:
      g_snprintf (expected_string,
		  expected_string_len,
		  "%sstring constant",
		  scanner->token == G_TOKEN_STRING ? "valid " : "");
      break;
    case G_TOKEN_IDENTIFIER:
    case G_TOKEN_IDENTIFIER_NULL:
      need_valid = (scanner->token == G_TOKEN_IDENTIFIER_NULL ||
		    scanner->token == G_TOKEN_IDENTIFIER);
      g_snprintf (expected_string,
		  expected_string_len,
		  "%s%s",
		  need_valid ? "valid " : "",
		  identifier_spec);
      break;
    case G_TOKEN_COMMENT_SINGLE:
      tstring = "single-line";
      g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_COMMENT_MULTI:
      tstring = "multi-line";
      g_snprintf (expected_string, expected_string_len, "%scomment (%s)",
		  scanner->token == expected_token ? "valid " : "", tstring);
      break;
    case G_TOKEN_NONE:
    case G_TOKEN_ERROR:
      /* this is handled upon printout */
      break;
    }
  
  if (message && message[0] != 0)
    message_prefix = " - ";
  else
    {
      message_prefix = "";
      message = "";
    }
  if (expected_token == G_TOKEN_ERROR)
    {
      json_scanner_error (scanner,
                          "failure around %s%s%s",
                          token_string,
                          message_prefix,
                          message);
    }
  else if (expected_token == G_TOKEN_NONE)
    {
      if (print_unexp)
	json_scanner_error (scanner,
                            "unexpected %s%s%s",
                            token_string,
                            message_prefix,
                            message);
      else
	json_scanner_error (scanner,
                            "%s%s%s",
                            token_string,
                            message_prefix,
                            message);
    }
  else
    {
      if (print_unexp)
	json_scanner_error (scanner,
                            "unexpected %s, expected %s%s%s",
                            token_string,
                            expected_string,
                            message_prefix,
                            message);
      else
	json_scanner_error (scanner,
                            "%s, expected %s%s%s",
                            token_string,
                            expected_string,
                            message_prefix,
                            message);
    }
  
  g_free (token_string);
  g_free (expected_string);
}

static void
json_scanner_get_token_i (JsonScanner	*scanner,
		          GTokenType	*token_p,
		          GTokenValue	*value_p,
		          guint		*line_p,
		          guint		*position_p)
{
  do
    {
      json_scanner_free_value (token_p, value_p);
      json_scanner_get_token_ll (scanner, token_p, value_p, line_p, position_p);
    }
  while (((*token_p > 0 && *token_p < 256) &&
	  strchr (scanner->config.cset_skip_characters, *token_p)) ||
	 (*token_p == G_TOKEN_CHAR &&
	  strchr (scanner->config.cset_skip_characters, value_p->v_char)) ||
	 (*token_p == G_TOKEN_COMMENT_MULTI &&
	  scanner->config.skip_comment_multi) ||
	 (*token_p == G_TOKEN_COMMENT_SINGLE &&
	  scanner->config.skip_comment_single));
  
  switch (*token_p)
    {
    case G_TOKEN_IDENTIFIER:
      if (scanner->config.identifier_2_string)
	*token_p = G_TOKEN_STRING;
      break;
      
    case G_TOKEN_SYMBOL:
      if (scanner->config.symbol_2_token)
	*token_p = GPOINTER_TO_INT (value_p->v_symbol);
      break;
      
    case G_TOKEN_BINARY:
    case G_TOKEN_OCTAL:
    case G_TOKEN_HEX:
      if (scanner->config.numbers_2_int)
	*token_p = G_TOKEN_INT;
      break;
      
    default:
      break;
    }
  
  errno = 0;
}

static void
json_scanner_get_token_ll (JsonScanner *scanner,
                           GTokenType  *token_p,
                           GTokenValue *value_p,
                           guint       *line_p,
                           guint       *position_p)
{
  const JsonScannerConfig *config;
  GTokenType token;
  bool in_comment_multi = false;
  bool in_comment_single = false;
  bool in_string_sq = false;
  bool in_string_dq = false;
  GString *gstring = NULL;
  GTokenValue value;
  guchar ch;
  
  config = &scanner->config;
  (*value_p).v_int64 = 0;
  
  if (scanner->text >= scanner->text_end ||
      scanner->token == G_TOKEN_EOF)
    {
      *token_p = G_TOKEN_EOF;
      return;
    }
  
  gstring = NULL;
  
  do /* while (ch != 0) */
    {
      bool dotted_float = false;
      
      ch = json_scanner_get_char (scanner, line_p, position_p);
      
      value.v_int64 = 0;
      token = G_TOKEN_NONE;
      
      /* this is *evil*, but needed ;(
       * we first check for identifier first character, because	 it
       * might interfere with other key chars like slashes or numbers
       */
      if (config->scan_identifier &&
	  ch != 0 &&
          strchr (config->cset_identifier_first, ch))
	goto identifier_precedence;
      
      switch (ch)
	{
	case 0:
	  token = G_TOKEN_EOF;
	  (*position_p)++;
	  /* ch = 0; */
	  break;
	  
	case '/':
	  if (!config->scan_comment_multi ||
	      json_scanner_peek_next_char (scanner) != '*')
	    goto default_case;
	  json_scanner_get_char (scanner, line_p, position_p);
	  token = G_TOKEN_COMMENT_MULTI;
	  in_comment_multi = true;
	  gstring = g_string_new (NULL);
	  while ((ch = json_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '*' && json_scanner_peek_next_char (scanner) == '/')
		{
		  json_scanner_get_char (scanner, line_p, position_p);
		  in_comment_multi = false;
		  break;
		}
	      else
		gstring = g_string_append_c (gstring, ch);
	    }
	  ch = 0;
	  break;
	  
	case '\'':
	  if (!config->scan_string_sq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_sq = true;
	  gstring = g_string_new (NULL);
	  while ((ch = json_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '\'')
		{
		  in_string_sq = FALSE;
		  break;
		}
	      else
		gstring = g_string_append_c (gstring, ch);
	    }
	  ch = 0;
	  break;
	  
	case '"':
	  if (!config->scan_string_dq)
	    goto default_case;
	  token = G_TOKEN_STRING;
	  in_string_dq = true;
	  gstring = g_string_new (NULL);
	  while ((ch = json_scanner_get_char (scanner, line_p, position_p)) != 0)
	    {
	      if (ch == '"' || token == G_TOKEN_ERROR)
		{
		  in_string_dq = FALSE;
		  break;
		}
	      else
		{
		  if (ch == '\\')
		    {
		      ch = json_scanner_get_char (scanner, line_p, position_p);
		      switch (ch)
			{
			  guint	i;
			  guint	fchar;
			  
			case 0:
			  break;
			  
			case '\\':
			  gstring = g_string_append_c (gstring, '\\');
			  break;
			  
			case 'n':
			  gstring = g_string_append_c (gstring, '\n');
			  break;
			  
			case 't':
			  gstring = g_string_append_c (gstring, '\t');
			  break;
			  
			case 'r':
			  gstring = g_string_append_c (gstring, '\r');
			  break;
			  
			case 'b':
			  gstring = g_string_append_c (gstring, '\b');
			  break;
			  
			case 'f':
			  gstring = g_string_append_c (gstring, '\f');
			  break;

                        case 'u':
                          fchar = json_scanner_peek_next_char (scanner);
                          if (is_hex_digit (fchar))
                            {
                              gunichar ucs;

                              ucs = json_scanner_get_unichar (scanner, line_p, position_p);

                              /* resolve UTF-16 surrogates for Unicode characters not in the BMP,
                                * as per ECMA 404, § 9, "String"
                                */
                              if (g_unichar_type (ucs) == G_UNICODE_SURROGATE)
                                {
                                  /* read next surrogate */
                                  if ('\\' == json_scanner_get_char (scanner, line_p, position_p) &&
                                      'u' == json_scanner_get_char (scanner, line_p, position_p))
                                    {
                                      gunichar units[2];

                                      units[0] = ucs;
                                      units[1] = json_scanner_get_unichar (scanner, line_p, position_p);

                                      if (0xdc00 <= units[1] && units[1] <= 0xdfff &&
                                          0xd800 <= units[0] && units[0] <= 0xdbff)
                                        {
                                          ucs = decode_utf16_surrogate_pair (units);
                                          g_assert (g_unichar_validate (ucs));
                                        }
                                      else
                                        {
                                          token = G_TOKEN_ERROR;
                                          value.v_error = JSON_ERR_MALFORMED_SURROGATE_PAIR;
                                          gstring = NULL;
                                          break;
                                        }

                                    }
                                }

                              gstring = g_string_append_unichar (gstring, ucs);
                            }
                          break;
			  
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			  i = ch - '0';
			  fchar = json_scanner_peek_next_char (scanner);
			  if (fchar >= '0' && fchar <= '7')
			    {
			      ch = json_scanner_get_char (scanner, line_p, position_p);
			      i = i * 8 + ch - '0';
			      fchar = json_scanner_peek_next_char (scanner);
			      if (fchar >= '0' && fchar <= '7')
				{
				  ch = json_scanner_get_char (scanner, line_p, position_p);
				  i = i * 8 + ch - '0';
				}
			    }
			  gstring = g_string_append_c (gstring, i);
			  break;
			  
			default:
			  gstring = g_string_append_c (gstring, ch);
			  break;
			}
		    }
		  else
		    gstring = g_string_append_c (gstring, ch);
		}
	    }
	  ch = 0;
	  break;
	  
	case '.':
	  if (!config->scan_float)
	    goto default_case;
	  token = G_TOKEN_FLOAT;
	  dotted_float = true;
	  ch = json_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;
	  
	case '$':
	  if (!config->scan_hex_dollar)
	    goto default_case;
	  token = G_TOKEN_HEX;
	  ch = json_scanner_get_char (scanner, line_p, position_p);
	  goto number_parsing;
	  
	case '0':
	  if (config->scan_octal)
	    token = G_TOKEN_OCTAL;
	  else
	    token = G_TOKEN_INT;
	  ch = json_scanner_peek_next_char (scanner);
	  if (config->scan_hex && (ch == 'x' || ch == 'X'))
	    {
	      token = G_TOKEN_HEX;
	      json_scanner_get_char (scanner, line_p, position_p);
	      ch = json_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (json_scanner_char_2_num (ch, 16) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_DIGIT_RADIX;
		  ch = 0;
		  break;
		}
	    }
	  else if (config->scan_binary && (ch == 'b' || ch == 'B'))
	    {
	      token = G_TOKEN_BINARY;
	      json_scanner_get_char (scanner, line_p, position_p);
	      ch = json_scanner_get_char (scanner, line_p, position_p);
	      if (ch == 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_UNEXP_EOF;
		  (*position_p)++;
		  break;
		}
	      if (json_scanner_char_2_num (ch, 10) < 0)
		{
		  token = G_TOKEN_ERROR;
		  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
		  ch = 0;
		  break;
		}
	    }
	  else
	    ch = '0';
	  /* fall through */
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	number_parsing:
	{
          bool in_number = true;
	  char *endptr;
	  
	  if (token == G_TOKEN_NONE)
	    token = G_TOKEN_INT;
	  
	  gstring = g_string_new (dotted_float ? "0." : "");
	  gstring = g_string_append_c (gstring, ch);
	  
	  do /* while (in_number) */
	    {
	      bool is_E;
	      
	      is_E = token == G_TOKEN_FLOAT && (ch == 'e' || ch == 'E');
	      
	      ch = json_scanner_peek_next_char (scanner);
	      
	      if (json_scanner_char_2_num (ch, 36) >= 0 ||
		  (config->scan_float && ch == '.') ||
		  (is_E && (ch == '+' || ch == '-')))
		{
		  ch = json_scanner_get_char (scanner, line_p, position_p);
		  
		  switch (ch)
		    {
		    case '.':
		      if (token != G_TOKEN_INT && token != G_TOKEN_OCTAL)
			{
			  value.v_error = token == G_TOKEN_FLOAT ? G_ERR_FLOAT_MALFORMED : G_ERR_FLOAT_RADIX;
			  token = G_TOKEN_ERROR;
			  in_number = false;
			}
		      else
			{
			  token = G_TOKEN_FLOAT;
			  gstring = g_string_append_c (gstring, ch);
			}
		      break;
		      
		    case '0':
		    case '1':
		    case '2':
		    case '3':
		    case '4':
		    case '5':
		    case '6':
		    case '7':
		    case '8':
		    case '9':
		      gstring = g_string_append_c (gstring, ch);
		      break;
		      
		    case '-':
		    case '+':
		      if (token != G_TOKEN_FLOAT)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = false;
			}
		      else
			gstring = g_string_append_c (gstring, ch);
		      break;
		      
		    case 'e':
		    case 'E':
		      if ((token != G_TOKEN_HEX && !config->scan_float) ||
			  (token != G_TOKEN_HEX &&
			   token != G_TOKEN_OCTAL &&
			   token != G_TOKEN_FLOAT &&
			   token != G_TOKEN_INT))
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = false;
			}
		      else
			{
			  if (token != G_TOKEN_HEX)
			    token = G_TOKEN_FLOAT;
			  gstring = g_string_append_c (gstring, ch);
			}
		      break;
		      
		    default:
		      if (token != G_TOKEN_HEX)
			{
			  token = G_TOKEN_ERROR;
			  value.v_error = G_ERR_NON_DIGIT_IN_CONST;
			  in_number = FALSE;
			}
		      else
			gstring = g_string_append_c (gstring, ch);
		      break;
		    }
		}
	      else
		in_number = FALSE;
	    }
	  while (in_number);
	  
	  endptr = NULL;
	  if (token == G_TOKEN_FLOAT)
	    value.v_float = g_strtod (gstring->str, &endptr);
	  else
	    {
	      guint64 ui64 = 0;
	      switch (token)
		{
		case G_TOKEN_BINARY:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 2);
		  break;
		case G_TOKEN_OCTAL:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 8);
		  break;
		case G_TOKEN_INT:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 10);
		  break;
		case G_TOKEN_HEX:
		  ui64 = g_ascii_strtoull (gstring->str, &endptr, 16);
		  break;
		default: ;
		}
	      if (scanner->config.store_int64)
		value.v_int64 = ui64;
	      else
		value.v_int = ui64;
	    }
	  if (endptr && *endptr)
	    {
	      token = G_TOKEN_ERROR;
	      if (*endptr == 'e' || *endptr == 'E')
		value.v_error = G_ERR_NON_DIGIT_IN_CONST;
	      else
		value.v_error = G_ERR_DIGIT_RADIX;
	    }
	  g_string_free (gstring, TRUE);
	  gstring = NULL;
	  ch = 0;
	} /* number_parsing:... */
	break;
	
	default:
	default_case:
	{
	  if (config->cpair_comment_single &&
	      ch == config->cpair_comment_single[0])
	    {
	      token = G_TOKEN_COMMENT_SINGLE;
	      in_comment_single = true;
	      gstring = g_string_new (NULL);
	      ch = json_scanner_get_char (scanner, line_p, position_p);
	      while (ch != 0)
		{
		  if (ch == config->cpair_comment_single[1])
		    {
		      in_comment_single = false;
		      ch = 0;
		      break;
		    }
		  
		  gstring = g_string_append_c (gstring, ch);
		  ch = json_scanner_get_char (scanner, line_p, position_p);
		}
	      /* ignore a missing newline at EOF for single line comments */
	      if (in_comment_single &&
		  config->cpair_comment_single[1] == '\n')
		in_comment_single = FALSE;
	    }
	  else if (config->scan_identifier && ch &&
		   strchr (config->cset_identifier_first, ch))
	    {
	    identifier_precedence:
	      
	      if (config->cset_identifier_nth && ch &&
		  strchr (config->cset_identifier_nth,
			  json_scanner_peek_next_char (scanner)))
		{
		  token = G_TOKEN_IDENTIFIER;
		  gstring = g_string_new (NULL);
		  gstring = g_string_append_c (gstring, ch);
		  do
		    {
		      ch = json_scanner_get_char (scanner, line_p, position_p);
		      gstring = g_string_append_c (gstring, ch);
		      ch = json_scanner_peek_next_char (scanner);
		    }
		  while (ch && strchr (config->cset_identifier_nth, ch));
		  ch = 0;
		}
	      else if (config->scan_identifier_1char)
		{
		  token = G_TOKEN_IDENTIFIER;
		  value.v_identifier = g_new0 (char, 2);
		  value.v_identifier[0] = ch;
		  ch = 0;
		}
	    }
	  if (ch)
	    {
	      if (config->char_2_token)
		token = ch;
	      else
		{
		  token = G_TOKEN_CHAR;
		  value.v_char = ch;
		}
	      ch = 0;
	    }
	} /* default_case:... */
	break;
	}
      g_assert (ch == 0 && token != G_TOKEN_NONE); /* paranoid */
    }
  while (ch != 0);
  
  if (in_comment_multi || in_comment_single ||
      in_string_sq || in_string_dq)
    {
      token = G_TOKEN_ERROR;
      if (gstring)
	{
	  g_string_free (gstring, TRUE);
	  gstring = NULL;
	}
      (*position_p)++;
      if (in_comment_multi || in_comment_single)
	value.v_error = G_ERR_UNEXP_EOF_IN_COMMENT;
      else /* (in_string_sq || in_string_dq) */
	value.v_error = G_ERR_UNEXP_EOF_IN_STRING;
    }
  
  if (gstring)
    {
      value.v_string = g_string_free (gstring, FALSE);
      gstring = NULL;
    }
  
  if (token == G_TOKEN_IDENTIFIER)
    {
      if (config->scan_symbols)
	{
	  JsonScannerKey *key;
	  guint scope_id;
	  
	  scope_id = scanner->scope_id;
	  key = json_scanner_lookup_internal (scanner, scope_id, value.v_identifier);
	  if (!key && scope_id && scanner->config.scope_0_fallback)
	    key = json_scanner_lookup_internal (scanner, 0, value.v_identifier);
	  
	  if (key)
	    {
	      g_free (value.v_identifier);
	      token = G_TOKEN_SYMBOL;
	      value.v_symbol = key->value;
	    }
	}
    }
  
  *token_p = token;
  *value_p = value;
}

gint64
json_scanner_get_int64_value (const JsonScanner *scanner)
{
  return scanner->value.v_int64;
}

double
json_scanner_get_float_value (const JsonScanner *scanner)
{
  return scanner->value.v_float;
}

const char *
json_scanner_get_string_value (const JsonScanner *scanner)
{
  return scanner->value.v_string;
}

char *
json_scanner_dup_string_value (const JsonScanner *scanner)
{
  return g_strdup (scanner->value.v_string);
}

const char *
json_scanner_get_identifier (const JsonScanner *scanner)
{
  return scanner->value.v_identifier;
}

char *
json_scanner_dup_identifier (const JsonScanner *scanner)
{
  return g_strdup (scanner->value.v_identifier);
}

guint
json_scanner_get_current_line (const JsonScanner *scanner)
{
  return scanner->line;
}

guint
json_scanner_get_current_position (const JsonScanner *scanner)
{
  return scanner->position;
}

GTokenType
json_scanner_get_current_token (const JsonScanner *scanner)
{
  return scanner->token;
}

guint
json_scanner_get_current_scope_id (const JsonScanner *scanner)
{
  return scanner->scope_id;
}
