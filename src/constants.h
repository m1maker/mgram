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

inline constexpr unsigned long long API_ID = TDAPI_ID;
inline constexpr std::string API_HASH = STRINGIFY(TDAPI_HASH);
#endif
