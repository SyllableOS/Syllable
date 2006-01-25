// Netword Preferences :: (C)opyright Daryl Dudey 2002
//
// 2 September 2003, Kaj de Vos
//   Fixed several off-by-one errors in Load().
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <sys/ioctl.h>
#include <atheos/types.h>
#include <sys/socket.h>
#include <net/nettypes.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "configuration.h"

// Constants for stream output
const char newl = '\n';

Configuration::Configuration()
{
  // Allocate memory
  pzHost = new char[C_CO_HOSTLEN+1];  
  pzDomain = new char[C_CO_DOMAINLEN+1];
  pzName1 = new char[C_CO_IPLEN+1];
  pzName2 = new char[C_CO_IPLEN+1];

  // Okay, now blank all possible adaptor slots
  for (int i=0;i<C_CO_MAXADAPTORS;i++) {
    pcAdaptors[i].pzDescription = new char[C_CO_DESCLEN+1];
    pcAdaptors[i].pzDescription[0] = 0;
    pcAdaptors[i].pzMac = new char[C_CO_MACLEN+1];
    pcAdaptors[i].pzMac[0] = 0;
    pcAdaptors[i].bEnabled = false;
    pcAdaptors[i].bExists = false;
    pcAdaptors[i].pzIP = new char[C_CO_IPLEN+1];
    pcAdaptors[i].pzSN = new char[C_CO_IPLEN+1];
    pcAdaptors[i].pzGW = new char[C_CO_IPLEN+1];
    pcAdaptors[i].bUseDHCP = false;
  }
}

Configuration::~Configuration()
{
  // Release memory
  delete pzHost;
  delete pzDomain;
  delete pzName1;
  delete pzName2;

  // Release memory for each adaptor
  for (int i=0;i<C_CO_MAXADAPTORS;i++) {
    delete pcAdaptors[i].pzDescription;
    delete pcAdaptors[i].pzMac;
    delete pcAdaptors[i].pzIP;
    delete pcAdaptors[i].pzSN;
    delete pcAdaptors[i].pzGW;
  }
}

void Configuration::Load()
{
  // Attempt to open file
  std::ifstream fsIn;
  //os::File
  fsIn.open("/system/config/net.cfg");

  if (!fsIn.is_open()) {

    // Configuration file did not exist, therefore set default values
    strncpy(pzHost, "default", C_CO_HOSTLEN);
    strncpy(pzDomain,"domain", C_CO_DOMAINLEN);
    strncpy(pzName1, "", C_CO_IPLEN);
    strncpy(pzName2, "", C_CO_IPLEN);
    for (int i=0;i<C_CO_MAXADAPTORS;i++) {
      pcAdaptors[i].bExists = false;
      pcAdaptors[i].bEnabled = false;
      strcpy(pcAdaptors[i].pzMac, "");
      strcpy(pcAdaptors[i].pzIP, "0.0.0.0");
      strcpy(pcAdaptors[i].pzSN, "0.0.0.0");
      strcpy(pcAdaptors[i].pzGW, "");
      pcAdaptors[i].bUseDHCP = false;
    }
    
  } else {

    // File does exist, start parsing
    const int C_SCRATCHLEN = 80;
    char *pzScratch = new char[C_SCRATCHLEN];

    // Get host & domain name
    fsIn.getline (pzHost, C_CO_HOSTLEN+1, '\n');
    fsIn.getline (pzDomain, C_CO_DOMAINLEN+1, '\n');

    // Get dns addresses
    fsIn.getline (pzName1, C_CO_IPLEN+1, '\n');
    fsIn.getline (pzName2, C_CO_IPLEN+1, '\n');

    // Loop through all adaptors and read in settings
    for (int i=0;i<C_CO_MAXADAPTORS;i++) {

      // Read in marker flag
      fsIn.getline (pzScratch, C_SCRATCHLEN, '\n');
      if (strcmp(pzScratch, "<ADAPTOR>")!=0)  
	  break;
      
      // Set to exists
      pcAdaptors[i].bExists = true;

      // First line is enabled status
      fsIn.getline (pzScratch, C_SCRATCHLEN, '\n');
      if (strcmp(pzScratch,"Enabled")==0) {
	pcAdaptors[i].bEnabled = true;
      } else {
	pcAdaptors[i].bEnabled = false;
      }

      // Description
      fsIn.getline (pcAdaptors[i].pzDescription, C_CO_DESCLEN+1, '\n');
      
      // Next line is mac address
      fsIn.getline (pcAdaptors[i].pzMac, C_CO_MACLEN+1, '\n');

      // Either the IP address or "DHCP" is next
      // Indicate if DHCP is used for this adaptor
      fsIn.getline( pzScratch, C_SCRATCHLEN, '\n' );
      if( strcmp( pzScratch, "DHCP") == 0 )
      {
        pcAdaptors[i].bUseDHCP = true;
      }
      else
      {
        pcAdaptors[i].bUseDHCP = false;
        // Next three are IP/SN/GW
        //fsIn.getline (pcAdaptors[i].pzIP, C_CO_IPLEN+1, '\n');
        strncpy( pcAdaptors[i].pzIP, pzScratch, C_CO_IPLEN+1 );
        fsIn.getline (pcAdaptors[i].pzSN, C_CO_IPLEN+1, '\n');
        fsIn.getline (pcAdaptors[i].pzGW, C_CO_IPLEN+1, '\n');
      }
    }
    
  }

  // And close
  fsIn.close();
}

