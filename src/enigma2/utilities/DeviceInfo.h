#pragma once

#include <regex>

namespace enigma2
{
  namespace utilities
  {
    class DeviceInfo
    {
    public:
      DeviceInfo() = default;
      DeviceInfo(const std::string& serverName, const std::string& enigmaVersion, const std::string& imageVersion, const std::string& distroName,
        const std::string& webIfVersion, unsigned int webIfVersionAsNum)
        : m_serverName(serverName), m_enigmaVersion(enigmaVersion), m_imageVersion(imageVersion), m_distroName(distroName),
          m_webIfVersion(webIfVersion), m_webIfVersionAsNum(webIfVersionAsNum) {};

      const std::string& GetServerName() const { return m_serverName; }
      const std::string& GetEnigmaVersion() const { return m_enigmaVersion; }
      const std::string& GetImageVersion() const { return m_imageVersion; }
      const std::string& GetDistroName() const { return m_distroName; }
      const std::string& GetWebIfVersion() const { return m_webIfVersion; }
      unsigned int GetWebIfVersionAsNum() const { return m_webIfVersionAsNum; }

    private:
      std::string m_serverName = "Enigma2";
      std::string m_enigmaVersion;
      std::string m_imageVersion;
      std::string m_distroName;
      std::string m_webIfVersion;
      unsigned int m_webIfVersionAsNum;
    };
  } //namespace utilities
} //namespace enigma2