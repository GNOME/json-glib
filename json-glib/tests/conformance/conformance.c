// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024  Emmanuele Bassi

#include <stdbool.h>
#include <stdlib.h>
#include <glib.h>
#include <json-glib/json-glib.h>

static char *opt_prefix;
static char *opt_type;
static char *opt_input;

static GOptionEntry opt_entries[] = {
  { "prefix", 'p', 0, G_OPTION_ARG_STRING, &opt_prefix, "Test prefix", "PREFIX" },
  { "type", 't', 0, G_OPTION_ARG_STRING, &opt_type, "Test type", "TYPE" },
  { "input", 'f', 0, G_OPTION_ARG_FILENAME, &opt_input, "Input file", "FILE" },
  { NULL, 0, 0, 0, NULL, NULL, NULL },
};

typedef struct {
  char *filename;
  bool strict;
  bool xfail;
} Fixture;

static void
fixture_free (gpointer data)
{
  Fixture *f = data;

  g_free (f->filename);
  g_free (f);
}

static void
parse_file (gconstpointer data)
{
  const Fixture *f = data;

  GError *error = NULL;
  JsonParser *parser = json_parser_new ();
  json_parser_set_strict (parser, f->strict);
  json_parser_load_from_file (parser, f->filename, &error);

  if (f->xfail)
    {
      g_assert_nonnull (error);
      g_test_message ("Expected error: %s", error->message);
      g_error_free (error);
    }
  else
    g_assert_no_error (error);

  g_object_unref (parser);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  GError *error = NULL;
  GOptionContext *context = g_option_context_new (NULL);
  g_option_context_add_main_entries (context, opt_entries, NULL);
  g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (error != NULL)
    {
      g_printerr ("Invalid arguments: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  g_test_message ("type:   %s", opt_type);
  g_test_message ("prefix: %s", opt_prefix);
  g_test_message ("input:  %s", opt_input);

  char *input_data = NULL;
  g_file_get_contents (opt_input, &input_data, NULL, &error);
  if (error != NULL)
    {
      g_printerr ("Invalid file: %s", error->message);
      g_error_free (error);
      return EXIT_FAILURE;
    }

  char *dirname = g_path_get_dirname (opt_input);

  char **lines = g_strsplit (input_data, "\n", -1);
  for (unsigned int i = 0; lines[i] != NULL && lines[i][0] != 0; i++)
    {
      // Allow comments
      if (lines[i][0] == '#')
        continue;

      char **l = g_strsplit (lines[i], ",", 2);
      char *filename = g_build_filename (dirname, l[1], NULL);
      char *test_path = g_strconcat ("/conformance/", opt_type, "/", l[1], NULL);

      Fixture *f = g_new (Fixture, 1);
      f->filename = filename;
      f->strict = TRUE;
      f->xfail = g_strcmp0 (l[0], "P") != 0;

      g_test_message ("test: %s (file: %s)", test_path, filename);
      g_test_add_data_func_full (test_path, f, parse_file, fixture_free);

      g_free (test_path);

      if (g_strcmp0 (l[0], "S") == 0)
        {
          test_path = g_strconcat ("/conformance/", opt_type, "/", l[1], "/permissive", NULL);
          f = g_new (Fixture, 1);
          f->filename = g_strdup (filename);
          f->strict = FALSE;
          f->xfail = FALSE;

          g_test_message ("test: %s (file: %s)", test_path, filename);
          g_test_add_data_func_full (test_path, f, parse_file, fixture_free);

          g_free (test_path);
        }

      g_strfreev (l);
    }

  g_strfreev (lines);
  g_free (input_data);
  g_free (dirname);

  return g_test_run ();
}
