// Network Preferences :: (C)opyright 2002 Daryl Dudey
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
#include <string>
#include "validator.h"

unsigned int iPos;

int GetIPPart(const char *pzSrc)
{
  bool bAnyAlpha = false;

  // If iPos > len then exit
  if (iPos > strlen(pzSrc)) {
    return -1;
  }

  // Make copy
  char *pzTmp = new char[strlen(pzSrc+iPos)];
  strcpy(pzTmp, pzSrc+iPos);

  unsigned int i = 0;
  while ( pzTmp[i]!='.' && i<strlen(pzTmp)) {
    if ( pzTmp[i]<'0' || pzTmp[i]>'9') {
      bAnyAlpha = true;
    }
    i++;
  };
  
  // If any non-numeric then exit
  if (i!=0 && bAnyAlpha) {
    iPos = iPos+i+1;
    return -1;
  }

  // Check end not reached
  if (i!=0) {
    pzTmp[i] = 0;
    iPos = iPos + i+1;
    return atoi(pzTmp);
  }
  
  return -1;
}

int ValidateIP(const char *pzIPSrc) 
{
  // This function checks the passed string is a valid ip address
  
  // Check it is not all blank
  unsigned int i=0;
  while ( i<strlen(pzIPSrc) && (pzIPSrc[i]==' ') );
  if (i==strlen(pzIPSrc))
      return 0;

  // Now check each part separately
  iPos = 0;
  int iIP1 = GetIPPart(pzIPSrc);
  int iIP2 = GetIPPart(pzIPSrc);
  int iIP3 = GetIPPart(pzIPSrc);
  int iIP4 = GetIPPart(pzIPSrc);

  if (iIP1==-1)
    return 1;
  if (iIP2==-1)
    return 2;
  if (iIP3==-1)
    return 3;
  if (iIP4==-1)
    return 4;

  return 0;
}



