<?
/*
    Access point counter script checked
    Pawel Furtak '2006 'crashev' <pawelf@komunix.pl>
*/
pcntl_alarm ( 60*3 );
function database_update($query) {

  $conn_string = "host=localhost dbname=komunix_pawelf_apcenterd user=apcenterd password=ap.bunczuczny.$%^";
    if (! ($conn = pg_connect($conn_string)) ) {
              die("Blad polaczenia z baza danych");
      }
	      
        $res = pg_query($conn, $query);
	if (!$res) {
	echo "Error with query : $query \n";
	}
	pg_close($conn);
}


function get_clients($apname) {
   $fp = fsockopen("/home/crashev/serwer/admin/admin.sock", 0, $errno, $errstr, 10);
   if (!$fp) {
		echo "$errstr ($errno)<br />\n";
		die;
   } 

   $query="+SWITCH $apname\n";
   fwrite($fp, $query);
   $str=fgets($fp, 128);

   if (!strncmp($str,"=END\n",5)) 
   {
	fclose($fp);
	return 0;
   }

   $query="+REMOTE $apname +EXEC wl assoclist\n";
   fwrite($fp, $query);
           $str=fgets($fp, 128);
   $cli=Array();
   $i=1;
   $zapisuj=0;
   while (!feof($fp)) {
           $str=fgets($fp, 128);
	   $len=strlen($str);

	   $end="=END EXEC wl assoclist";
	   if (!strncmp($str, $end, strlen($end))) 
	   			break;
	   if ($zapisuj) {
		   list($assoc, $mac) = split(" ", $str, 2);
		   $mac = chop($mac);
		   $cli[$i++]=$mac;
	   }
	   $end="=START EXEC wl assoclist";
	   if (!strncmp($str, $end ,strlen($end)))
		   $zapisuj=1;
   }
   fclose($fp);
   return $cli;
}
    
   $fp = fsockopen("/home/crashev/serwer/admin/admin.sock", 0, $errno, $errstr, 10);
   if (!$fp) {
		echo "$errstr ($errno)<br />\n";
		die;
   } 

    /* GET LIST OF ACTIVE ACCESS POINTS */
       $out = "+STATUS\n";
       fwrite($fp, $out);
       $ap=Array();
       while (!feof($fp)) 
       {
           $str=fgets($fp, 128);
	   if (!strncmp($str,"=END\n",5)) 
	   			break;
	   list($apname, $apip) =  split("\t", $str, 4);
	   $ap[$apname]=$apip;
       }
       fclose($fp);
	sleep(1);

       foreach ($ap as $apn => $api) {
	if (strlen($apn)>3) 
	    {
        	echo "[??] Listing Clients for $apn ($api) -> ";
		$clients = get_clients($apn);
		echo "Klientow: ".count($clients)."\n";
		$query="UPDATE access_points set licznik='".count($clients)."' WHERE apname='$apn' and apip='$api'";
		echo "	QUERY: $query \n";
		database_update($query);
//		foreach ($clients as $cli) {
//		    echo "[-->] $cli \n";
//		}
		sleep(1);
	    }
       }

?>


