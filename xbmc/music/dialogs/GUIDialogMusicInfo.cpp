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

#include "GUIDialogMusicInfo.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "dialogs/GUIDialogSelect.h"
#include "GUIPassword.h"
#include "GUIUserMessages.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "filesystem/File.h"
#include "FileItem.h"
#include "profiles/ProfilesManager.h"
#include "storage/MediaManager.h"
#include "settings/MediaSourceSettings.h"
#include "input/Key.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/StringUtils.h"
#include "TextureCache.h"
#include "music/MusicThumbLoader.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "filesystem/Directory.h"
#include "utils/log.h"
#include "music/windows/GUIWindowMusicBase.h"

#include "GUIUserMessages.h"

using namespace XFILE;

#define CONTROL_BTN_REFRESH      6
#define CONTROL_USERRATING       7
#define CONTROL_BTN_GET_THUMB   10
#define  CONTROL_BTN_GET_FANART 12

#define CONTROL_LIST            50
#define CONTROL_LIST_DISCO      51
#define CONTROL_LIST_2          52

CGUIDialogMusicInfo::CGUIDialogMusicInfo(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_INFO, "DialogMusicInfo.xml")
    , m_albumItem(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
  m_loadType = KEEP_IN_MEMORY;
  m_startUserrating = -1;
  m_needsUpdate = false;
  m_bViewReview = false;
  m_hasUpdatedThumb = false; 
  m_bArtistInfo = false;
}

CGUIDialogMusicInfo::~CGUIDialogMusicInfo(void)
{
  delete m_albumSongs;
}

bool CGUIDialogMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_startUserrating != m_albumItem->GetMusicInfoTag()->GetUserrating())
      {
        CMusicDatabase db;
        if (db.Open())
        {
          m_needsUpdate = true;
          db.SetAlbumUserrating(m_albumItem->GetPath(), m_albumItem->GetMusicInfoTag()->GetUserrating());
          db.Close();
        }
      }

      CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(msg);
      CGUIMessage message_disco(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_DISCO);
      OnMessage(message_disco);
      CGUIMessage message_list2(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_2);
      OnMessage(message_list2);
      m_albumSongs->Clear();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bViewReview = true;
      m_bRefresh = false;
      Update();
      return true;
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_UPDATE)
      {
          // Catch notifications from the ListProvider, afer a directory is fetched
          Update();
          return true;
      }
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_USERRATING)
      {
        OnSetUserrating();
      }
      else if (iControl == CONTROL_BTN_REFRESH)
      {
        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_LIST || iControl == CONTROL_LIST_DISCO || iControl == CONTROL_LIST_2)
      {
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction || ACTION_MOUSE_LEFT_CLICK == iAction)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= (int)m_albumSongs->Size())
            break;
          CFileItemPtr pItem = m_albumSongs->Get(iItem);
          bool bResult       = false;

          if(m_bArtistInfo)
          {
            // when in artist view, show album details
            OnSearch(pItem.get());
            bResult = true;
          }
          else
          {
            if( pItem->GetMusicInfoTag()->GetDatabaseId() > 0 )
            {
              // when in album view, play the current song
              bResult = g_application.PlayFile(*pItem) == PLAYBACK_OK;

              if (pItem->m_lStartOffset == STARTOFFSET_RESUME)
                pItem->m_lStartOffset = 0;

              Close();
              // Also switch visualisation OR screensaver to fullscreen
              // Switch to fullscreen mode if we can
              g_application.OnAction(CAction(ACTION_SHOW_GUI));
            }
          }
          return bResult;
        }
      }
      else if (iControl == CONTROL_BTN_GET_FANART)
      {
        OnGetFanart();
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogMusicInfo::OnAction(const CAction &action)
{
  int userrating = m_albumItem->GetMusicInfoTag()->GetUserrating();
  if (action.GetID() == ACTION_INCREASE_RATING)
  {
    SetUserrating(userrating + 1);
    return true;
  }
  else if (action.GetID() == ACTION_DECREASE_RATING)
  {
    SetUserrating(userrating - 1);
    return true;
  }
  else if (action.GetID() == ACTION_SHOW_INFO)
  {
    Close();
    return true;
  }
  /* Navigate back to the Artist info */
  else if (action.GetID() == ACTION_PREVIOUS_MENU && !m_bArtistInfo)
  {
    CGUIWindowMusicBase * win_base = (CGUIWindowMusicBase *)g_windowManager.GetWindow(WINDOW_MUSIC_NAV);
    if(win_base && win_base->m_last_artist != NULL)
    {
        SetArtist(*(win_base->m_last_artist), win_base->m_last_artist->strPath);
        Update();
        return true;
    }
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogMusicInfo::SetAlbum(const CAlbum& album, const std::string &path)
{
  m_album = album;
  SetSongs(m_album.infoSongs, m_album.songs);
  *m_albumItem = CFileItem(path, true);
  m_albumItem->GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_album.GetAlbumArtist());
  m_albumItem->GetMusicInfoTag()->SetArtist(m_album.GetAlbumArtist());
  m_albumItem->GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating(m_album.fRating);
  m_albumItem->GetMusicInfoTag()->SetVotes(m_album.iVotes);
  m_albumItem->GetMusicInfoTag()->SetUserrating(m_album.iUserrating);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_album.genre);
  m_albumItem->GetMusicInfoTag()->SetDatabaseId(m_album.idAlbum, MediaTypeAlbum);
  CMusicDatabase::SetPropertiesFromAlbum(*m_albumItem,m_album);

  CMusicThumbLoader loader;
  loader.LoadItem(m_albumItem.get());

  // set the artist thumb, fanart
  if (!m_album.GetAlbumArtist().empty())
  {
    CMusicDatabase db;
    db.Open();
    std::map<std::string, std::string> artwork;
    if (db.GetArtistArtForItem(m_album.idAlbum, MediaTypeAlbum, artwork))
    {
      if (artwork.find("thumb") != artwork.end())
        m_albumItem->SetProperty("artistthumb", artwork["thumb"]);
      if (artwork.find("fanart") != artwork.end())
        m_albumItem->SetArt("fanart",artwork["fanart"]);
    }
  }
  m_startUserrating = m_album.iUserrating;
  m_hasUpdatedThumb = false;
  m_bArtistInfo = false;
  m_needsUpdate = false;
  m_albumSongs->SetContent("albums");
}

void CGUIDialogMusicInfo::SetArtist(const CArtist& artist, const std::string &path)
{
  m_artist = artist;
  SetDiscography();
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_artist.genre);
  m_albumItem->GetMusicInfoTag()->SetDatabaseId(m_artist.idArtist, MediaTypeArtist);
  CMusicDatabase::SetPropertiesFromArtist(*m_albumItem,m_artist);

  CMusicThumbLoader loader;
  loader.LoadItem(m_albumItem.get());

  m_hasUpdatedThumb = false;
  m_bArtistInfo = true;
  m_albumSongs->SetContent("artists");
}

