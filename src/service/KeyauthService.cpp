#include "69/service/KeyauthService.h"

#include "keyauth/keyauth.hpp"
#include "obfuscate/obfuscate.h"

#include <Windows.h>

namespace service
{

bool KeyauthService::ValidateUser(const std::string& licenseKey)
{
    // TODO : Your licensing logic
    KeyAuth::API api(std::string(OBF("Surakarndragon's Application")),
                     std::string(OBF("mCoqYG4Adm")), std::string(OBF("1.0")),
                     std::string(OBF("https://keyauth.win/api/1.3/")));

    api.init();

    if (!api.response.success)
        return false;

    api.license(licenseKey);

    if (api.response.success && !api.user_data.username.empty() && !api.user_data.ip.empty() &&
        !api.user_data.hwid.empty())
        return true;

    return false;
}

std::vector<SoftwareItem> KeyauthService::GetAvailableSoftware()
{
    // TODO : Your available software
    return {{std::string(OBF("FPS Boost System")), 0, 0}};
}

bool KeyauthService::LaunchSoftware(int id)
{
    // TODO : Your launching software logic
    if (id == 0)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        wchar_t cmdLine[] = L"cmd.exe";
        if (!CreateProcessW(NULL, cmdLine, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
            return false;

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return true;
}

} // namespace service
