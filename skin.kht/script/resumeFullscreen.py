import xbmc

# Check if the movie is playing fullscreen.
#print xbmc.getInfoLabel("Visualisation.Preset") + '--' + xbmc.getInfoLabel("Visualisation.Name") 

#if  xbmc.getCondVisibility("!videoplayer.isfullscreen + player.paused"):
if  xbmc.getCondVisibility("!videoplayer.isfullscreen + player.paused") and xbmc.getInfoLabel("Visualisation.Preset") == "" :
    xbmc.executebuiltin("Dialog.Close(musicinformation,false)")
    xbmc.executebuiltin("Dialog.Close(contextmenu,false)")
    #xbmc.executebuiltin("Dialog.Close(all,false)")
    xbmc.executebuiltin("Action(fullscreen)")

# When visualisation is active and music is playing, exit fullscreen
if  xbmc.getCondVisibility("!player.paused") and xbmc.getInfoLabel("Visualisation.Preset") != "" :
    xbmc.executebuiltin("Action(fullscreen)")

xbmc.executebuiltin("Action(Pause)")
