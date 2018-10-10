#include "AutoTimer.h"


#include "inttypes.h"
#include "p8-platform/util/StringUtils.h"

using namespace enigma2::data;

bool AutoTimer::Like(const AutoTimer &right) const
{
  return m_backendId == right.m_backendId;;
}

bool AutoTimer::operator==(const AutoTimer &right) const
{
  bool bChanged = true;
  bChanged = bChanged && (!m_title.compare(right.m_title));
  bChanged = bChanged && (m_startTime == right.m_startTime); 
  bChanged = bChanged && (m_endTime == right.m_endTime); 
  bChanged = bChanged && (m_channelId == right.m_channelId); 
  bChanged = bChanged && (m_weekdays == right.m_weekdays); 

  bChanged = bChanged && (m_searchPhrase == right.m_searchPhrase); 
  bChanged = bChanged && (m_searchType == right.m_searchType); 
  bChanged = bChanged && (m_searchCase == right.m_searchCase); 
  bChanged = bChanged && (m_state == right.m_state); 
  bChanged = bChanged && (m_searchFulltext == right.m_searchFulltext); 
  bChanged = bChanged && (m_startAnyTime == right.m_startAnyTime); 
  bChanged = bChanged && (m_endAnyTime == right.m_endAnyTime); 
  bChanged = bChanged && (m_anyChannel == right.m_anyChannel); 
  bChanged = bChanged && (m_deDup == right.m_deDup); 

  return bChanged;
}

void AutoTimer::UpdateFrom(const AutoTimer &right)
{
  Timer::UpdateFrom(right);

  m_searchPhrase = right.m_searchPhrase;
  m_encoding = right.m_encoding;
  m_searchCase = right.m_searchCase;
  m_searchType = right.m_searchType;
  m_searchFulltext = right.m_searchFulltext;
  m_startAnyTime = right.m_startAnyTime;
  m_endAnyTime = right.m_endAnyTime;
  m_anyChannel = right.m_anyChannel;
  m_deDup = right.m_deDup;
}

void AutoTimer::UpdateTo(PVR_TIMER &left) const
{
  strncpy(left.strTitle, m_title.c_str(), sizeof(left.strTitle));
  //strncpy(tag.strDirectory, "/", sizeof(tag.strDirectory));   // unused
  //strncpy(tag.strSummary, timer.strPlot.c_str(), sizeof(tag.strSummary));
  strncpy(left.strEpgSearchString, m_searchPhrase.c_str(), sizeof(left.strEpgSearchString));
  left.iTimerType          = m_type;
  if (m_anyChannel)
    left.iClientChannelUid = PVR_TIMER_ANY_CHANNEL;
  else
    left.iClientChannelUid = m_channelId;
  left.startTime           = m_startTime;
  left.endTime             = m_endTime;
  left.state               = m_state;
  left.iPriority           = 0;     // unused
  left.iLifetime           = 0;     // unused
  left.firstDay            = 0;     // unused
  left.iWeekdays           = m_weekdays;
  //right.iEpgUid             = timer.iEpgID;
  left.iMarginStart        = 0;     // unused
  left.iMarginEnd          = 0;     // unused
  left.iGenreType          = 0;     // unused
  left.iGenreSubType       = 0;     // unused
  left.iClientIndex        = m_clientIndex;
  left.bStartAnyTime       = m_startAnyTime;
  left.bEndAnyTime         = m_endAnyTime;
  left.bFullTextEpgSearch  = m_searchFulltext;
  left.iPreventDuplicateEpisodes = m_deDup;
}