void CGUIDialogMusicInfo::SetSongs(const VECSONGS &infoSongs, const VECSONGS &songs)
{
  CLog::Log(LOGDEBUG,"%s:::%s Set %d Songs in Album", __FILE__, __FUNCTION__,
      songs.size());
  m_albumSongs->Clear();

  std::set<unsigned int> songsToSkip;

  // Combine songs info from the local file DB and the fetched infoSongs
  // First add all songs from the local files
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItemPtr item(new CFileItem(song));

    CLog::Log(LOGDEBUG,"%s:::%s Song %s / %d ", __FILE__, __FUNCTION__, songs[i].strTitle.c_str(), songs[i].iTrack);

    // Add info from the infoSongs DB
    for (unsigned int j = 0; j < infoSongs.size(); j++)
    {
      if(songs[i].iTrack == infoSongs[j].iTrack)
         //StringUtils::EqualsNoCaseAlphaNumOnly(songs[i].strTitle, infoSongs[j].strTitle))
      {
        songsToSkip.insert(j);
        // Add duration
        // Replace track title?
      }
    }

    m_albumSongs->Add(item);
  }

  // Handle a weird case when the infosongs table does not
  // contain any entry for the album, and returns a single empty item
  if( !(infoSongs.size() == 1 && infoSongs[0].strTitle.empty()) )
  {
    // Add songs not listed by the local file DB
    for (unsigned int j = 0; j < infoSongs.size(); j++)
    {
      if(songsToSkip.find(j) == songsToSkip.end())
      {
        const CSong& song = infoSongs[j];
        CFileItemPtr item(new CFileItem(song));

        m_albumSongs->Add(item);
      }
    }
  }
  // Sort the end result by track number
  m_albumSongs->Sort(SortByTrackNumber, SortOrderAscending);
}

