import xbmc

# Check if the movie is playing fullscreen.
if  xbmc.getCondVisibility("!VideoPlayer.IsFullscreen + Player.Paused"):
    xbmc.executebuiltin("Action(Fullscreen)")

xbmc.executebuiltin("Action(Play)")
