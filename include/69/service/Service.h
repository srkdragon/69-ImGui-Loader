#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace service
{

// Data Structures
struct SoftwareItem
{
    std::string Name;
    int IconIndex;
    int ID;
};

// Interfaces
class IService
{
  public:
    virtual ~IService() = default;
    virtual bool ValidateUser(const std::string& licenseKey) = 0;
    virtual std::vector<SoftwareItem> GetAvailableSoftware() = 0;
    virtual bool LaunchSoftware(int id) = 0;
};

} // namespace service
