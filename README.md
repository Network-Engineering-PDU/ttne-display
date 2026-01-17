# Network-Engineering-PDU Display

This application is designed to run on the 240x320 LCD screen of a PDU designed by Network Enigneering and Network-Engineering-PDU.

## Usage

1. Clone this repositore with recursive option:
    ```git clone --recursive git@github.com:Network-Engineering-PDU/ttne-display.git```
2. Go to the project folder and create a folder named ```build```
    ```cd ttne-display```
    ```mkdir build```
3. This application can be compilled in a simulator (your PC) or in the final device (VAR-SM-MX7) with the real screen.
    1. If you want to run it in your PC (for development): in the ```build``` folder
        ```cmake -DSIMULATOR_ENABLED=on ..```
        ```make -j```
    2. If you want to run it in the final device: in the ```build``` folder:
        ```cmake```
        ```make -j2```
        2.1. If the app is running from ```/usr/bin```, it is necessary to run cmake with this command:
            ```cmake -DASSET_PATH="\"A:/usr/share/cmdisplay/assets/\"" ..```
4. It is necessary to run the [Network-Engineering-PDU NE firmware API](https://github.com/Network-Engineering-PDU/ne-fw-api)
    ```ttnedaemon restart```
5. Run the application
    ```make run```
