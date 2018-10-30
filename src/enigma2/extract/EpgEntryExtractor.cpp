#include "EpgEntryExtractor.h"

#include "GenreIdMapper.h"
#include "GenreRytecTextMapper.h"
#include "ShowInfoExtractor.h"
#include "../utilities/FileUtils.h"

using namespace enigma2;
using namespace enigma2::data;
using namespace enigma2::extract;
using namespace enigma2::utilities;

EpgEntryExtractor::EpgEntryExtractor()
  : IExtractor()
{
  FileUtils::CopyDirectory(FileUtils::GetResourceDataPath() + GENRE_DIR, GENRE_ADDON_DATA_BASE_DIR, true);
  
  m_extractors.emplace_back(new GenreIdMapper());
  m_extractors.emplace_back(new GenreRytecTextMapper());
  m_extractors.emplace_back(new ShowInfoExtractor());

  m_anyExtractorEnabled = false;
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      m_anyExtractorEnabled = true;
  }  
}

EpgEntryExtractor::~EpgEntryExtractor(void)
{
}

void EpgEntryExtractor::ExtractFromEntry(BaseEntry &entry)
{
  for (auto& extractor : m_extractors)
  {
    if (extractor->IsEnabled())
      extractor->ExtractFromEntry(entry);
  }  
}

bool EpgEntryExtractor::IsEnabled()
{
  return m_anyExtractorEnabled;
}