#include <gui/view.h>
#include "blendablecolor.h"

using namespace os;

class GradientBar : public View
{
	public:
	GradientBar(const Rect &cFrame, const char *cName);

	void SetValue(float v);
	float GetValue() { return m_Value; }

	const Color32_s &GetStartCol() { return m_Start; }
	const Color32_s &GetEndCol() { return m_End; }
	void SetStartCol(const Color32_s &c);
	void SetEndCol(const Color32_s &c);

	void Paint(const Rect &cUpdateRect);

	private:

	void DeltaRedraw();

	float		m_Value;
	float		m_OldValue;
	Color32_s	m_Start;
	Color32_s	m_End;
};







