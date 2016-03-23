# the & symbol runs the app in the background  
./lpmtX &  
# wait for the window to open  
sleep 1  
# put the currently active window into fullscreen mode  
wmctrl -r :ACTIVE: -b add,fullscreen  
# wait for the app to die  
wait  
