//  AView 0.2 -:-  (C)opyright 2000-2001 Kristian Van Der Vliet
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//  MA 02111-1307, USA
//

//Local include files

#include "mainapp.h"

//Main

int main(int argc, char *argv[])
{

    ImageApp* pc_ImageApp=NULL;
    char *imageFilename=NULL;

    if (argc>0)
        imageFilename=argv[1];
    pc_ImageApp= new ImageApp(imageFilename);
    pc_ImageApp->Run();

    return(0);

}





