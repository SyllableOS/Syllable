#ifndef CALCULATOR_H
#define CALCULATOR_H

#include "stack.h"

class Calculator : public Stack {
	public:
	Calculator();

	long double Add(long double p_dVal);
	long double Sub(long double p_dVal);
	long double Mul(long double p_dVal);
	long double Div(long double p_dVal);
	long double Neg(long double p_dVal);

	long double Equals(long double p_dVal);

	long double MPlus(long double p_dVal);
	long double MMinus(long double p_dVal);
	long double MClear(long double p_dVal);
	long double MRead(long double p_dVal);

	long double OpenParen(long double p_dVal);
	long double CloseParen(long double p_dVal);

	long double Sin(long double p_dVal);
	long double Cos(long double p_dVal);
	long double Tan(long double p_dVal);
	long double Log(long double p_dVal);
	long double Log2(long double p_dVal);
	long double Ln(long double p_dVal);
	long double Sqrt(long double p_dVal);
	long double Pow3(long double p_dVal);
	long double Pow2(long double p_dVal);
	long double PowE(long double p_dVal);
	long double PowX(long double p_dVal);
	long double And(long double p_dVal);
	long double Or(long double p_dVal);
	long double Xor(long double p_dVal);
	long double Not(long double p_dVal);
	long double Int(long double p_dVal);

	int ParDepth(void) { return m_Depth; }

	void Clear(void);
	void Inv(bool);
	void Hyp(bool);

	private:
	long double DoOp(long double a, char op, long double b);

	long double	m_Memory;
	int				m_Depth;
	bool			m_Inv;		// Inverse trigonometrics
	bool			m_Hyp;
};

class StackVal : public StackNode {
	public:
	StackVal() { m_dValueA = m_dValueB = 0; m_cOpA = m_cOpB = '\0'; }

	long double	m_dValueA;
	char			m_cOpA;
	long double	m_dValueB;
	char			m_cOpB;
};

#endif /* CALCULATOR_H */

