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
private:
  GenreExtractor genreExtractor;
  ShowInfoExtractor showInfoExtractor;
  std::vector<std::unique_ptr<IExtractor>> extractors;
  
public:
  EpgEntryExtractor();
  ~EpgEntryExtractor(void);
  void ExtractFromEntry(VuBase &entry);
};

} // end vuplus