<?php

/*
This file receives a POST from the Particle Cloud with a JSON message containing the measurement data.
It will upload this data into the MySQL database, both as raw data and as processed geo-data.
*/

//this makes sure we can accept POST statements
ini_set("allow_url_fopen", true);

//Make sure that it is a POST request.
if(strcasecmp($_SERVER['REQUEST_METHOD'], 'POST') != 0){
    throw new Exception('Request method must be POST!');
}
 
//Make sure that the content type of the POST request has been set to application/json
$contentType = isset($_SERVER["CONTENT_TYPE"]) ? trim($_SERVER["CONTENT_TYPE"]) : '';
if(strcasecmp($contentType, 'application/json') != 0){
    throw new Exception('Content type must be: application/json');
}

//decode the JSON string received in the post into a json object for php.
$json = trim(file_get_contents('php://input'));
$obj = json_decode($json, true);

//relate json names to database names
$deviceID = $obj['deviceID'];
$rawData = $obj['rawData'];
$rawType = $obj['rawType'];


//connact to the database, the serverLogin.php file contains the server credentials
include 'serverLogin.php';
$con=mysqli_connect($hostname,$username,$password,$database);

// Check connection
if (mysqli_connect_errno()) {
  echo "Failed to connect to MySQL: " . mysqli_connect_error();
}

//insert the raw data into the propper mysql table
$sql = "INSERT INTO ElectronGPSRawData (deviceID, rawType, rawData)
VALUES('$deviceID', '$rawData', '$rawType')";

//kill with an error if insert went wrong, otherwise continue
if (!mysqli_query($con,$sql)) {
  die('Error: ' . mysqli_error($con));
}
echo "1 record added to rawData";

//if the data is location (and not battery level), also add the data to the location table in the database
if ($rawType == "location"){

  //the rawdata can hold up to 4 measurements. Each one is timestamp, lat , lon.
  //this piece of code parses the rawData into an array: valuesArr
  $data = str_getcsv($rawData);
  $valuesArr = array();
  for ($n=0; $n < floor(count($data)/3);$n++){
      $timeStamp=$data[$n * 3];
      (float) $lat=(float) $data[($n * 3) + 1];
      (float) $lon=(float) $data[($n * 3) + 2];
      

      $valuesArr[]="('$deviceID', '$timeStamp', '$lat', '$lon' )";
  }

  //construct the query by imploding the values array 
  $sql = "INSERT INTO ElectronGPSGeoData (deviceID, timeStamp, lat, lon)
     VALUES ";
  $sql .= implode(',', $valuesArr);

  //kill with an error if the insertion failed
  if (!mysqli_query($con,$sql)) {
    die('Error: ' . mysqli_error($con));
  }
  echo "1 record added to geoData";
}	 



mysqli_close($con);
?>
