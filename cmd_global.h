#ifndef CMD_GLOBAL_H
#define CMD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(CMD_LIBRARY)
#  define CMDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define CMDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // CMD_GLOBAL_H
