#ifndef FRIDGECAM_FCWIN_H
#define FRIDGECAM_FCWIN_H
//-----------------------------------------------------------------------------

#include <gui/window.h>
namespace os { class CheckBox; class LayoutNode; class LayoutView; class Slider; class TabView; }

//-----------------------------------------------------------------------------

class BitmapView;

class FCWin : public os::Window
{
public:
    FCWin( os::Rect rect, const char *title );

    void DrawBitmap( const os::Bitmap *bitmap );
    void SetExposure( float exposure, int gain );
    void SetColorBalance( float red, float green, float blue );

private:
    virtual bool OkToQuit();

    virtual void HandleMessage( os::Message* message );

private:
    void LoadConfig();
    void SaveConfig() const;

    void GuiSensorFPS( os::LayoutNode *parent );
    void GuiExposure( os::LayoutNode *parent );
    void GuiCompress( os::LayoutNode *parent );

    void GuiColorParams( os::LayoutNode *parent );
    void GuiColorBalance( os::LayoutNode *parent );

    
    int Framerate_Rate2Index( float rate );
    float Framerate_Index2Rate( int index );

    void SetCameraParms( uint32 flags );
    void CamSetCompression();
    
    os::LayoutView *fRootView;
    BitmapView	   *fBitmapView;
    os::CheckBox   *fExposureCheckBox;
    os::Slider     *fExposureSlider;
    os::Slider     *fGainSlider;
    os::TabView	   *fCompressionTabView;

      // compress:none
    os::CheckBox   *fCompNoneDecimateCheckBox;

      // compress:auto
    os::CheckBox   *fCompAutoDecimateCheckBox;
    os::CheckBox   *fCompAutoTargetCheckBox;
    os::Slider     *fCompAutoQualitySlider;
    os::Slider     *fCompAutoFramerateSlider;

      // compress:manual
    os::CheckBox   *fCompManualDecimateCheckBox;
    os::Slider     *fCompManualYQualitySlider;
    os::Slider     *fCompManualUVQualitySlider;

      // advanced:colorparams
    os::Slider     *fBrightnessSlider;
    os::Slider	   *fContrastSlider;
    os::Slider     *fSaturationSlider;

      // advanced:colorbalance
    os::CheckBox   *fColorBalanceCheckBox;
    os::Slider	   *fColorBalanceSlider[3];
    
      // config
    float	   fFramerate;
    bool	   fManualExposure;
    float	   fExposure;
    int		   fGain;
    int		   fCompressionMode; // in the same order as the tabs
    bool	   fAutoTargetQuality;
    float	   fAutoQuality;
    float	   fAutoFramerate;
    bool	   fNoneDecimate;
    bool	   fAutoDecimate;
    bool	   fManualDecimate;
    float	   fManualYQuality;
    float	   fManualUVQuality;
    float          fBrightness;
    float          fContrast;
    float          fSaturation;
    bool           fManualColorBalance;
    float          fColorbalance[3];
};

//-----------------------------------------------------------------------------
#endif
