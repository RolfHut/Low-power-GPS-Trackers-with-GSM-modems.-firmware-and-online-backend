# Low power GPS Trackers with GSM modems. firmware and online backend
This repo contains the code used in our experiment where we tracked drifters equipped with GSM enabled GPS trackers as they drifted down the Irrawaddy and Chindwin rivers in Myanmar. 

For the drifters we used Particle Electron Asset Trackers, but had to modify the exampel firmware significanly to get the devices to be low power enough for our purpose. In the firmware folder, the code that ran on the electrons can be found.

For the online backend we used a combination of php, javascript and a MySQL database. This allowed us to monitor the position of our trackers in real time. The readmy file in the online folder details how to setup the database and how to link the Particle Cloud to the database using a webhook.

We believe this code can be usefull for scientists who want to have a real time view on their data. Furthermore, the firmware for the electron is optimized for low power use, which is also beneficial for other experiments than our own.

## Usage and maintenance
This code is provided as is, under a GPL license. This experiment has ended for us and we do not activly maintain this code.

## Citation
If this code is used in a publication, or build upon, please cite this code as [to be included after first release on git]. The paper that we wrote about this work will be submitted to an academic journal shortly.