void CGUIDialogMusicInfo::SetDiscography() const
{
  m_albumSongs->Clear();
  CMusicDatabase database;
  database.Open();

  std::vector<int> albumsByArtist;
  std::set<int> albumsToSkip;
  database.GetAlbumsByArtist(m_artist.idArtist, albumsByArtist);

  // Handle a weird case when the discography table does not
  // contain any entry for the artist, and returns a single empty item
  if( !(m_artist.discography.size() == 1 && m_artist.discography[0].first.empty()) )
  {
  for (unsigned int i=0;i<m_artist.discography.size();++i)
  {
    CFileItemPtr item(new CFileItem(m_artist.discography[i].first));
    item->SetLabel2(m_artist.discography[i].second);

    int idAlbum = -1;
    for (std::vector<int>::const_iterator album = albumsByArtist.begin(); album != albumsByArtist.end(); ++album)
    {
        if (StringUtils::EqualsNoCaseAlphaNumOnly(database.GetAlbumById(*album), item->GetLabel()))
      {
        idAlbum = *album;
        item->GetMusicInfoTag()->SetDatabaseId(idAlbum, "album");
          // Mark this album as already in the list
          albumsToSkip.insert(idAlbum);
        break;
      }
    }

    if (idAlbum != -1) // we need this slight stupidity to get correct case for the album name
      item->SetArt("thumb", database.GetArtForItem(idAlbum, MediaTypeAlbum, "thumb"));
    else
      item->SetArt("thumb", "DefaultAlbumCover.png");

    m_albumSongs->Add(item);
  }
}
  // Add albums from the db in the discography
  for (std::vector<int>::const_iterator it_album = albumsByArtist.begin(); it_album != albumsByArtist.end(); ++it_album)
  {
    if(albumsToSkip.find(*it_album) == albumsToSkip.end())
    {
      CAlbum album;
      if (database.GetAlbum(*it_album, album))
      {
        CFileItemPtr item(new CFileItem(album.strAlbum));
        if(album.iYear != 0)
          item->SetLabel2(std::to_string(album.iYear));
        item->GetMusicInfoTag()->SetDatabaseId(*it_album, "album");
        item->SetArt("thumb", database.GetArtForItem(*it_album, MediaTypeAlbum, "thumb"));

        m_albumSongs->Add(item);
      }
    }
  }
}

void CGUIDialogMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_GET_FANART);
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);

    SetLabel(CONTROL_TEXTAREA, m_artist.strBiography);
