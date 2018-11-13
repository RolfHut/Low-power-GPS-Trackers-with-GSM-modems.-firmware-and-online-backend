<?php
/*
This file contains the server information to connect to the MySQL. This is in a seperate file to make it easy to 
transfer this repo to a different server. If the MySQL server is on the same computer as the rest of the code, 
"localhost" can be used as hostname. 

*/

header('Content-type: text/plain');
$username = "UserName";
$password = "Password";
$hostname = "localhost";	
$database = "DataBaseName";
?>