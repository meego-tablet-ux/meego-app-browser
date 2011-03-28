
#ifndef __browser_service_marshal_MARSHAL_H__
#define __browser_service_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:INT64,STRING,STRING,STRING (browser/qt/browser-service/marshal.list:1) */
extern void browser_service_marshal_VOID__INT64_STRING_STRING_STRING (GClosure     *closure,
                                                                      GValue       *return_value,
                                                                      guint         n_param_values,
                                                                      const GValue *param_values,
                                                                      gpointer      invocation_hint,
                                                                      gpointer      marshal_data);

/* VOID:INT64 (browser/qt/browser-service/marshal.list:2) */
extern void browser_service_marshal_VOID__INT64 (GClosure     *closure,
                                                 GValue       *return_value,
                                                 guint         n_param_values,
                                                 const GValue *param_values,
                                                 gpointer      invocation_hint,
                                                 gpointer      marshal_data);

/* VOID:STRING,UINT (browser/qt/browser-service/marshal.list:3) */
extern void browser_service_marshal_VOID__STRING_UINT (GClosure     *closure,
                                                       GValue       *return_value,
                                                       guint         n_param_values,
                                                       const GValue *param_values,
                                                       gpointer      invocation_hint,
                                                       gpointer      marshal_data);

G_END_DECLS

#endif /* __browser_service_marshal_MARSHAL_H__ */