#if 1
    // Manage the discography list with extra content
    CGUIMessage messageExtraContent(GUI_MSG_EXTRA_CONTENT_GET, GetID(), CONTROL_LIST_DISCO, 0, 0);
    OnMessage(messageExtraContent);
    std::vector< CGUIListItemPtr> * m_itemsExtra = (std::vector<CGUIListItemPtr>*)messageExtraContent.GetPointer();
    CGUIInfoLabel label_year = CGUIInfoLabel("$INFO[listItem.Year]", "0000", CONTROL_LIST_DISCO);

    if( m_itemsExtra != NULL && !m_itemsExtra->empty() )
    {
        for (int i = 0; i < (int)m_itemsExtra->size(); i++)
        {
            CGUIListItemPtr item = m_itemsExtra->at(i);
            CLog::Log(LOGDEBUG,"%s:::%s label in dialog is %s, %s", __FILE__, __FUNCTION__, 
                    item->GetLabel().c_str(),
                    label_year.GetItemLabel(item.get(), false).c_str());
        }

        CFileItemList* albumSongs_copy = new CFileItemList;
        albumSongs_copy->Clear();
        CFileItemList* albumSongs_end= new CFileItemList;
        albumSongs_end->Clear();

        // Compare the database content with extra content
        for (int i = 0; i < (int)m_albumSongs->Size(); i++)
        {
            CGUIInfoLabel label_year = CGUIInfoLabel("$INFO[listItem.Year]", "", CONTROL_LIST_DISCO);
            //CGUIListItemPtr item_db_current = m_albumSongs[i];
            CFileItemPtr item_db_current = m_albumSongs->Get(i);
            CLog::Log(LOGDEBUG,"%s:::%s item: %s, %s", __FILE__, __FUNCTION__, 
                    item_db_current->GetLabel().c_str(),
                    item_db_current->GetLabel2().c_str());

            // items from the db, mark item as 'playable', (fallback for isboolean() is false by default)
            //item_db_current->SetProperty("isplayable", true);
            // FIXME: Pas utile on peut utiliser listitem.dbid pour verifier si l'item est dans la db

            for (int j = 0; j < (int)m_itemsExtra->size(); j++)
            {
                CGUIListItemPtr item_extra_current = m_itemsExtra->at(j);

                if( StringUtils::EqualsNoCaseAlphaNumOnly(item_db_current->GetLabel(), item_extra_current->GetLabel()) )
                    //StringUtils::CompareNoCase(item_db_current->GetLabel2(), label_year.GetItemLabel(item_extra_current.get(), false)))
                {
                    // Item is both in the db and extra content, complete db item if usefull
                    if( item_db_current->GetLabel2().empty() )
                    {
                        // Missing date
                        item_db_current->SetLabel2(label_year.GetItemLabel(item_extra_current.get(), false));
                    }
                    if( !StringUtils::CompareNoCase("DefaultAlbumCover.png", item_db_current->GetArt("thumb")) ||
                        item_db_current->GetArt("thumb").empty() )
                    {
                        // Missing art
                        item_db_current->SetArt("thumb", item_extra_current->GetArt("thumb"));
                    }

                    // Mark the extra item as duplicate
                    item_extra_current->SetProperty("extra_duplicate", true);
                }
            }

            // Make sure the year field is correctly set 
            std::string label2 = item_db_current->GetLabel2();
            try
            {
              int albumYear = std::stoi(label2);
              item_db_current->GetMusicInfoTag()->SetYear(albumYear);
            }
            catch(...)
            {
            }

            //if(label2.size() > 0)
              //item_db_current->GetMusicInfoTag()->SetYear(std::stoi(label2));

            // Item is in the db are always added to the list, items not playable are added in the end list
            if( item_db_current->GetMusicInfoTag()->GetDatabaseId() > 0 )
                albumSongs_copy->Add(item_db_current);
            else
                albumSongs_end->Add(item_db_current);
        }
#if 1
        // Complete the list with items in extra content only
        for (int i = 0; i < (int)m_itemsExtra->size(); i++)
        {
            CGUIListItemPtr item_extra_current = m_itemsExtra->at(i);

            if( !item_extra_current->GetProperty("extra_duplicate").asBoolean() )
            {
                CLog::Log(LOGDEBUG,"%s:::%s adding: %s", __FILE__, __FUNCTION__, 
                        item_extra_current->GetLabel().c_str());
                albumSongs_end->Add(std::dynamic_pointer_cast<CFileItem>(item_extra_current));
            }
        }
#endif
        // Sort the album lists by year
        albumSongs_copy->Sort(SortByYear, SortOrderAscending);
        albumSongs_end->Sort(SortByYear, SortOrderAscending);

        // Append both lists
        m_albumSongs->Assign(*albumSongs_copy);
        m_albumSongs->Append(*albumSongs_end);
        delete albumSongs_end;
        delete albumSongs_copy;

        for (int i = 0; i < (int)m_albumSongs->Size(); i++)
        {
            CGUIListItemPtr item = m_albumSongs->Get(i);
            CLog::Log(LOGDEBUG,"%s:::%s label after is %s, %s, %s", __FILE__, __FUNCTION__, 
                    item->GetLabel().c_str(),
                    item->GetLabel2().c_str(),
                    item->GetArt("thumb").c_str());
        }
#endif
    }
    // Bind the content with the Control_List container 
    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_DISCO, 0, 0, m_albumSongs);
    OnMessage(message);
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_LIST_DISCO);
    OnMessage(msg);

      }
      else
      {
    SET_CONTROL_VISIBLE(CONTROL_USERRATING);
    SET_CONTROL_HIDDEN(CONTROL_BTN_GET_FANART);

    SetLabel(CONTROL_TEXTAREA, m_album.strReview);
    if(!m_album.strReview.empty())
    {
    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_LIST);
      OnMessage(msg);
    }
    else
    {
      CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST_2, 0, 0, m_albumSongs);
      OnMessage(message);
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), CONTROL_LIST_2);
      OnMessage(msg);
    }

  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, CProfilesManager::GetInstance().GetCurrentProfile().canWriteDatabases() || g_passwordManager.bMasterUser);
}

