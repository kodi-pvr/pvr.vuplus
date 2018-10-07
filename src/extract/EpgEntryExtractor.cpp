#include "EpgEntryExtractor.h"

using namespace VUPLUS;
using namespace ADDON;

EpgEntryExtractor::EpgEntryExtractor(const VUPLUS::Settings &settings)
  : IExtractor(settings)
{
   extractors.emplace_back(new GenreExtractor(settings));
   extractors.emplace_back(new ShowInfoExtractor(settings));
}

EpgEntryExtractor::~EpgEntryExtractor(void)
{
}

void EpgEntryExtractor::ExtractFromEntry(VuBase &entry)
{
  for (auto& extractor : extractors)
  {
    extractor->ExtractFromEntry(entry);
  }  
}