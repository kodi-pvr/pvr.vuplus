#include "EpgEntryExtractor.h"
#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"

using namespace vuplus;
using namespace ADDON;

EpgEntryExtractor::EpgEntryExtractor(const vuplus::Settings &settings) : IExtractor(settings)
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