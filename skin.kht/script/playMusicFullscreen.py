import xbmc

# check the current path
path = xbmc.getInfoLabel("Container.FolderPath")
button1_focus = xbmc.getCondVisibility("Control.HasFocus(2201)")
button2_focus = xbmc.getCondVisibility("Control.HasFocus(2202)")
button3_focus = xbmc.getCondVisibility("Control.HasFocus(2203)")
#print "focus:" + str(button1_focus) + "-" + str(button2_focus) + "-" + str(button3_focus)
#print "path: " + path
if path == "musicdb://songs/" and button1_focus == 0 and button2_focus == 0 and button3_focus == 0 :
    xbmc.executebuiltin("Action(Play)")
    xbmc.executebuiltin("Action(Fullscreen)")
else :
    #print "Action Select"
    xbmc.executebuiltin("Action(Select)")
