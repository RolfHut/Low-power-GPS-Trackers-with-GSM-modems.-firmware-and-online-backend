# Low power GPS Trackers with GSM modems. Online backend
The files in this folder work together with a MySQL database and two webhooks on the Particle Cloud. The webhooks subscribe to the "location" and "batLvL" topics, which are published by the electrons. The webhooks POST the measurement data from the devices to the uploadData.php file. This file inserts the data in the MySQL database. The other files are for viewing and downloading the data from the database.
## Usage
Once setup (see below), users can open the file `singleTracker.html` in a browser or on a mobile device and follow the latest 30 measurements of each of the devices in the database. This allows for real time tracking of these devices.

By opening the `getRawData.php` file, all data in the database is dumped on screen in a csv format. This can be stored locally and used for further analyses of the data.

## Webhook setup
webhooks are created on the console of the Particle Cloud, at [console.particle.io/integrations/webhook](https://console.particle.io/integrations/webhooks)
the fields to be filled in are:

- Event Name, this is either `location`, of `batLvL`
- Full URL, this is the location of the uploadData.php file.
- Request Type, this is `POST`
- Request Format, this is `JSON`
- JSON, here the following block of JSON code needs to be submitted: 

```
{
  "event": "{{{PARTICLE_EVENT_NAME}}}",
  "data": "{{{PARTICLE_EVENT_VALUE}}}",
  "coreid": "{{{PARTICLE_DEVICE_ID}}}",
  "published_at": "{{{PARTICLE_PUBLISHED_AT}}}",
  "rawData": "{{PARTICLE_EVENT_VALUE}}",
  "rawType": "{{PARTICLE_EVENT_NAME}}",
  "deviceID": "{{PARTICLE_DEVICE_ID}}"
} 
```
- Header, here the following header needs to be submitted:

```
{
  "Content-Type": "application/json",
  "Accept": "application/json"
}
```

- enforce SSL, has to be Yes.
  
If this is setup correctly, messages from the electrons with the topic `location` or `batLvL` will be forwarded to the uploadData.php file.

## Database setup
the MySQL database has to have three tables:

- ElectronGPSRawData with the fields deviceID, rawType and rawData
- ElectronGPSGeoData with the fields deviceID, timeStamp, lat and lon
- ElectronDeviceIDName with the fields deviceID, name and group

This last table has to be filled by hand coupling the device IDs from the different electrons to names you like to give them. These names are used in the pulldown in `singleTracker.html`, so those must match. (a future version might pull the names list from the database).

## Future work
This back end was build by a geoscientist without formal training as a web developer and the code most likely shows this. If we ever return to this code for a follow-up research, I intend to wrap it in a container so anyone can run it without the hassle of installing it as I did.
