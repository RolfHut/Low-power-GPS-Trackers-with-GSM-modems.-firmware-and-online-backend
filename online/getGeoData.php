<?php
/*
this file gets all the processed location data from the MySQL server and presents it as csv data.
 */


header('Content-type: text/plain');

//get MySQL credentials
include 'serverLogin.php';

//make mySQLi object
$mysqli = new mysqli($hostname, $username, $password, $database);

/* check connection */
if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit();
}


//construct and run query
$query = "SELECT * FROM ElectronGPSGeoData";
$dbquery = $mysqli->query($query);


//present data as csv
echo "DeviceID, timeStampe, lat, lon\r\n";
while($row =  $dbquery->fetch_assoc()) {
  echo $row['deviceID'].",".$row['timeStamp'].",".$row['lat'].",".$row['lon']."\r\n";
};

?>