void CGUIDialogMusicInfo::SetLabel(int iControl, const std::string& strLabel)
{
  if (strLabel.empty())
  {
    SET_CONTROL_LABEL(iControl, 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIDialogMusicInfo::OnInitWindow()
{
  SET_CONTROL_LABEL(CONTROL_BTN_REFRESH, 184);
  SET_CONTROL_LABEL(CONTROL_USERRATING, 38023);
  SET_CONTROL_LABEL(CONTROL_BTN_GET_THUMB, 13405);
  SET_CONTROL_LABEL(CONTROL_BTN_GET_FANART, 20413);

  if (m_bArtistInfo)
    SET_CONTROL_HIDDEN(CONTROL_USERRATING);
  else
    SET_CONTROL_HIDDEN(CONTROL_BTN_GET_FANART);

  CGUIDialog::OnInitWindow();
}

void CGUIDialogMusicInfo::SetUserrating(int userrating) const
{
  if (userrating < 0) userrating = 0;
  if (userrating > 10) userrating = 10;
  if (userrating != m_albumItem->GetMusicInfoTag()->GetUserrating())
  {
    m_albumItem->GetMusicInfoTag()->SetUserrating(userrating);
    // send a message to all windows to tell them to update the fileitem (eg playlistplayer, media windows)
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_ITEM, 0, m_albumItem);
    g_windowManager.SendMessage(msg);
  }
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

// TODO: Currently no support for "embedded thumb" as there is no easy way to grab it
//       without sending a file that has this as it's album to this class
void CGUIDialogMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_albumItem->GetArt("thumb")))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetArt("thumb", m_albumItem->GetArt("thumb"));
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  std::vector<std::string> thumbs;
  if (m_bArtistInfo)
    m_artist.thumbURL.GetThumbURLs(thumbs);
  else
    m_album.thumbURL.GetThumbURLs(thumbs);

  for (unsigned int i = 0; i < thumbs.size(); ++i)
  {
    std::string strItemPath;
    strItemPath = StringUtils::Format("thumb://Remote%i", i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetArt("thumb", thumbs[i]);
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20015));
    
    // TODO: Do we need to clear the cached image?
    //    CTextureCache::GetInstance().ClearCachedImage(thumb);
    items.Add(item);
  }

  // local thumb
  std::string localThumb;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    std::string strArtistPath;
    if (database.GetArtistPath(m_artist.idArtist,strArtistPath))
      localThumb = URIUtils::AddFileToFolder(strArtistPath, "folder.jpg");
  }
  else
    localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb))
  {
    CFileItemPtr item(new CFileItem("thumb://Local", false));
    item->SetArt("thumb", localThumb);
    item->SetLabel(g_localizeStrings.Get(20017));
    items.Add(item);
  }
  else
  {
    CFileItemPtr item(new CFileItem("thumb://None", false));
    if (m_bArtistInfo)
      item->SetIconImage("DefaultArtist.png");
    else
      item->SetIconImage("DefaultAlbumCover.png");
    item->SetLabel(g_localizeStrings.Get(20018));
    items.Add(item);
  }

  std::string result;
  bool flip=false;
  VECSOURCES sources(*CMediaSourceSettings::GetInstance().GetSources("music"));
  AddItemPathToFileBrowserSources(sources, *m_albumItem);
  g_mediaManager.GetLocalDrives(sources);
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(1030), result, &flip))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  std::string newThumb;
  if (StringUtils::StartsWith(result, "thumb://Remote"))
  {
    int number = atoi(result.substr(14).c_str());
    newThumb = thumbs[number];
  }
  else if (result == "thumb://Local")
    newThumb = localThumb;
  else if (CFile::Exists(result))
    newThumb = result;
  else // none
    newThumb = "-"; // force local thumbs to be ignored

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_albumItem->GetMusicInfoTag()->GetDatabaseId(), m_albumItem->GetMusicInfoTag()->GetType(), "thumb", newThumb);
    db.Close();
  }

  m_albumItem->SetArt("thumb", newThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}