void Configuration::Save() 
{
  // Open file for write
  std::ofstream fsOut;
  fsOut.open("/system/config/net.cfg" );

  // Host/Domain name first
  fsOut << pzHost << newl << pzDomain << newl;

  // Now name servers
  fsOut << pzName1 << newl << pzName2 << newl;

  // Now output information for all adaptors
  for (int i=0;i<C_CO_MAXADAPTORS;i++) {
    
    // Only output adaptors that exist
    if (pcAdaptors[i].bExists) {
      
      // Output a start marker
      fsOut << "<ADAPTOR>" << newl;

      // Enabled status first
      if (pcAdaptors[i].bEnabled) { 
	fsOut << "Enabled" << newl;
      } else { 
	fsOut << "Disabled" << newl;
      }
      
      // Description
      fsOut << pcAdaptors[i].pzDescription << newl;

      // Mac address
      fsOut << pcAdaptors[i].pzMac << newl;

	  if( pcAdaptors[i].bUseDHCP )
      {
        // Use DHCP
        fsOut << "DHCP" << newl;
      }
      else
      {
        // IP/SN/GW
        fsOut << pcAdaptors[i].pzIP << newl;
        fsOut << pcAdaptors[i].pzSN << newl;
        fsOut << pcAdaptors[i].pzGW << newl;
      }
    }
  }
  
  // Now output an end marker, so the load routine knows when to stop
  fsOut << "<ADAPTOR-END>" << newl;

  // And Close
  fsOut.close();

  // Now set correct permissions
  chmod( "/system/config/net.cfg", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); 
}

