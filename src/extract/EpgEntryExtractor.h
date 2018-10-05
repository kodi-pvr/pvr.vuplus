#pragma once

#include "../client.h"
#include "IExtractor.h"
#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"
#include <string>
#include "libXBMC_addon.h"

namespace vuplus
{

class EpgEntryExtractor
  : public IExtractor
{
public:
  EpgEntryExtractor(const vuplus::Settings &settings);
  ~EpgEntryExtractor(void);
  void ExtractFromEntry(VuBase &entry);

private:
  std::vector<std::unique_ptr<IExtractor>> extractors;  
};

} // end vuplus