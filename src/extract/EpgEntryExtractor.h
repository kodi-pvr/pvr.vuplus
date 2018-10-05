#pragma once

#include <string>

#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"
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
  std::vector<std::unique_ptr<IExtractor>> extractors;  
};

} //namespace VUPLUS