#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <obfuscate.h>
#include <string>
#include <wx/wx.h>

inline constexpr unsigned long long API_ID = TDAPI_ID;
inline auto& API_HASH = AY_OBFUSCATE(TDAPI_HASH);
inline const wxString IPC_SERVICE_PORT = "5381";

#endif
