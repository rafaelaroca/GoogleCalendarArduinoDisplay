<?php

/**
 * Show real time events from Google Calendar on an LCD Display connected to an Arduino
 *
 * Copyright (C) Rafael V. Aroca - UFSCar - aroca@ufscar.br
 *
 *
 * Based on the ICS parter Library by Martin Thoma <info@martin-thoma.de>
 * http://code.google.com/p/ics-parser/
 *
 * Library downloaded at: https://github.com/MartinThoma/ics-parser
 *
 * lynx -dump "https://calendar.google.com/calendar/ical/pdkdn7emaouc2qts9hauusfkgo%40group.calendar.google.com/private-5a175bcea70ebdb618b8555f803bd100/basic.ics" > /tmp/rafael_ufscar.ics
 *
 * Rafael: tive problema com eventos recorrentes. Solucao foi trocar biblioteca
 * https://github.com/rashnk/ics-parser
 * Modified version of https://github.com/MartinThoma/ics-parser/ to include event recurrence.
 *
 */
require 'class.iCalReader.php';

//10.7.5.10 -> Modulo abertura porta do laboratorio
//10.7.5.30 -> Display rafael 


//Para a resposta ser mais rapida, usa um arquivo ja baixado
//Para isso, usei o CRONTAB
/*

Entrada no crontab

* *     * * *   aroca   /home/aroca/update-calendar.sh 2>> /home/aroca/update-calendar.log >> /home/aroca/update-calendar.log

Script

#!/bin/sh

lynx -dump "https://calendar.google.com/calendar/ical/pdkdn7emaouc2qts9hauusfkgo%40group.calendar.google.com/private-5a175bcea70ebdb618b8555f803bd100/basic.ics" > /tmp/rafael-ufscar.ics


*/

$IP=$_SERVER['REMOTE_ADDR'];

$ical   = new ICal('/tmp/rafael-ufscar.ics');

//Baixa o calendario do google a cada requisicao
//Resposta mais lenta, mas nao precisa ficar atualizando o arquivo
//$ical   = new ICal('https://calendar.google.com/calendar/ical/pdkdn7emaouc2qts9hauusfkgo%40group.calendar.google.com/private-5a175bcea70ebdb618b8555f803bd100/basic.ics');

$dMap = array('Dom', 'Seg', 'Ter', 'Qua', 'Qui', 'Sex', 'Sab');

$events = $ical->events();
//$debug=1;
$debug=0;

if ($debug) {
	$date = $events[0]['DTSTART'];
	echo "The ical date: ";
	echo $date;
	echo "<br/>";

	echo "The Unix timestamp: ";
	echo $ical->iCalDateToUnixTimestamp($date);
	echo "<br/>";

	echo "The number of events: ";
	echo $ical->event_count;
	echo "<br/>";

	echo "The number of todos: ";
	echo $ical->todo_count;
	echo "<br/>";
	echo "<hr/><hr/>";
}

//Fuso horario - somar 3 horas sem horario de verao
$fuso=3600*3;

//Fuso horario - somar 2 horas com horario de verao
$fuso=3600*2;

$ts = time(); // could be any timestamp
$d=getdate($ts);

//Sempre vamos enviar data e hora, independente de ter envento ou nao
echo "|:,.";
//Envia o dia da semana - desativado por espaco na tela
//echo $dMap[$d['wday']] . " - ";

//Envia o dia do mes e mes
echo " " . $d['mday'] . "/" . $d['mon'] . " - ";

//Envia a hora atual
echo date('H:i', $ts) . "   \n";

//10.7.5.10 -> Modulo abertura porta do laboratorio
//10.7.5.30 -> Display rafael 
//Se for o IP do dispositivo do Rafael, mostra agenda do Rafael
if ($IP == "10.7.5.10") {
	//Senao vamos mostrar outra coisa
	echo "DEMec - UFSCar_";
	echo "]]]]";
	exit();
}

//Se chegou aqui eh porque é para o modulo com agenda do Rafael

foreach ($events as $event) {

	//Trata eventos UTC
	$tz_utc = substr($event['DTSTART'], -1);
	//echo "\r\nTZ UTC = " . $tz_utc . "\r\n";
	
	if ($tz_utc == 'Z') {
		//echo " (UTC) ";
		$inicio=$ical->iCalDateToUnixTimestamp($event['DTSTART'])-$fuso;
		$fim=$ical->iCalDateToUnixTimestamp($event['DTEND'])-$fuso;
	}
	else {
		//echo " (Not UTC) ";
		$inicio=$ical->iCalDateToUnixTimestamp($event['DTSTART']);
		$fim=$ical->iCalDateToUnixTimestamp($event['DTEND']);
	}
	$uts=time();


	//Jeito antigo, sem tratar UTC
	//$inicio=$ical->iCalDateToUnixTimestamp($event['DTSTART'])-$fuso;
	//$inicio=$ical->iCalDateToUnixTimestamp($event['DTSTART']);
	//$fim=$ical->iCalDateToUnixTimestamp($event['DTEND'])-$fuso;
	//$fim=$ical->iCalDateToUnixTimestamp($event['DTEND']);
	//$uts=time();


		
	if (($uts >= $inicio) && ($uts <= $fim) ) {

//$debug=1;
		if ($debug) {
			echo "<pre>";
			echo "Inicio: $inicio (" . date('m/d/Y H:i:s', $inicio) . " ) \r\n";
			echo "   Fim: $fim (" . date('m/d/Y H:i:s', $fim) . " ) \r\n";

			echo " Atual: $uts (" . date('m/d/Y H:i:s', $uts) . " ) \r\n";

			echo "</pre>";

			echo "SUMMARY: ".$event['SUMMARY']."<br/>";
			echo "DTSTART: ".$event['DTSTART']." - UNIX-Time: ".$ical->iCalDateToUnixTimestamp($event['DTSTART'])."<br/>";
			echo "DTEND: ".$event['DTEND']."<br/>";
			echo "DTSTAMP: ".$event['DTSTAMP']."<br/>";
			echo "UID: ".$event['UID']."<br/>";
			echo "CREATED: ".$event['CREATED']."<br/>";
			echo "DESCRIPTION: ".$event['DESCRIPTION']."<br/>";
			echo "LAST-MODIFIED: ".$event['LAST-MODIFIED']."<br/>";
			echo "LOCATION: ".$event['LOCATION']."<br/>";
			echo "SEQUENCE: ".$event['SEQUENCE']."<br/>";
			echo "STATUS: ".$event['STATUS']."<br/>";
			echo "TRANSP: ".$event['TRANSP']."<br/>";



			echo "<hr/>";
		}

		//Envia a descricao do evento
		$x = $event['SUMMARY'];
		$l = strlen($x);
		echo $x;

		if ($l < 16) {
			for ($m = $l; $m<=17; $m=$m+1)
				echo "_";
		} 
		echo "\0";

	}
}

echo "]]]]";
?>
