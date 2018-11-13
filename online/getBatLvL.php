<?php
/*
This script gets all the battery levels from the rawData on the MySQL server. This is usefull in the field to check of devices
have enouhg power left. It was also used to measure the power consumption for the article.
*/


//get MySQL details
include 'serverLogin.php';

//make mySQLi object
$mysqli = new mysqli($hostname, $username, $password, $database);

/* check connection */
if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit();
}

// construct and run query 
$deviceName = $_REQUEST['trackerName'];
$query = "SELECT * FROM ElectronGPSGeoData, ElectronDeviceIDName where (ElectronGPSGeoData.deviceID = ElectronDeviceIDName.deviceID AND ElectronDeviceIDName.name = '".$deviceName."') ORDER BY ElectronGPSGeoData.ID";
$dbquery =  $mysqli->query($query);

//return all data as csv
echo "data for tracker: ".$deviceName."\r\nTimestamp,lat,lon\r\n";
while($row =$dbquery->fetch_assoc()) {
  echo $row['timeStamp'].",".$row['lat'].",".$row['lon']."\r\n";
};

?>
