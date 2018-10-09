#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IExtractor.h"

namespace enigma2
{
  namespace extract
  {
    class EpgEntryExtractor
      : public IExtractor
    {
    public:
      EpgEntryExtractor();
      ~EpgEntryExtractor(void);

      void ExtractFromEntry(enigma2::data::BaseEntry &entry);

    private:
      std::vector<std::unique_ptr<IExtractor>> m_extractors;  
    };
  } //namespace extract
} //namespace enigma2