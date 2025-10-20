# avt_vimba_camera (ROS1)

This repo contains a ROS driver for cameras manufactured by [Allied Vision Technologies](https://www.alliedvision.com).
The driver relies on libraries provided by AVT as part of their [Vimba SDK](https://www.alliedvision.com/en/products/software.html).

*See the ROS2 version of this README [here](https://github.com/astuff/avt_vimba_camera/blob/ros2_master/README.md).*

___
## This is the Pointlaz modified version
 Last modification 2023-04-07 by CASL and Antoine Gruet.

### How to update this version
Since the PointLaz modifications are contained on a branch, it is possible to isolate the modifications from the update.
- While working on this fork, start by checking out on the master branch
```
git checkout ros1_master
```
- Then sync the fork with the update source version by using the method of your choice (see https://docs.github.com/en/pull-requests/collaborating-with-pull-requests/working-with-forks/syncing-a-fork)
- Then checkout back to the Pointlaz branch and merge the updated master into the branch.
```
git checkout Pointlaz
git merge ros1_master
git push
```
- It's possible that the merge doesn't complete automatically depending on the complexity of Pointlaz's modifications. In this case, be prepared to do a manual merge. You can use the method of your choice, by example using CLION: check https://www.jetbrains.com/help/clion/resolving-conflicts.html#distributed-version-control-systems

## Running a debug node in CLION
https://www.jetbrains.com/help/clion/ros-setup-tutorial.html#example
run a roscore on a new terminal
```bash
roscore
```

on another terminal load params from yaml file
```bash
rosparam load /home/alex/third_party/Scanner/RosScan/Projects/avt_vimba_camera/launch/no_trigger_params.yaml
```

in clion run the node in debug mode

CTRL+ALT+5 and link the node ( Main | Run | Attach to process)

___
## Vimba cameras configuration

### Vimba Tools
Vimba has 2 useful tools to help us configure our cameras:
- **Vimba Viewer**: to configure your cameras and capture images with them. The **Vimba Viewer manual** is stored in */RosScan/Vimba/Vimba_Viewer_Guide.pdf* or can be found in the Vimba installation folders at */opt/Vimba_6_0/Documentation/'Vimba Viewer Guide.pdf'* or online at [this link](https://cdn.alliedvision.com/fileadmin/content/documents/products/software/software/Vimba/docu/manuals/Vimba_Viewer_Guide.pdf).
- **Vimba Firmware Updater**: to update the firmware of your cameras. The **Vimba Firmware Updater manual** is part of the Vimba manual (page 24 to 28). The **Vimba manual** is stored in */RosScan/Vimba/'Vimba Manual.pdf'* or can be found can be found in the Vimba installation folders at */opt/Vimba_6_0/Documentation/'Vimba Manual.pdf'*.

It may be useful to create desktop shortcuts for Vimba Tools, using the next commands:
```console
    ln -sf "/opt/Vimba_6_0/Tools/Viewer/Bin/x86_64bit/VimbaViewer" "$HOME/Desktop"
    ln -sf "/opt/Vimba_6_0/Tools/FirmwareUpdater/Bin/x86_64bit/VimbaFirmwareUpdater" "$HOME/Desktop"
```

### Update your cameras firmwares
To update your cameras' firmwares:
1. Go to */RosScan/Vimba* and extract **Alvium_U3V_Camera_00.11.00.9cf0c21e.zip**, the firmware we are currently using. You can also go to [this link](https://www.alliedvision.com/en/support/firmware-downloads/) and download the last firmware version available for **Alvium USB**, and then extract it.
2. Plug your cameras into you computer using the USB ports.
3. Open the **Vimba Firmware Updater** by double-clicking the link on your desktop.
4. You should see all your cameras in the list of cameras. If not, click the **Update camera list** button.
5. You should see the current firmwares of you cameras under **Current firmware**.
6. Click the **Open** button and select the new firmware version you just extracted.
7. For each camera in the camera list, select your new firmware version under **New firmware**.
8. Click **Update cameras**, then click **OK**.
9. **WARNING! Do not unplug any cameras during the update process!**
10. Once it is finished, click **Close**, then click **Update camera list**. You should see the new firmware under the **Current firmware** of your cameras.

### Launch files configuration
Even if multiple launch exist in */RosScan/Projects/avt_vimba_camera/launch*. Only 2 are useful for us:
- **multi_camera_node.launch**: To operate multiple Vimba cameras using ROS.
- **mono_camera.launch**: To operate one Vimba camera using ROS.

#### Cameras IDs configuration for **multi_camera_node.launch**
You will have to configure the IDs of your cameras in the **cameras_ids.yaml** config file specific for your system. Without this configuration, the node will not be able to find and operate your cameras:
1. In **config** directory, create a copy of **cameras_ids_example.yaml**, named **cameras_ids.yaml**.
2. Plug your cameras into you computer using the USB ports.
3. Open **cameras_ids.yaml**, and change the **0** by your number of cameras you have:
```
camera_qty: 0
```
4. Open the **Vimba Viewer** by double-clicking the link on your desktop.
5. You should see all your cameras under **Detected Cameras**.
6. Click once on your first camera. Wait for a new window to open.
7. Click on the **play** button at the top left of the window. The image capture should start. You can read the FPS under the image. If the image capture starts, go to step 10 directly. If not, follow the next steps.
8. If the image stays black, and the FPS counter stays to zero, it can be because your camera is in triggering mode (if you already used the ROS node with it, it will be the case).
9. You will need to go back to continuous mode. Stop the acquisition with the **play** button. In the params to the right, go to **Trigger IO** and under **Trigger Input/Trigger**, change **Mode** to **Off**. Press **play** again. The acquisition should start.
10. Observe the image to find which camera of the Scanner you just opened. You can position an object in front of each camera until it appears in the center of the image.
11. Once you found the right camera, take note of the number written on the Scanner, next to the lentil of the camera. The **X** value of the **guid_X** of the camera will be this number, minus 1.
12. In the Viewer, go to the black window at the bottom, and copy the **ID** of the camera (it must look like DEV_XXXXXXXXXXXX).
13. Paste this **ID** to replace "DEV_XXXXXXXXXXXX" in **cameras_ids.yaml**, for the right **guid_X**:
```
guid_0: "DEV_XXXXXXXXXXXX"
```
14. **Example:** if the number written on the Scanner next to the camera's lentil is **1**, then you have to replace the "DEV_XXXXXXXXXXXX" of the **guid_0** (1 - 1 = 0) by the **ID** you copied from the black window.
15. Close the window and repeat the steps 6. to 13. for each one of your cameras, to fill **guid_1**, **guid_2** and so on.

The **cameras_ids.yaml** will be called in **multi_camera_node.launch** using the lines:
```
<arg name="cameras_ids_config_file"   default="$(find avt_vimba_camera)/config/cameras_ids.yaml" />
    ...
    <rosparam command="load" file="$(arg cameras_ids_config_file)" />
```

*NOTE: If you have less than 7 cameras, do not bother with the arguments you don't need, the node will only use the one needed, depending on the value of **camera_qty**.*

#### Cameras ID configuration in **mono_camera.launch**
To configure the **mono_camera.launch** to work with your camera, follow the same steps as for the **multi_camera_node.launch**.
You just do not have any *camera_qty* to set.
The next line is an example of where to set your camera ID (line 9):
```
    <arg name="guid"                    default="DEV_XXXXXXXXXXXX"  doc="The GUID for the camera to connect to"/>
```

#### Other important parameters in **multi_camera_node.launch** and **mono_camera.launch**
Several other parameters than the *guid* are important.
You may not need to tune them as they are already set to work with our Scanner configuration, but in case, here is the list:
- **trigger_source**: The GPIO Line you are using to trigger de camera.
- **trigger_mode**: *True* if you want your camera to capture image on trigger signal, *False* otherwise.
- **pixel_format**: To choose the type of compression you want to use.
- **stream_bytes_per_second**: To choose the bandwidth allowed to the camera.
- **width** and **height**: To choose the size of the images (in number of pixels).
- **exposure**: To set the ExposureTime of your cameras.
- **compression_format**: To choose the type of image compression you want to use (jpeg or png).
- **compression_jpeg_quality**: To choose the quality of jpeg compression you want to use.

___
## Installation

### Dependencies
First, you will need to install the Vimba SDK.
Download it from AVT's website [here](https://www.alliedvision.com/en/products/vimba-sdk/#c1497).

Also see the [linux vimba installation instructions](https://cdn.alliedvision.com/fileadmin/content/documents/products/software/software/Vimba/appnote/Vimba_installation_under_Linux.pdf).

It is highly recommended to open the "Vimba Viewer" tool that came along with the SDK and make sure you can connect to your camera.

It may be useful to create a desktop shortcut to Vimba Viewer:
```sh
ln -sf "Vimba_5_0/Tools/Viewer/Bin/x86_64bit/VimbaViewer" "$HOME/Desktop"
```

### ROS Driver

Once you've successfully connected to your camera using Vimba Viewer, you can continue with the ROS driver install:

```
sudo apt install ros-$ROS_DISTRO-avt-vimba-camera
```

## Operational Advice

### MTU Size
If you are using a GigE camera (ethernet-based camera), it is recommended to adjust some settings in your network interface to be able to handle the potentially high bandwidth usage of the camera stream.

On Linux, you will need to increase the MTU (Maximum Transmission Unit) on the network interface attached to the camera.

You can check what your current mtu setting is by running the following command:
```
ip a | grep mtu
```

According to AVT documentation, increase the mtu to `9014`.
If you use Network Manager, this can be done by opening the network interface settings and editing the "MTU" box under the "Identity" tab.

See the "Optimize system performance" section of your camera's technical manual for full details.
For example, the Mako camera technical manual is available [here](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/Mako/techman/Mako_TechMan_en.pdf).

### Receive Buffer Size

It is also recommended to increase your network receive buffer size.
By default, Ubuntu uses `212992`.

You can check what your current buffer size is:
```
sudo sysctl 'net.core.rmem_max'
```
Update the buffer size with the following command:
```
sudo sysctl -w 'net.core.rmem_max=26214400'
```

`26214400` has been tested successfully, but anything above `2000000` is likely fine.

Once you find a value that works for you, you can make the change permanent (persist across reboots) by updating the `/etc/sysctl.conf` file with the following line:

```
net.core.rmem_max=26214400
```

### Camera Settings in General

If you are having difficulty getting the camera to do what you want using the ROS driver, it is suggested to first use Vimba Viewer to play around with settings that work.
The Vimba Viewer GUI will help you determine what settings are available to your camera model and help you tune them easier.
Once you have settings that you are happy with, save them into your own rosparam file or launch file, and the driver will use those settings every time it launches.

Note that this driver makes use of both ROS parameters and dynamic reconfigure.
When the driver first starts, the dyanmic reconfigure server will initialize with all current ROS param values, then trigger a callback to configure the camera.
This means ROS params will take precedence and should be the preferred way to save camera configs meant to be reused.
After the driver initializes, changes to the parameters can be made using dynamic reconfigure RQT tool (rqt_reconfigure).

## ROS Nodes

### mono_camera_node

The mono_camera_node is the main driver that connects to the camera, configures it according to ROS parameters/dynamic reconfigure, and starts publishing image frames.
The driver uses [image_transport](http://wiki.ros.org/image_transport) to publish image frames, so all expected image topics should be available.
See the config file (`cfg/AvtVimbaCamera.cfg`) for documentation regarding the various parameters that can be used to configure the camera itself.
See the launch file (launch/mono_camera.launch) for documentation regarding the operational parameters of the driver.

### trigger_node

The trigger_node is a standalone node for sending out ethernet-based action commands to AVT cameras.
Action commands are useful for triggering frame captures over ethernet.
See AVT's [application note](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/appnote/GigE/Action-Commands_Appnote.pdf) for more details.
Note that cameras must be configured to receive the action commands in addition to running the trigger_node.

## Clock Synchronization

If you wish to use the exact time the image was measured in the header of the ROS messages, it is suggested to use PTP synchronization.
PTP will ensure the clock on the camera is synchronized with the computer, so that measurement times are all based off of the same clock.
Setting the `use_measurement_time` parameter will set the ROS header timestamp to the frame timestamp, but it is up to you to make sure the camera clock is synced with the computer.
[linuxptp](http://linuxptp.sourceforge.net) is a great tool for PTP synchronization and is suggested for ensuring the camera is in sync with the computer.
See the links below for more details on PTP sync.

## Useful Technical References and Application Notes

- [GigE Features Reference](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/features/GigE_Features_Reference.pdf) (To better understand what features your camera supports and how to tune them)
- [Trigger over Ethernet - Action Commands](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/appnote/GigE/Action-Commands_Appnote.pdf)
- [PTP Clock Sync](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/appnote/GigE/PTP_IEEE1588_with_Prosilica_GT_GC_Manta.pdf) (Highly recommended if you care about exact image acquisition time)
- [Image Timestamp on Allied Vision GigE Cameras](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/appnote/GigE/Image_Timestamp.pdf)
- [Decimation](https://cdn.alliedvision.com/fileadmin/content/documents/products/cameras/various/appnote/various/Decimation.pdf) (Binning is similar)
