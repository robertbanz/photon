
Photon Game system currently requires one Mac OS capable system, with

-   2 Network Interfaces (one will connect to the private game network, one to connect to the outside world)
    
-   A DMX interface (such as the [EntTec DMX USB Pro](https://www.enttec.com/products/controls/dmx-usb/2-universe-usb-computer-interface-dmx/)) to run the field lights
    

  

The Game system itself will run two VMs

-   PFSense gateway (to isolate the private game network)
    
-   “Game Computer” which runs FreeBSD
    

  

How To Build:

-   Install OSX
    
-   Create a user (e.g. game)
    
-   Configure the user for auto-login on bootup.
    
-   Git clone [https://github.com/robertbanz/photon.git](https://github.com/robertbanz/photon.git) in home directory
    
-   [Install QLC+](https://www.qlcplus.org/) for lighting. Make sure open photon/lighting/LightingStartup.app works, and can talk to the DMX device
    
-   Install VMWare Fusion
    
-   Add photon/lighting/LightingStartup.app to Startup Items
    

  

Build PFSense Instance

-   Download PFSense image from pfsense.org (ISO installer is probably the best)
    
-   Edit /Library/Preferences/VMware Fusion/networking:
    

-   VNET_2_HOSTONLY_NETMASK
    
-   VNET_2_HOSTONLY_SUBNET
    
-   VNET_2_VIRTUAL_ADAPTER
    
-   VNET_2_VIRTUAL_ADAPTER_ADDR 172.16.1.2
    

-   Set the IP network config for whatever interface you’re using to the private network to ‘off’
    
-   Run sudo touch /Library/Preferences/VMware\ Fusion/promiscAuthorized
    
-   Create the VM
    

-   Freebsd 64-bit
    
-   Customize settings ->
    

-   Save as “gw”
    
-   Give 1024M of ram.
    
-   Set the primary network adapter to “bridged (autodetect)” that’s connected to the intertubes.
    
-   Add a network adapter, and set it to custom (vmnet2)
    
-   Add a third network adapter, set to bridged (to the external private network for the game) to what will be your private game network.
    
-   (you’ll probably have to reboot to get that to take effect)
    
-   Do the easy install of PFSense
    
-   Configure the LAN network IP to be something you like (e.g. 172.16.1.1) and enable the DHCP server
    
-   Set the start address as 172.16.1.100 or so
    
-   Hostname: gw
    
-   Domain: something…
    
-   Disable blocking of RFC1918 networks or bogons, since we’re probably already sitting on a private network.
    
-   Assign the third network interface, and name it LAN_EXT (don’t assign any IP configuration)
    
-   Create a bridge between LAN & LAN_EXT
    

-   Add the VM to StartupItems so you don’t have to worry about it again.
    
-   Probably want to add “suspend.disabled=TRUE” to the vmx file.
    
-   Reboot to make sure everything works.
    

  

Building the Game Computer image

-   Download current FreeBSD release (e.g. 11.2) .iso image
    
-   Create VM
    

-   Save as ‘game’
    
-   Edit custom settings:
    

-   Give it more cores maybe
    
-   Give it some more ram
    
-   Configure network adapter to connect to vmnet2
    
-   Set hard disk size to something good (like 100g)
    
-   Remove extra stuff (Camera?)
    

-   Do a pretty much default install of the OS
    
-   Make a ‘game’ user (uid 1000)
    
-   Add a static mapping for the mac address of the game vm in the pfsense dhcp leases
    
-   In DNS resolver, “register DHCP static mappings”
    
-   As root:
    

-   Install prereqs from etc/packgelist.txt (pkg install `cat packagelist.txt`)
    
-   Install OLA (open Lighting Architecture). Will have to be built from source.  
    ([https://opendmx.net/index.php/OLA_on_FreeBSD](https://opendmx.net/index.php/OLA_on_FreeBSD)) Used to control QLC+ running on the Mac.
    
-   Will probably need to :sudo python3 -m pip install numpy
    

-   And fix .configure to not care about -lresolv.
    

-   Mkdir /usr/photon; chown game.game
    
-   cd /usr; git clone [https://github.com/robertbanz/photon.git](https://github.com/robertbanz/photon.git)
    

-   Add or edit passwd file entries to those in photon/etc/passwd
    
-   Install devd config (also in etc)
    
-   Append rc.conf entries to /etc/rc.conf
    
-   Copy rc.local to /etc/rc.local
    
-   Setup game.conf.local
    
-   /usr/local/etc/postgresql initdb
    
-     
    
-   Edit postgres conf:
    

-   listen_addresses = '*'
    

-   And pg_hba.conf:
    

-   host all all 172.16.1.0/24 trust
    

-   /usr/local/etc/postgresql start
    
-   Run setupdatabase as the psql user.
    
-   As game, run psql photon < database_schema
    
-   And then run psql photon < g_modes.data, and g_tracks.data to intialize the db.
    

-   Install Chicken (VNC viewer) on the mac.
    
-   Add a boot delay to the game computer in /etc/loader.conf so the game instance won’t try to do anything before the gw is up.
    

  

Post Cloning the drive image:

-   Change Subnet
    

-   Pfsense (address, dhcp range), interface plumbing.
    
-   vmnet2 config
    
-   Osx network config
    
-   Regenerate mac addresses on vms
