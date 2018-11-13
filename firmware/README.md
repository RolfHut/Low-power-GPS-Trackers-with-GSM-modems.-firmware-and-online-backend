# Low power GPS Trackers with GSM modems. firmware
The code provided here is written for the first generation of the Particle Electron Asset tracker, datasheets to be found [here](https://docs.particle.io/datasheets/cellular/electron-datasheet/) for the electron and [here](https://docs.particle.io/datasheets/accessories/legacy-accessories/#electron-asset-tracker) for the Asset Tracker Shield.

Although the electron allows for over-the-air updates, we strongly suggest using the DFU mode to program the device over USB. This saves on roaming data use and (thus) cost. See [here](https://docs.particle.io/tutorials/device-os/led/electron/#dfu-mode-device-firmware-upgrade-) for details on how to update the electron using DFU.

## Warning on potential brownout
There is a risk of the electron "browning out" when connected to a power source that can deliver the required voltage, but not amperage, to start up. In our case, this happened when connecting a small solar panel that was hard wired to the electron picked up a few rays of sun without either the battery of the USB power connected. This brownout can brick the device, making it un-usable. Always make sure the battery is connected before connecting a solar panel and make sure the battery does not run completly empty.
