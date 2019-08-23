#pragma once
/*
 *      Copyright (C) 2005-2019 Team XBMC
 *      http://www.xbmc.org
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

namespace enigma2
{
  namespace utilities
  {
    struct Tuner
    {
      Tuner(int tunerNumber, const std::string& tunerName, const std::string& tunerModel)
        : m_tunerNumber(tunerNumber), m_tunerName(tunerName), m_tunerModel(tunerModel){};

      int m_tunerNumber;
      std::string m_tunerName;
      std::string m_tunerModel;
    };
  } //namespace utilities
} //namespace enigma2