// Allow user to select a Fanart
void CGUIDialogMusicInfo::OnGetFanart()
{
  CFileItemList items;

  if (m_albumItem->HasArt("fanart"))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetArt("thumb", m_albumItem->GetArt("fanart"));
    itemCurrent->SetLabel(g_localizeStrings.Get(20440));
    items.Add(itemCurrent);
  }

  // Grab the thumbnails from the web
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    std::string strItemPath = StringUtils::Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    std::string thumb = m_artist.fanart.GetPreviewURL(i);
    item->SetArt("thumb", CTextureUtils::GetWrappedThumbURL(thumb));
    item->SetIconImage("DefaultPicture.png");
    item->SetLabel(g_localizeStrings.Get(20441));

    // TODO: Do we need to clear the cached image?
    //    CTextureCache::GetInstance().ClearCachedImage(thumb);
    items.Add(item);
  }

  // Grab a local thumb
  CMusicDatabase database;
  database.Open();
  std::string strArtistPath;
  database.GetArtistPath(m_artist.idArtist,strArtistPath);
  CFileItem item(strArtistPath,true);
  std::string strLocal = item.GetLocalFanart();
  if (!strLocal.empty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetArt("thumb", strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20438));

    // TODO: Do we need to clear the cached image?
    CTextureCache::GetInstance().ClearCachedImage(strLocal);
    items.Add(itemLocal);
  }
  else
  {
    CFileItemPtr itemNone(new CFileItem("fanart://None", false));
    itemNone->SetIconImage("DefaultArtist.png");
    itemNone->SetLabel(g_localizeStrings.Get(20439));
    items.Add(itemNone);
  }

  std::string result;
  VECSOURCES sources = *CMediaSourceSettings::GetInstance().GetSources("music");
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20437), result, &flip, 20445))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (StringUtils::EqualsNoCase(result, "fanart://Current"))
   return;

  if (StringUtils::EqualsNoCase(result, "fanart://Local"))
    result = strLocal;

  if (StringUtils::StartsWith(result, "fanart://Remote"))
  {
    int iFanart = atoi(result.substr(15).c_str());
    m_artist.fanart.SetPrimaryFanart(iFanart);
    result = m_artist.fanart.GetImageURL();
  }
  else if (StringUtils::EqualsNoCase(result, "fanart://None") || !CFile::Exists(result))
    result.clear();

  if (flip && !result.empty())
    result = CTextureUtils::GetWrappedImageURL(result, "", "flipped");

  // update thumb in the database
  CMusicDatabase db;
  if (db.Open())
  {
    db.SetArtForItem(m_albumItem->GetMusicInfoTag()->GetDatabaseId(), m_albumItem->GetMusicInfoTag()->GetType(), "fanart", result);
    db.Close();
  }

  m_albumItem->SetArt("fanart", result);
  m_hasUpdatedThumb = true;
  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}

void CGUIDialogMusicInfo::OnSearch(const CFileItem* pItem)
{
  CMusicDatabase database;
  database.Open();
  if (pItem->HasMusicInfoTag() &&
      pItem->GetMusicInfoTag()->GetDatabaseId() > 0)
  {
    CAlbum album;
    if (database.GetAlbum(pItem->GetMusicInfoTag()->GetDatabaseId(), album))
    {
      std::string strPath;
      database.GetAlbumPath(pItem->GetMusicInfoTag()->GetDatabaseId(), strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
// FIXME: if album is not in local db, fetch info using exttended info script
}

CFileItemPtr CGUIDialogMusicInfo::GetCurrentListItem(int offset)
{
  return m_albumItem;
}

void CGUIDialogMusicInfo::AddItemPathToFileBrowserSources(VECSOURCES &sources, const CFileItem &item)
{
  std::string itemDir;

  if (item.HasMusicInfoTag() && item.GetMusicInfoTag()->GetType() == MediaTypeSong)
    itemDir = URIUtils::GetParentPath(item.GetMusicInfoTag()->GetURL());
  else
    itemDir = item.GetPath();

  if (!itemDir.empty() && CDirectory::Exists(itemDir))
  {
    CMediaSource itemSource;
    itemSource.strName = g_localizeStrings.Get(36041);
    itemSource.strPath = itemDir;
    sources.push_back(itemSource);
  }
}

void CGUIDialogMusicInfo::OnSetUserrating() const
{
  CGUIDialogSelect *dialog = static_cast<CGUIDialogSelect *>(g_windowManager.GetWindow(WINDOW_DIALOG_SELECT));
  if (dialog)
  {
    // If we refresh and then try to set the rating there will be an items already here...
    dialog->Reset();

    dialog->SetHeading(CVariant{ 38023 });
    dialog->Add(g_localizeStrings.Get(38022));
    for (int i = 1; i <= 10; i++)
      dialog->Add(StringUtils::Format("%s: %i", g_localizeStrings.Get(563).c_str(), i));

    dialog->SetSelected(m_albumItem->GetMusicInfoTag()->GetUserrating());

    dialog->Open();

    int iItem = dialog->GetSelectedItem();
    if (iItem < 0)
      return;

    SetUserrating(iItem);
  }
}

void CGUIDialogMusicInfo::ShowFor(CFileItem item)
{
  auto window = static_cast<CGUIWindowMusicNav*>(g_windowManager.GetWindow(WINDOW_MUSIC_NAV));
  if (window)
    window->OnItemInfo(&item);
}