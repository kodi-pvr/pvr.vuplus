#include "EpgEntryExtractor.h"

#include "GenreExtractor.h"
#include "ShowInfoExtractor.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;

EpgEntryExtractor::EpgEntryExtractor()
  : IExtractor()
{
   m_extractors.emplace_back(new GenreExtractor());
   m_extractors.emplace_back(new ShowInfoExtractor());
}

EpgEntryExtractor::~EpgEntryExtractor(void)
{
}

void EpgEntryExtractor::ExtractFromEntry(BaseEntry &entry)
{
  for (auto& extractor : m_extractors)
  {
    extractor->ExtractFromEntry(entry);
  }  
}