void Configuration::Activate() 
{
  int i;

  // Okay, first lets create the /etc/hostname file and assign permissions
  std::ofstream fsOut;
  fsOut.open("/etc/hostname" );
  fsOut << pzHost << "." << pzDomain << newl;
  fsOut.close();
  chmod("/etc/hostname", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  // Now, the /etc/hosts file and assign permissions
  fsOut.open("/etc/hosts");
  fsOut << "127.0.0.1 localhost localhost.localdomain" << newl;
  
  for (i=0;i<C_CO_MAXADAPTORS;i++) {
    if (pcAdaptors[i].bEnabled) {
      fsOut << pcAdaptors[i].pzIP << " " << pzHost << " " << pzHost << "." << pzDomain << newl;
    }
  }
  fsOut.close();
  chmod("/etc/hosts", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

  // And, resolv.conf, again right permissions
  fsOut.open("/etc/resolv.conf");
  fsOut << "search " << pzDomain << newl;
  if (strlen(pzName1)>0)
    fsOut << "nameserver " << pzName1 << newl;
  if (strlen(pzName2)>0)
    fsOut << "nameserver " << pzName2 << newl;
  fsOut.close();
  chmod("/etc/resolv.conf", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  
  // Next job, create the network-init.sh file containing initialise information
  fsOut.open("/system/network-init.sh");
  fsOut << "#!/bin/sh" << newl;
  for (i=0;i<C_CO_MAXADAPTORS;i++) {
    if (pcAdaptors[i].bEnabled) {
      if( pcAdaptors[i].bUseDHCP )
      {
        fsOut << "dhcpc -d " << pcAdaptors[i].pzDescription << " 2>>/var/log/dhcpc" << newl;
      }
      else
      {
       fsOut << "ifconfig " << pcAdaptors[i].pzDescription << " inet " << pcAdaptors[i].pzIP << " netmask " << pcAdaptors[i].pzSN << newl;
       if (strlen(pcAdaptors[i].pzGW)>0 && strcmp(pcAdaptors[i].pzGW, "0.0.0.0")!=0) 
         fsOut << "route add net " << pcAdaptors[i].pzIP << " mask 0.0.0.0 gw " << pcAdaptors[i].pzGW << newl;
      }
    }
  }
  fsOut.close();
  chmod("/system/network-init.sh", S_IRWXU | S_IRGRP | S_IROTH);
}

int Configuration::Detect() {
  int iSocket = -1;
  int iNumInterfaces;
  struct ifreq *sIFReqs = NULL;
  struct ifconf sIFConf;

  // Allocate global socket
  iSocket = socket(AF_INET, SOCK_STREAM, 0);

  // Get number of interfaces
  iNumInterfaces = ioctl(iSocket, SIOCGIFCOUNT, NULL);
  if (iNumInterfaces==0) { // If no interfaces, we may as well give up now
    return(0);
  }

  // Allocate memory for interfaces
  sIFReqs = (struct ifreq *)calloc(iNumInterfaces, sizeof(struct ifreq));

  // Setup ifConf
  sIFConf.ifc_len = iNumInterfaces * sizeof(struct ifreq);
  sIFConf.ifc_req = sIFReqs;

  // Okay, now get the interface list
  ioctl(iSocket, SIOCGIFCONF, &sIFConf);

  // Okay loop through all found net interfaces
  short sFlags;
  int iAdaptor = 0;
  AdaptorConfiguration *pcAdap;
  for (int i=0;i<iNumInterfaces;i++) {

    // Create a link to the next adaptor configuration
    pcAdap = &pcAdaptors[iAdaptor];
    
    // Copy and read in flags
    struct ifreq sReq;
    memcpy(&sReq, &(sIFConf.ifc_req[i]), sizeof(sReq)); 
    ioctl(iSocket, SIOCGIFFLAGS, &sReq);
    sFlags = sReq.ifr_flags;

    // We are not interested in loopbacks
    if (!(sFlags & IFF_LOOPBACK)) {
      
      // Mark the adaptor as existing
      pcAdap->bExists = true;
      
      // Enable this particular adaptor if its up!
      pcAdap->bEnabled = (sFlags & IFF_UP);
      
      // Copy the name as the description
      sprintf(pcAdap->pzDescription, "%s", sIFConf.ifc_req[i].ifr_name);

      // Get ip address and save it in an adaptor config
      uint8 *ip = (uint8 *)&((struct sockaddr_in *)&(sReq.ifr_addr))->sin_addr; 
      sprintf(pcAdap->pzIP, "%d.%d.%d.%d", (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]) );
      
      // Get the subnet mask and save it
      ioctl(iSocket, SIOCGIFNETMASK, &sReq);
      sprintf(pcAdap->pzSN, "%d.%d.%d.%d", (int)(ip[0]), (int)(ip[1]), (int)(ip[2]), (int)(ip[3]) );

      // And now the mac address
      ioctl(iSocket, SIOCGIFHWADDR, &sReq);
      uint8 *mac = (uint8 *)(sReq.ifr_hwaddr.sa_data);
      sprintf(pcAdap->pzMac, "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X", (uint)mac[0], (uint)mac[1], (uint)mac[2], (uint)mac[3], (uint)mac[4], (uint)mac[5] );
      
      // Onto to next adaptor, if more than max, bug out and maybe miss adaptors off
      if ((iAdaptor++)==C_CO_MAXADAPTORS) {
	return(0);
      }
    }
  };

  // Release global socket
  return(1);
}

bool Configuration::DetectInterfaceChanges()
{
  int i;
  bool bChanges;

  // Save description/mac address for comparison later
  char *pzMacs[C_CO_MAXADAPTORS];
  for (i=0;i<C_CO_MAXADAPTORS;i++) {
    pzMacs[i] = new char[C_CO_MACLEN+1];
    strcpy(pzMacs[i], pcAdaptors[i].pzMac);
  }

  // Now run detection code
  Detect();
  
  // And see if any changes are detected
  for (i=0;i<C_CO_MAXADAPTORS;i++) {
    if (strcmp(pzMacs[i], pcAdaptors[i].pzMac)!=0)
      break;
  }
  if (i!=C_CO_MAXADAPTORS) {
    bChanges = true;
  } else {
    bChanges = false;
  }

  // Now release memory used
  for (i=0;i<C_CO_MAXADAPTORS;i++) {
    delete pzMacs[i];
  }

  return bChanges;
}

