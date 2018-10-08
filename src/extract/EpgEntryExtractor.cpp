#include "EpgEntryExtractor.h"

#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"

using namespace VUPLUS;
using namespace ADDON;

EpgEntryExtractor::EpgEntryExtractor(const VUPLUS::Settings &settings)
  : IExtractor(settings)
{
   m_extractors.emplace_back(new GenreExtractor(settings));
   m_extractors.emplace_back(new ShowInfoExtractor(settings));
}

EpgEntryExtractor::~EpgEntryExtractor(void)
{
}

void EpgEntryExtractor::ExtractFromEntry(VuBase &entry)
{
  for (auto& extractor : m_extractors)
  {
    extractor->ExtractFromEntry(entry);
  }  
}