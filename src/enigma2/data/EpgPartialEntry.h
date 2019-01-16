#pragma once

#include <string>

namespace enigma2
{
  namespace data
  {
    struct EpgPartialEntry
    {
      std::string title;
      std::string shortDescription;
      unsigned int epgUid;

      bool EntryFound() { return epgUid != 0; };
    };
  } //namespace data
} //namespace enigma2