/* json-scanner.h: Tokenizer for JSON
 *
 * This file is part of JSON-GLib
 * Copyright (C) 2008 OpenedHand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*
 * JsonScanner is a specialized tokenizer for JSON adapted from
 * the GScanner tokenizer in GLib; GScanner came with this notice:
 * 
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 *
 * JsonScanner: modified by Emmanuele Bassi <ebassi@openedhand.com>
 */

#ifndef __JSON_SCANNER_H__
#define __JSON_SCANNER_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _JsonScanner       JsonScanner;

typedef void (* JsonScannerMsgFunc) (JsonScanner *scanner,
                                     const char  *message,
                                     gpointer     user_data);

/*< private >
 * JsonTokenType:
 * @JSON_TOKEN_INVALID: marker
 * @JSON_TOKEN_TRUE: symbol for 'true' bareword
 * @JSON_TOKEN_FALSE: symbol for 'false' bareword
 * @JSON_TOKEN_NULL: symbol for 'null' bareword
 * @JSON_TOKEN_VAR: symbol for 'var' bareword
 * @JSON_TOKEN_LAST: marker
 *
 * Tokens for JsonScanner-based parser, extending #GTokenType.
 */
typedef enum {
  JSON_TOKEN_INVALID = G_TOKEN_LAST,

  JSON_TOKEN_TRUE,
  JSON_TOKEN_FALSE,
  JSON_TOKEN_NULL,
  JSON_TOKEN_VAR,

  JSON_TOKEN_LAST
} JsonTokenType;

G_GNUC_INTERNAL
JsonScanner *json_scanner_new                  (void);
G_GNUC_INTERNAL
void         json_scanner_destroy              (JsonScanner *scanner);
G_GNUC_INTERNAL
void         json_scanner_input_text           (JsonScanner *scanner,
                                                const gchar *text,
                                                guint        text_len);
G_GNUC_INTERNAL
GTokenType   json_scanner_get_next_token       (JsonScanner *scanner);
G_GNUC_INTERNAL
GTokenType   json_scanner_peek_next_token      (JsonScanner *scanner);
G_GNUC_INTERNAL
void         json_scanner_scope_add_symbol     (JsonScanner *scanner,
                                                guint        scope_id,
                                                const gchar *symbol,
                                                gpointer     value);
G_GNUC_INTERNAL
void         json_scanner_unexp_token          (JsonScanner *scanner,
                                                GTokenType   expected_token,
                                                const gchar *identifier_spec,
                                                const gchar *symbol_spec,
                                                const gchar *symbol_name,
                                                const gchar *message);
G_GNUC_INTERNAL
void         json_scanner_set_msg_handler      (JsonScanner        *scanner,
                                                JsonScannerMsgFunc  msg_handler,
                                                gpointer            user_data);
G_GNUC_INTERNAL
void         json_scanner_error                (JsonScanner *scanner,
                                                const gchar *format,
                                                ...) G_GNUC_PRINTF (2,3);

G_GNUC_INTERNAL
gint64       json_scanner_get_int64_value      (const JsonScanner *scanner);
G_GNUC_INTERNAL
double       json_scanner_get_float_value      (const JsonScanner *scanner);
G_GNUC_INTERNAL
const char * json_scanner_get_string_value     (const JsonScanner *scanner);
G_GNUC_INTERNAL
char *       json_scanner_dup_string_value     (const JsonScanner *scanner);
G_GNUC_INTERNAL
const char * json_scanner_get_identifier       (const JsonScanner *scanner);
G_GNUC_INTERNAL
char *       json_scanner_dup_identifier       (const JsonScanner *scanner);

G_GNUC_INTERNAL
guint        json_scanner_get_current_line     (const JsonScanner *scanner);
G_GNUC_INTERNAL
guint        json_scanner_get_current_position (const JsonScanner *scanner);
G_GNUC_INTERNAL
GTokenType   json_scanner_get_current_token    (const JsonScanner *scanner);
G_GNUC_INTERNAL
guint        json_scanner_get_current_scope_id (const JsonScanner *scanner);

G_END_DECLS

#endif /* __JSON_SCANNER_H__ */
