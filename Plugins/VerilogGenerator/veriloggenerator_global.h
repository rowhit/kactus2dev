
#ifndef VERILOGGENERATOR_GLOBAL_H
#define VERILOGGENERATOR_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef VERILOGGENERATOR_LIB
# define VERILOGGENERATOR_EXPORT Q_DECL_EXPORT
#else
# define VERILOGGENERATOR_EXPORT Q_DECL_IMPORT
#endif

#endif // ALTERABSPGENERATOR_GLOBAL_H