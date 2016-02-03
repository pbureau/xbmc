/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PictureContentCheck.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

CPictureContentCheck::CPictureContentCheck() : CThread("PictureContentCheck"), m_fileCountReader(this, "PictureFileCounter")
{
  m_bRunning      = false;
  m_flags         = 0;
/*
  m_showDialog    = false;
  m_handle        = NULL;
  m_bCanInterrupt = false;
  m_currentItem   = 0;
  m_itemCount     = 0;
*/
}

CPictureContentCheck::~CPictureContentCheck()
{
}

void CPictureContentCheck::Process()
{
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnScanStarted");
  try
  {
    unsigned int tick = XbmcThreads::SystemClockMillis();
    tick = XbmcThreads::SystemClockMillis() - tick;
    CLog::Log(LOGNOTICE, "My Music: Scanning for music info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "MusicInfoScanner: Exception while scanning.");
  }
}

void CPictureContentCheck::Start(const std::string& strDirectory, int flags)
{
  m_fileCountReader.StopThread();
  StopThread();
  m_flags = flags;

  Create();
  m_bRunning = true;
}

bool CPictureContentCheck::IsScanning()
{
  return m_bRunning;
}

void CPictureContentCheck::Stop()
{
  //if (m_bCanInterrupt)
    //m_musicDatabase.Interupt();

  StopThread(false);
}
