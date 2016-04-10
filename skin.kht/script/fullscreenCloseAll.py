import xbmc

# Switch fullscreen, and close all modal dialogs
xbmc.executebuiltin("Dialog.Close(musicinformation,false)")
xbmc.executebuiltin("Dialog.Close(contextmenu,false)")
#xbmc.executebuiltin("Dialog.Close(all,false)")
xbmc.executebuiltin("Action(fullscreen)")
