/*
 * Albert 0.4 * Copyright (C)2002 Henrik Isaksson
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "calculator.h"

#define __USE_ISOC9X

#include <math.h>

#include <stdio.h>

#include <gui/window.h>

Calculator::Calculator()
{
	m_Depth = 0;
	m_Memory = 0;
	m_Inv = false;
	m_Hyp = false;
}

void Calculator::Inv(bool inv)
{
	m_Inv = inv;
}

void Calculator::Hyp(bool hyp)
{
	m_Hyp = hyp;
}

void Calculator::Clear()
{
	Stack::Clear();
	m_Depth = 0;
}

long double Calculator::DoOp(long double a, char op, long double b)
{
	switch(op) {
		case '*':
			return a*b;
		case '/':
			if(b != 0.0)
				return a/b;
			return 0;	// ERR
		case '+':
			return a+b;
		case '-':
			return a-b;
		case '^':
			return powl(a, b);
		case '&':
			return (int64)a & (int64)b;
		case '|':
			return (int64)a | (int64)b;
		case 'X':
			return (int64)a ^ (int64)b;
		case '\0':
			return b;
	}
	return 0;
}

long double Calculator::Mul(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = '*';
		Push((StackNode *)val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = '*';
		Push((StackNode *)val);
	}
	return val->m_dValueB;
}

long double Calculator::Div(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = '/';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = '/';
		Push(val);
	}
	return val->m_dValueB;
}

long double Calculator::Add(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		if(val->m_cOpB) {
			val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
			val->m_cOpB = 0;
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, val->m_dValueB);
		} else {
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, p_dVal);
		}
		val->m_cOpA = '+';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueA = p_dVal;
		val->m_cOpA = '+';
		Push(val);
	}
	return val->m_dValueA;
}

long double Calculator::Sub(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		if(val->m_cOpB) {
			val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
			val->m_cOpB = 0;
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, val->m_dValueB);
		} else {
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, p_dVal);
		}
		val->m_cOpA = '-';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueA = p_dVal;
		val->m_cOpA = '-';
		Push(val);
	}
	return val->m_dValueA;
}

long double Calculator::Equals(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		if(val->m_cOpB) {
			val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
			val->m_cOpB = 0;
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, val->m_dValueB);
		} else {
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, p_dVal);
		}
		val->m_cOpA = '\0';

		long double tmpval = val->m_dValueA;

		delete val;

		return tmpval;
	}

	return 0.0;
}

long double Calculator::Neg(long double p_dVal)
{
	return -p_dVal;
}

long double Calculator::MPlus(long double p_dVal)
{
	m_Memory += p_dVal;
	return p_dVal;
}

long double Calculator::MMinus(long double p_dVal)
{
	m_Memory -= p_dVal;
	return p_dVal;
}

long double Calculator::MClear(long double p_dVal)
{
	m_Memory = 0;
	return p_dVal;
}

long double Calculator::MRead(long double p_dVal)
{
	return m_Memory;
}

long double Calculator::Sin(long double p_dVal)
{
	if(m_Hyp) {
		if(m_Inv)
			return asinhl(p_dVal);
		else
			return sinhl(p_dVal);
	} else {
		if(m_Inv)
			return asinl(p_dVal);
		else
			return sinl(p_dVal);
	}
}

long double Calculator::Cos(long double p_dVal)
{
	if(m_Hyp) {
		if(m_Inv)
			return acoshl(p_dVal);
		else
			return coshl(p_dVal);
	} else {
		if(m_Inv)
			return acosl(p_dVal);
		else
			return cosl(p_dVal);
	}
}

long double Calculator::Tan(long double p_dVal)
{
	if(m_Hyp) {
		if(m_Inv)
			return atanhl(p_dVal);
		else
			return tanhl(p_dVal);
	} else {
		if(m_Inv)
			return atanl(p_dVal);
		else
			return tanl(p_dVal);
	}
}

long double Calculator::Log2(long double p_dVal)
{
	return log10l(p_dVal)/log10l(2);
}

long double Calculator::Log(long double p_dVal)
{
	return log10l(p_dVal);
}

long double Calculator::Ln(long double p_dVal)
{
	return logl(p_dVal);
}

long double Calculator::Pow2(long double p_dVal)
{
	return powl(p_dVal, 2);
}

long double Calculator::Pow3(long double p_dVal)
{
	return powl(p_dVal, 3);
}

long double Calculator::Sqrt(long double p_dVal)
{
	return sqrtl(p_dVal);
}

long double Calculator::PowE(long double p_dVal)
{
	return expl(p_dVal);
}

long double Calculator::PowX(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = '^';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = '^';
		Push(val);
	}
	return val->m_dValueB;
}

long double Calculator::Int(long double p_dVal)
{
	return (int64)p_dVal;
}

long double Calculator::Not(long double p_dVal)
{
	return ~(int64)p_dVal;
}

long double Calculator::And(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = '&';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = '&';
		Push(val);
	}
	return val->m_dValueB;
}

long double Calculator::Or(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = '|';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = '|';
		Push(val);
	}
	return val->m_dValueB;
}

long double Calculator::Xor(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(val) {
		val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
		val->m_cOpB = 'X';
		Push(val);
	} else {
		val = new StackVal;
		val->m_dValueB = p_dVal;
		val->m_cOpB = 'X';
		Push(val);
	}
	return val->m_dValueB;
}

long double Calculator::OpenParen(long double p_dVal)
{
	StackVal *val = new StackVal;
	Push(val);
	m_Depth++;
	return 0.0;
}

long double Calculator::CloseParen(long double p_dVal)
{
	StackVal *val;

	val = (StackVal *)Pop();

	if(m_Depth)
		m_Depth--;

	if(val) {
		if(val->m_cOpB) {
			val->m_dValueB = DoOp(val->m_dValueB, val->m_cOpB, p_dVal);
			val->m_cOpB = 0;
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, val->m_dValueB);
		} else {
			val->m_dValueA = DoOp(val->m_dValueA, val->m_cOpA, p_dVal);
		}
		val->m_cOpA = '\0';

		long double tmpval = val->m_dValueA;

		delete val;

		return tmpval;
	}

	return 0.0;
}

