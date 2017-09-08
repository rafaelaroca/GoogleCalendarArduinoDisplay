#!/bin/sh

echo "`date` Atualizando ical Rafael..."
lynx -dump "https://calendar.google.com/calendar/ical/pdkdn7emaouc2qts9hauusfkgo%40group.calendar.google.com/private-5a175bcea70ebdb618b8555f803bd100/basic.ics" > /tmp/rafael-ufscar.ics

echo "`date` Atualizando ical LEA"
lynx -dump "https://calendar.google.com/calendar/ical/17peiedo0btslesv2mhbkml69k%40group.calendar.google.com/public/basic.ics" > /tmp/demec-lea.ics

#echo "`date` Sincronizando relogio..."
#ntpdate pool.ntp.org
