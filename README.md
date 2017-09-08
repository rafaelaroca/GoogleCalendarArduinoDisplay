# GoogleCalendarArduinoDisplay

This set of files allows an Arduino to show in a LCD display, current events of a certain Google Agenda. I made this about one year ago (!), and this is installed on my door since then.

Linux crontab calls a script that downloads Google Calendar events from Google periodically and stores on a Linux Server filesystem. 

One (or more) Arduino(s) connected to the local network accesses index.php, which returns the current event of the selected calendar. 

Arduino sketch (.ino file) queries the index.php file over the network each minute, and updates the LCD display. 

Google Calendar must be configured to export a public URL of the desired Calendar.

![Screenshot](screenshot.png)



