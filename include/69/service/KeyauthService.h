#pragma once

#include "69/service/Service.h"

namespace service
{

class KeyauthService final : public IService
{
  public:
    KeyauthService() = default;
    bool ValidateUser(const std::string& licenseKey) override;
    std::vector<SoftwareItem> GetAvailableSoftware() override;
    bool LaunchSoftware(int id) override;
};

} // namespace service
