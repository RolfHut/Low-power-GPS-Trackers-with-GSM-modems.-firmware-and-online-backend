<?php
/*this php script logs in the MYSQL server that holds all the tracker data and 
  returns the last 30 measurements of the tracker "trackername" that is given as
  input in the URL. The response is in geoJson format. 

  The MYSQL server should have the following tables and fields:
  ElectronGPSGeoData
    ID           an incrementing field to identify the measurement
    timestamp    the timestamp (ddmmyyHHMMSS)
    lat          latitude (decimal)
    lng          longitude (decimal)
    deviceID     24 character hex string that identifies the specific electron.
  ElectronDeviceIDName
    ID           an incrementing field to identify the measurement
    deviceID     24 character hex string that identifies the specific electron.
    name         the common name for the device
    group        the group that the device belongs to. Not important in this script.
 */

header('Content-type: text/plain');

/*a file with the server credentials. Change the content of this file to match
  the specifics of the server being used.*/
include 'serverLogin.php';

//make mySQLi object
$mysqli = new mysqli($hostname, $username, $password, $database);

/* check connection */
if ($mysqli->connect_errno) {
    printf("Connect failed: %s\n", $mysqli->connect_error);
    exit();
}

// get deviceName from the request URL and run the query.
$deviceName = $_REQUEST['trackerName'];
$query = "SELECT * FROM ElectronGPSGeoData, ElectronDeviceIDName where (ElectronGPSGeoData.deviceID = ElectronDeviceIDName.deviceID AND ElectronDeviceIDName.name = '".$deviceName."') ORDER BY ElectronGPSGeoData.ID DESC LIMIT 30 ";
$dbquery = $mysqli->query($query);


// Parse the dbquery into geojson 
// Return markers as GeoJSON
$geojson = array(
		 'type'      => 'FeatureCollection',
		 'features'  => array()
		 );


while($row = $dbquery->fetch_assoc()) {
  $feature = array(
		   'type' => 'Feature', 
		   'geometry' => array(
				       'type' => 'Point',
				       'coordinates' => array((float)$row['lon'], (float)$row['lat'])
				       ),
		   'properties' => array(
					 'name' => $row['name'] ,
					 'timeStamp' => $row['timeStamp'] 
					 )
		   );
  array_push($geojson['features'], $feature);
};

// Return result as geojson
header("Content-Type:application/json",true);
echo json_encode($geojson);
?>