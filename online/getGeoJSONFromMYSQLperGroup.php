<?php
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

// json output - insert table name below after "FROM"
$deviceName = $_REQUEST['group'];
$query = "SELECT * FROM ElectronGPSGeoData, ElectronDeviceIDName where (ElectronGPSGeoData.deviceID = ElectronDeviceIDName.deviceID AND ElectronDeviceIDName.deviceGroup = '".$deviceName."') ORDER BY ElectronGPSGeoData.ID DESC LIMIT 1000";
$dbquery = $mysqli->query($query);


// Parse the dbquery into geojson 
// ================================================
// ================================================
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
        //Other fields here, end without a comma
            )
        );
    array_push($geojson['features'], $feature);
};

// // Return routing result
    header("Content-Type:application/json",true);
    echo json_encode($geojson);
?>