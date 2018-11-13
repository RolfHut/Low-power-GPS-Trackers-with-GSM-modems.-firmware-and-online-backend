<?php
/*
this script dumps all raw data that was send to the MySQL server in a single csv output
 */


header('Content-type: text/plain');


//get the login credentials for the MySQL server
include 'serverLogin.php';

//make mySQLi object
$mysqli = new mysqli($hostname, $username, $password, $database);

/* check connection */
if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit();
}


//construct and run query
$query = "SELECT * FROM ElectronGPSRawData";
$dbquery = $mysqli->query($query);


//build a csv output row by row
echo "Received, DeviceID, rawData, rawType\r\n";
while($row =  $dbquery->fetch_assoc()) {
  echo $row['received'].",".$row['deviceID'].",".$row['rawData'].",".$row['rawType']."\r\n";
};

?>
