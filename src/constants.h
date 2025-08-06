#ifndef CONSTANTS_H
#define CONSTANTS_H
#ifndef STRINGIFY
#define STRINGIFY(x) #x
#endif

#ifndef TDAPI_ID
#define TDAPI_ID
#endif

#ifndef TDAPI_HASH
#define TDAPI_HASH
#endif

#include <string>
#include <obfuscate.h>

inline constexpr unsigned long long API_ID = TDAPI_ID;
inline auto& API_HASH = AY_OBFUSCATE(STRINGIFY(TDAPI_HASH));
#endif
