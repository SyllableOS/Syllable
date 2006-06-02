//========================================================================
//
// XPDFApp.cc
//
// Copyright 2002-2003 Glyph & Cog, LLC
//
//========================================================================

#include <aconf.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "GString.h"
#include "GList.h"
#include "Error.h"
#include "XPDFViewer.h"
#include "XPDFApp.h"
#include "config.h"


static bool defInstallCmap = false;
static int defRGBCubeSize = defaultRGBCube;
static bool defReverseVideo = false;
static bool defViKeys = false;

//------------------------------------------------------------------------
// XPDFApp
//------------------------------------------------------------------------


XPDFApp::XPDFApp(int *argc, char *argv[]) {

  appContext = new os::Application( "application/x-vnd.syllable-Xpdf" );

/* Select string catalogue */
	try {
		appContext->SetCatalog( "xpdf.catalog" );
	} catch( ... ) {
		printf( "Failed to load catalog file!\n" );
	}

/* Register filetypes */
	m_pcManager = NULL;

	try
	{
		m_pcManager = os::RegistrarManager::Get();
		
		m_pcManager->RegisterType( "text/x-pdf", "PDF document" );
		m_pcManager->RegisterTypeExtension( "text/x-pdf", "pdf" );
		m_pcManager->RegisterTypeIconFromRes( "text/x-pdf", "text_pdf.png" );
		m_pcManager->RegisterAsTypeHandler( "text/x-pdf" );
	} catch( ... ) {}

  fullScreen = gFalse;
  
  getResources();

  viewers = new GList();
}

void XPDFApp::getResources() {
	title = new GString( "Xpdf" );
	installCmap = gFalse;
	rgbCubeSize = defaultRGBCube;
	reverseVideo = defReverseVideo;
	if (reverseVideo) {
    paperRGB = splashMakeRGB8(0x00, 0x00, 0x00);
    paperColor = 0x00000000;
  } else {
    paperRGB = splashMakeRGB8(0xff, 0xff, 0xff);
    paperColor = 0xffffffff;
  }
  initialZoom = (GString *)NULL;;
  viKeys = defViKeys;

}

XPDFApp::~XPDFApp() {
	if( m_pcManager )
		m_pcManager->Put();
  deleteGList(viewers, XPDFViewer);
  if (geometry) {
    delete geometry;
  }
  if (title) {
    delete title;
  }
  if (initialZoom) {
    delete initialZoom;
  }
}

XPDFViewer *XPDFApp::open(GString *fileName, int page,
			  GString *ownerPassword, GString *userPassword) {
  XPDFViewer *viewer;
  viewer = new XPDFViewer(this, fileName, page, NULL,
			  ownerPassword, userPassword);

  if (!viewer->isOk()) {
    delete viewer;
    return NULL;
  }

  viewers->append(viewer);
  return viewer;
}

XPDFViewer *XPDFApp::openAtDest(GString *fileName, GString *dest,
				GString *ownerPassword,
				GString *userPassword) {
  XPDFViewer *viewer;

  viewer = new XPDFViewer(this, fileName, 1, dest,
			  ownerPassword, userPassword);
  if (!viewer->isOk()) {
    delete viewer;
    return NULL;
  }
  viewers->append(viewer);
  return viewer;
}

void XPDFApp::close(XPDFViewer *viewer, GBool closeLast) {
  int i;

  if (viewers->getLength() == 1) {
    if (viewer != (XPDFViewer *)viewers->get(0)) {
      return;
    }
    if (closeLast) {
      quit();
    } else {
      viewer->clear();
    }
  } else {
    for (i = 0; i < viewers->getLength(); ++i) {
      if (((XPDFViewer *)viewers->get(i)) == viewer) {
	viewers->del(i);
	delete viewer;
	return;
      }
    }
  }
}

void XPDFApp::quit() {
  while (viewers->getLength() > 0) {
    delete (XPDFViewer *)viewers->del(0);
  }
  //exit(0);
  appContext->PostMessage( os::M_TERMINATE );
}

void XPDFApp::run() {

	appContext->Run();
}



