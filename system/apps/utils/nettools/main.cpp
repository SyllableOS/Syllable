#include <string.h>

#include <exec/types.h>
#include <atheos/time.h>

#include <gui/app.hpp>
#include <gui/message.hpp>
#include <gui/screen.hpp>
#include <gui/window.hpp>
#include <gui/widget.hpp>
#include <gui/slider.hpp>

using namespace Os;

class MonWindow : public Window
{
public:
  MonWindow( const Rect& cFrame );

  bool	OkToQuit();

private:
};

