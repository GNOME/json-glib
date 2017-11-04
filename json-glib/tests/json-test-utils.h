#include <string.h>
#include <math.h>
#include <float.h>
#include <glib.h>
#include <json-glib/json-glib.h>

#define json_fuzzy_equals(n1,n2,epsilon) \
  (((n1) > (n2) ? ((n1) - (n2)) : ((n2) - (n1))) < (epsilon))

#define json_assert_fuzzy_equals(n1,n2,epsilon) \
  G_STMT_START { \
    double __n1 = (n1), __n2 = (n2), __epsilon = (epsilon); \
    if (json_fuzzy_equals (__n1, __n2, __epsilon)) ; else { \
      g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                  #n1 " == " #n2 " (+/- " #epsilon ")", \
                                  __n1, "==", __n2, 'f'); \
    } \
  } G_STMT_END

#define json_assert_almost_equals(n1,n2) \
  json_assert_fuzzy_equals (n1, n2, DBL_EPSILON)
