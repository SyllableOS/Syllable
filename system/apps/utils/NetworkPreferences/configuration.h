// Network Preferences :: (C)opyright Daryl Dudey 2002
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

#ifndef __CONFIGURATION_H_
#define __CONFIGURATION_H_

const int C_CO_DESCLEN = 25;
const int C_CO_MACLEN = 18;
const int C_CO_HOSTLEN = 25;
const int C_CO_DOMAINLEN = 25;
const int C_CO_IPLEN = 15;

const int C_CO_MAXADAPTORS = 4;

class AdaptorConfiguration {
public:
  char *pzDescription;
  char *pzMac;
  bool bExists;
  bool bEnabled;
  char *pzIP, *pzSN, *pzGW;
};

class Configuration {
public:
  Configuration();
  ~Configuration();
  void Load();
  void Save();
  void Activate();
  int Detect();
  bool DetectInterfaceChanges();

  char *pzHost, *pzDomain, *pzName1, *pzName2;
  AdaptorConfiguration pcAdaptors[C_CO_MAXADAPTORS]; // FIXME: BAD Practice I know, sort it out so it only allocates enough later
};

// The global configuration variable
extern Configuration *pcConfig;

#endif /* __CONFIGURATION_H */
