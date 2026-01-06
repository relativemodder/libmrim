#ifndef LIBMRIM_GLOBAL_H
#define LIBMRIM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(LIBMRIM_LIBRARY)
#define LIBMRIM_EXPORT Q_DECL_EXPORT
#else
#define LIBMRIM_EXPORT Q_DECL_IMPORT
#endif

#endif // LIBMRIM_GLOBAL_H
