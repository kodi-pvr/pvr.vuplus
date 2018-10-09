#pragma once 
/*
 *      Copyright (C) 2005-2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1335, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <string>
#include <vector>

#include "EPGEntry.h"

namespace enigma2
{
  namespace data
  {
    class ChannelGroup 
    {
    public:           
      const std::string& GetServiceReference() const { return m_serviceReference; }
      void SetServiceReference(const std::string& value ) { m_serviceReference = value; }          

      const std::string& GetGroupName() const { return m_groupName; }
      void SetGroupName(const std::string& value ) { m_groupName = value; }        

      int GetGroupState() const { return m_groupState; }
      void SetGroupState(int value) { m_groupState = value; }      

      std::vector<EPGEntry>& GetInitialEPG() { return m_initialEPG; }

    private:
      std::string m_serviceReference;
      std::string m_groupName;
      int m_groupState;
      std::vector<EPGEntry> m_initialEPG; 
    };
  } //namespace data
} //namespace enigma2