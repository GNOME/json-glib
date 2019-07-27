/* json-pointer.h - JSONPointer implementation
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

#ifndef __JSON_POINTER_H__
#define __JSON_POINTER_H__

#if !defined(__JSON_GLIB_INSIDE__) && !defined(JSON_COMPILATION)
#error "Only <json-glib/json-glib.h> can be included directly."
#endif

#include <json-glib/json-types.h>

G_BEGIN_DECLS

#define JSON_TYPE_POINTER          (json_pointer_get_type ())
#define JSON_POINTER(obj)          (G_TYPE_CHECK_INSTANCE_CAST ((obj), JSON_TYPE_POINTER, JsonPointer))
#define JSON_IS_POINTER(obj)       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), JSON_TYPE_POINTER))

/**
 * JSON_POINTER_ERROR:
 *
 * Error domain for #JsonPointer errors
 *
 * Since: 1.6
 */
#define JSON_POINTER_ERROR         (json_pointer_error_quark ())

/**
 * JsonPointerError:
 * @JSON_POINTER_ERROR_SYNTAX_ERROR: Syntax error in JSON Pointer expression
 *
 * Error code enumeration for the %JSON_POINTER_ERROR domain.
 *
 * Since: 1.6
 */
typedef enum {
  JSON_POINTER_ERROR_SYNTAX_ERROR
} JsonPointerError;

/**
 * JsonPointer:
 *
 * The `JsonPointer` structure is an opaque object whose members cannot be
 * directly accessed except through the provided API.
 *
 * Since: 1.6
 */
typedef struct _JsonPointer        JsonPointer;

/**
 * JsonPointerClass:
 *
 * The `JsonPointerClass` structure is an opaque object class whose members
 * cannot be directly accessed.
 *
 * Since: 1.6
 */
typedef struct _JsonPointerClass   JsonPointerClass;

JSON_AVAILABLE_IN_1_6
GType              json_pointer_get_type      (void) G_GNUC_CONST;
JSON_AVAILABLE_IN_1_6
GQuark             json_pointer_error_quark   (void);

JSON_AVAILABLE_IN_1_6
JsonPointer *      json_pointer_new           (void);

JSON_AVAILABLE_IN_1_6
gboolean           json_pointer_compile       (JsonPointer    *pointer,
                                               const char     *expression,
                                               GError        **error);
JSON_AVAILABLE_IN_1_6
JsonNode *         json_pointer_match         (JsonPointer    *pointer,
                                               JsonNode       *root);

JSON_AVAILABLE_IN_1_6
JsonNode *         json_pointer_query         (const char  *expression,
                                               JsonNode    *root,
                                               GError     **error);

#ifdef G_DEFINE_AUTOPTR_CLEANUP_FUNC
G_DEFINE_AUTOPTR_CLEANUP_FUNC (JsonPointer, g_object_unref)
#endif

G_END_DECLS

#endif /* __JSON_POINTER_H__ */
