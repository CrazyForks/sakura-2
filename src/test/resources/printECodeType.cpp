/*
	Copyright (C) 2021-2022, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/
#include "pch.h"
#include <ostream>

#include "charset/charset.h"

/*!
	googletestの出力にECodeTypeを出力させる

	パラメータテストのパラメータにECodeTypeを渡した場合に文字列を併記して分かりやすくする。

	テストで使用するコード値のみを定義してあるので、必要があれば追加定義すること。
 */
std::ostream& operator << (std::ostream& os, const ECodeType& eCodeType)
{
	switch(eCodeType){
	case CODE_SJIS:			return os << "SJIS";
	case CODE_JIS:			return os << "JIS";
	case CODE_EUC:			return os << "EUC";
	case CODE_UTF8:			return os << "UTF8";
	case CODE_UTF16LE:		return os << "UTF16LE";
	case CODE_UTF16BE:		return os << "UTF16BE";
	case CODE_UTF32LE:		return os << "UTF32LE";
	case CODE_UTF32BE:		return os << "UTF32BE";
	case CODE_UTF7:			return os << "UTF7";
	case CODE_CESU8:		return os << "CESU8";
	case CODE_LATIN1:		return os << "LATIN1";
	default:
		throw std::invalid_argument("unknown ECodeType");
	}
}
