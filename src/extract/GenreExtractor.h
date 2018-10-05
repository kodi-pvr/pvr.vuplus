#pragma once

#include "IExtractor.h"
#include "../client.h"

#include <map>
#include <regex>
#include <string>

#include "libXBMC_addon.h"

namespace VUPLUS
{
  static const std::string GENRE_PATTERN = "^\\[([a-zA-Z /]{3}[a-zA-Z ./]+)\\][^]*";
  static const std::string GENRE_MAJOR_PATTERN = "^([a-zA-Z /]{3,})\\.?.*";
  static const std::string GENRE_RESERVED_IGNORE = "reserved";

class GenreExtractor
  : public IExtractor
{
public:
  GenreExtractor(const VUPLUS::Settings &settings);
  ~GenreExtractor(void);

  void ExtractFromEntry(VuBase &base);

private:
  static int GetGenreTypeFromCombined(int combinedGenreType);
  static int GetGenreSubTypeFromCombined(int combinedGenreType);

  int GetGenreTypeFromText(const std::string &genreText, const std::string &showName);
  int LookupGenreValueInMaps(const std::string &genreText);

  std::regex genrePattern;
  std::regex genreMajorPattern;
  std::map<std::string, int> kodiGenreToKeyMap;
  const std::map<int, std::string> kodiKeyToGenreMap
  {
    {0x00, "Undefined"},
    //MOVIE/DRAMA
    {0x10, "Movie/Drama"},
    {0x11, "Detective/Thriller"},
    {0x12, "Adventure/Western/War"},
    {0x13, "Science Fiction/Fantasy/Horror"},
    {0x14, "Comedy"},
    {0x15, "Soap/Melodrama/Folkloric"},
    {0x16, "Romance"},
    {0x17, "Serious/Classical/Religious/Historical Movie/Drama"},
    {0x18, "Adult Movie/Drama"},
    //NEWS/CURRENT AFFAIRS
    {0x20, "News/Current Affairs"},
    {0x21, "News/Weather Report"},
    {0x22, "News Magazine"},
    {0x23, "Documentary"},
    {0x24, "Discussion/Interview/Debate"},
    //SHOW
    {0x30, "Show/Game Show"},
    {0x31, "Game Show/Quiz/Contest"},
    {0x32, "Variety Show"},
    {0x33, "Talk Show"},
    //SPORTS
    {0x40, "Sports"},
    {0x41, "Special Event"},
    {0x42, "Sport Magazine"},
    {0x43, "Football"},
    {0x44, "Tennis/Squash"},
    {0x45, "Team Sports"},
    {0x46, "Athletics"},
    {0x47, "Motor Sport"},
    {0x48, "Water Sport"},
    {0x49, "Winter Sports"},
    {0x4A, "Equestrian"},
    {0x4B, "Martial Sports"},
    //CHILDREN/YOUTH
    {0x50, "Children's/Youth Programmes"},
    {0x51, "Pre-school Children's Programmes"},
    {0x52, "Entertainment Programmes for 6 to 14"},
    {0x53, "Entertainment Programmes for 10 to 16"},
    {0x54, "Informational/Educational/School Programme"},
    {0x55, "Cartoons/Puppets"},
    //MUSIC/BALLET/DANCE
    {0x60, "Music/Ballet/Dance"},
    {0x61, "Rock/Pop"},
    {0x62, "Serious/Classical Music"},
    {0x63, "Folk/Traditional Music"},
    {0x64, "Musical/Opera"},
    {0x65, "Ballet"},
    //ARTS/CULTURE
    {0x70, "Arts/Culture"},
    {0x71, "Performing Arts"},
    {0x72, "Fine Arts"},
    {0x73, "Religion"},
    {0x74, "Popular Culture/Traditional Arts"},
    {0x75, "Literature"},
    {0x76, "Film/Cinema"},
    {0x77, "Experimental Film/Video"},
    {0x78, "Broadcasting/Press"},
    {0x79, "New Media"},
    {0x7A, "Arts/Culture Magazines"},
    {0x7B, "Fashion"},
    //SOCIAL/POLITICAL/ECONOMICS
    {0x80, "Social/Political/Economics"},
    {0x81, "Magazines/Reports/Documentary"},
    {0x82, "Economics/Social Advisory"},
    {0x83, "Remarkable People"},
    //EDUCATIONAL/SCIENCE
    {0x90, "Education/Science/Factual"},
    {0x91, "Nature/Animals/Environment"},
    {0x92, "Technology/Natural Sciences"},
    {0x93, "Medicine/Physiology/Psychology"},
    {0x94, "Foreign Countries/Expeditions"},
    {0x95, "Social/Spiritual Sciences"},
    {0x96, "Further Education"},
    {0x97, "Languages"},
    //LEISURE/HOBBIES
    {0xA0, "Leisure/Hobbies"},
    {0xA1, "Tourism/Travel"},
    {0xA2, "Handicraft"},
    {0xA3, "Motoring"},
    {0xA4, "Fitness & Health"},
    {0xA5, "Cooking"},
    {0xA6, "Advertisement/Shopping"},
    {0xA7, "Gardening"},
    //SPECIAL
    {0xB0, "Special Characteristics"},
    {0xB1, "Original Language"},
    {0xB2, "Black & White"},
    {0xB3, "Unpublished"},
    {0xB4, "Live Broadcast"},
    //USERDEFINED
    {0xF0, "Drama"},
    {0xF1, "Detective/Thriller"},
    {0xF2, "Adventure/Western/War"},
    {0xF3, "Science Fiction/Fantasy/Horror"},
    //---- below currently ignored by XBMC see http://trac.xbmc.org/ticket/13627
    {0xF4, "Comedy"},
    {0xF5, "Soap/Melodrama/Folkloric"},
    {0xF6, "Romance"},
    {0xF7, "Serious/ClassicalReligion/Historical"},
    {0xF8, "Adult"}
  };  
  
  const std::map<std::string, int> genreMap
  {
    //MOVIE/DRAMA
    {"General Movie/Drama", 0x10},
    {"Movie", 0x10},
    {"Film", 0x10},
    {"Animated Movie/Drama", 0x10},
    {"Thriller", 0x11},
    {"Detective/Thriller", 0x11},
    {"Action", 0x12},
    {"Adventure", 0x12},
    {"Adventure/War", 0x12},
    {"Western", 0x12},
    {"Gangster", 0x12},
    {"Fantasy", 0x13},
    {"Science Fiction", 0x13},
    {"Family", 0x14},
    {"Sitcom", 0x14},
    {"Comedy", 0x14},
    {"TV Drama. Comedy", 0x14},
    {"Drama", 0x15},
    {"Soap/Melodrama/Folkloric", 0x15},
    {"TV Drama. Melodrama", 0x15},
    {"TV Drama. Factual", 0x15},
    {"TV Drama", 0x15},
    {"TV Drama. Crime", 0x15},
    {"TV Drama. Period", 0x15},
    {"Romance", 0x16},
    {"Medical Drama", 0x15},
    {"Crime drama", 0x17},
    {"Historical/Period Drama", 0x17},
    {"Police/Crime Drama", 0x17},
    //NEWS/CURRENT AFFAIRS
    {"News", 0x20},
    {"General News/Current Affairs", 0x20},
    {"Documentary", 0x23},
    {"Documentary. News", 0x23},
    {"Discussion. News", 0x24},
    //SHOW
    {"Series", 0x30},
    {"Show", 0x30},
    {"Vets/Pets", 0x30},
    {"Wildlife", 0x30},
    {"Property", 0x30},
    {"General Show/Game Show", 0x31},
    {"Game Show", 0x31},
    {"Challenge/Reality Show", 0x31},
    {"Show. Variety Show", 0x32},
    {"Variety Show", 0x32},
    {"Entertainment", 0x32},
    {"Miscellaneous", 0x32},
    {"Talk Show", 0x33},
    {"Show. Talk Show", 0x33},
    //SPORTS
    {"Sport", 0x40},
    {"Live/Sport", 0x40},
    {"General Sports", 0x40},
    {"Football. Sports", 0x43},
    {"Martial Sports", 0x4B},
    {"Martial Sports. Sports", 0x4B},
    {"Wrestling", 0x4B},
    //CHILDREN/YOUTH
    {"Children", 0x50},
    {"Educational/Schools Programmes", 0x50},
    {"Animation", 0x55},
    {"Cartoons/Puppets", 0x55},
    //MUSIC/BALLET/DANCE
    {"Music", 0x60},
    {"General Music/Ballet/Dance", 0x60},
    {"Music. Folk", 0x63},
    {"Musical", 0x64},
    //ARTS/CULTURE
    {"General Arts/Culture", 0x70},
    {"Arts/Culture", 0x70},
    {"Arts/Culture. Fine Arts", 0x72},
    {"Religion", 0x73},
    //SOCIAL/POLITICAL/ECONOMICS
    {"Social/Political", 0x80},
    {"Social/Political. Famous People", 0x83},
    //EDUCATIONAL/SCIENCE
    {"Education", 0x90},
    {"Educational", 0x90},
    {"History", 0x90},
    {"Factual", 0x90},
    {"General Education/Science/Factual Topics", 0x90},
    {"Science", 0x90},
    {"Educational. Nature", 0x91},
    {"Environment", 0x91},
    {"Technology", 0x92},
    {"Computers/Internet/Gaming", 0x92},
    //LEISURE/HOBBIES
    {"Leisure", 0xA0},
    {"Leisure. Lifestyle", 0xA0},
    {"Travel", 0xA1},
    {"Health", 0xA4},
    {"Leisure. Health", 0xA4},
    {"Medicine/Health", 0xA4},
    {"Cookery", 0xA5},
    {"Leisure. Cooking", 0xA5},
    {"Leisure. Shopping", 0xA6},
    {"Advertisement/Shopping", 0xA6},
    {"Consumer", 0xA6},
    //SPECIAL
    //USERDEFINED
    {"Factual Crime", 0xF1},
  };

/*
Useful reference to Genres uses by Kodi PVR

EPG_EVENT_CONTENTMASK_UNDEFINED                0x00
EPG_EVENT_CONTENTMASK_MOVIEDRAMA               0x10
EPG_EVENT_CONTENTMASK_NEWSCURRENTAFFAIRS       0x20
EPG_EVENT_CONTENTMASK_SHOW                     0x30
EPG_EVENT_CONTENTMASK_SPORTS                   0x40
EPG_EVENT_CONTENTMASK_CHILDRENYOUTH            0x50
EPG_EVENT_CONTENTMASK_MUSICBALLETDANCE         0x60
EPG_EVENT_CONTENTMASK_ARTSCULTURE              0x70
EPG_EVENT_CONTENTMASK_SOCIALPOLITICALECONOMICS 0x80
EPG_EVENT_CONTENTMASK_EDUCATIONALSCIENCE       0x90
EPG_EVENT_CONTENTMASK_LEISUREHOBBIES           0xA0
EPG_EVENT_CONTENTMASK_SPECIAL                  0xB0
EPG_EVENT_CONTENTMASK_USERDEFINED              0xF0

EPG_GENRE_USE_STRING                           0x100
EPG_STRING_TOKEN_SEPARATOR ","
*/
};

} //namespace vuplus