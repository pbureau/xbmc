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
#include "MediaSource.h"
#include "FileItem.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "settings/AdvancedSettings.h"
#include "GUIInfoManager.h"
#include "guiinfo/GUIInfoLabels.h"

using namespace XFILE;

CPictureContentCheck::CPictureContentCheck() : CThread("PictureContentCheck")
{
  m_bRunning      = false;
  m_flags         = 0;
  m_bStop         = false;
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

int CPictureContentCheck::CountFilesRecursively(const std::string& strPath)
{
  int count = 0;

  if(m_bStop)
    return 0;

  /* load sub folder */
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items, g_advancedSettings.m_pictureExtensions, DIR_FLAG_NO_FILE_DIRS);
  /* count items */
  for (int i=0; i<items.Size(); ++i)
  {
    const CFileItemPtr pItem=items[i];
    
    if (pItem->m_bIsFolder)
      count += CountFilesRecursively(pItem->GetPath());
    else if (pItem->IsPicture())
    {
      count++;
      if(m_bStopAtFirst)
      {
        m_bStop = true;
        return count;
      }
    }
  }
  return count;
}

void CPictureContentCheck::Process()
{
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().Announce(ANNOUNCEMENT::AudioLibrary, "xbmc", "OnPictureContentCheckStarted");
  try
  {
    int count = 0;

    for (int i = 0; i < (int)m_sources->size() && !m_bStop; ++i)
    {
      CMediaSource share = m_sources->at(i);
      /* load subfolder & count items */
      count += CountFilesRecursively(share.strPath);
      CLog::Log(LOGDEBUG, "%s::%s : sources: %s - %d files",__FILE__,__FUNCTION__,share.strName.c_str(), count);
    }
    /* Set the library status */
    g_infoManager.SetLibraryBool(LIBRARY_HAS_PICTURES, (count>0)?true:false);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "PictureContentCheck: Exception while scanning.");
  }

  unsigned int tick = XbmcThreads::SystemClockMillis();
  tick = XbmcThreads::SystemClockMillis() - tick;
  CLog::Log(LOGNOTICE, "PictureContentCheck: Scanning for music info using worker thread, operation took %s", StringUtils::SecondsToTimeString(tick / 1000).c_str());
}

void CPictureContentCheck::Start(const VECSOURCES * sources, bool stopAtFirst, int flags)
{
  StopThread();
  m_flags = flags;

  /* keep reference to the source list */
  m_sources = sources;

  Create();
  m_bRunning     = true;
  m_bStop        = false;
  m_bStopAtFirst = stopAtFirst;
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
  m_bStop    = true;
  m_bRunning = false;
}
