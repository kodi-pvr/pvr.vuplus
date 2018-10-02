#include "EpgEntryExtractor.h"
#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"

using namespace vuplus;
using namespace ADDON;

EpgEntryExtractor::EpgEntryExtractor()
{
   extractors.emplace_back(new GenreExtractor());
   extractors.emplace_back(new ShowInfoExtractor());
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