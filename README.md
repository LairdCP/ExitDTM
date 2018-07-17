# Laird ExitDTM

## About

ExitDTM is a cross-platform utility for exiting from DTM mode and going back to smartBASIC interactive mode on Laird's range of wireless modules, and uses Qt 5. The code uses functionality only supported in Qt 5.7 or greater but with some slight modifications will compile and run fine on earlier versions of Qt 5 (with no loss of functionality). ExitDTM has been tested on Windows, Mac, Arch Linux and Ubuntu Linux and on the Raspberry Pi running Raspbian.

## Downloading

Pre-compiled builds can be found by clicking the [Releases](https://github.com/LairdCP/ExitDTM/releases) tab on Github, builds are available for Linux (32-bit and 64-bit builds, and a 32-bit ARM Raspberry Pi build), Windows (32-bit build) and Mac (64-bit build). 

## Setup

### Windows:

Download and open the zip file, extract the files to a folder on your computer and double click 'ExitDTM.exe' to run ExitDTM.

### Mac:

(**Mac OS X version 10.10 or later is required if using the pre-compiled binaries**): Download the dmg file, open it to mount it as a drive on your computer, go to the disk drive and copy the ExitDTM application to a folder on your computer. You can run ExitDTM by double clicking the icon - if you are greeted with a warning that the executable is untrusted then you can run it by right clicking it and selecting the 'run' option. If this does not work then you may need to view the executable security settings on your mac.

### Linux (Including Raspberry Pi):

Download the tar file and extract it's contents to a location on your computer, this can be done using a graphical utility or from the command line using:

	tar xf ExitDTM_<version>.tar.gz ~/

Where '\~/' is the location of where you want it extracted to, '\~/' will extract to the home directory of your local user account). To launch ExitDTM, either double click on the executable and click the 'run' button (if asked), or execute it from a terminal as so:

	./ExitDTM

Before running, you may need to install some additional libraries, please see https://github.com/LairdCP/ExitDTM/wiki/Installing for further details. You may also be required to add a udev rule to grant non-root users access to USB devices, please see https://github.com/LairdCP/ExitDTM/wiki/Granting-non-root-USB-device-access-(Linux) for details.

## Help and contributing

There are various instructions and help pages available on the [wiki site](https://github.com/LairdCP/ExitDTM/wiki/) and a pdf help document is available from the [Laird BL654 product page](https://www.lairdtech.com/products/bl654-ble-thread-nfc-modules).

## Companian Applications

 * [UwTerminalX](https://github.com/LairdCP/UwTerminalX): a cross platform application for communicating with Laird's range of wireless modules.

## Compiling

For details on compiling, please refer to [the UwTerminalX wiki](https://github.com/LairdCP/UwTerminalX/wiki/Compiling).

## License

ExitDTM is released under the [GPLv3 license](https://github.com/LairdCP/ExitDTM/blob/master/LICENSE).