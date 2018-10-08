#pragma once

#include <memory>
#include <string>
#include <vector>

#include "IExtractor.h"
#include "../client.h"

#include "libXBMC_addon.h"

namespace VUPLUS
{

class EpgEntryExtractor
  : public IExtractor
{
public:
  EpgEntryExtractor(const VUPLUS::Settings &settings);
  ~EpgEntryExtractor(void);

  void ExtractFromEntry(VuBase &entry);

private:
  std::vector<std::unique_ptr<IExtractor>> m_extractors;  
};

} //namespace VUPLUS