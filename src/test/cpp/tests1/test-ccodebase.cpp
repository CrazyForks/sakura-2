/*
	Copyright (C) 2021-2026, Sakura Editor Organization

	SPDX-License-Identifier: Zlib
*/

#include "pch.h"
#include "charset/CCodeFactory.h"

namespace convert {

std::string ToUtf16LeBytes(std::string_view ascii)
{
	std::string dest;
	dest.reserve(ascii.size() * 2);
	std::ranges::for_each(ascii, [&dest](char8_t c) {
		dest += LOBYTE(c);
		dest += '\0';
	});
	return dest;
}

std::string ToUtf16LeBytes(std::wstring_view wide)
{
	std::string dest;
	dest.reserve(wide.size() * 2);
	std::ranges::for_each(wide, [&dest](uchar16_t c) {
		dest += LOBYTE(c);
		dest += HIBYTE(c);
	});
	return dest;
}

std::string ToUtf16BeBytes(std::string_view ascii)
{
	std::string dest;
	dest.reserve(ascii.size() * 2);
	std::ranges::for_each(ascii, [&dest](char8_t c) {
		dest += '\0';
		dest += LOBYTE(c);
	});
	return dest;
}

std::string ToUtf16BeBytes(std::wstring_view wide)
{
	std::string dest;
	dest.reserve(wide.size() * 2);
	std::ranges::for_each(wide, [&dest](uchar16_t c) {
		dest += HIBYTE(c);
		dest += LOBYTE(c);
	});
	return dest;
}

std::string ToUtf32LeBytes(std::string_view ascii)
{
	std::string dest;
	dest.reserve(ascii.size() * 4);
	std::ranges::for_each(ascii, [&dest](uchar32_t c) {
		dest += LOBYTE(LOWORD(c));
		dest += HIBYTE(LOWORD(c));
		dest += LOBYTE(HIWORD(c));
		dest += HIBYTE(HIWORD(c));
	});
	return dest;
}

std::string ToUtf32LeBytes(std::wstring_view wide)
{
	std::string dest;
	dest.reserve(wide.size() * 2);
	std::ranges::for_each(wide, [&dest](uchar32_t c) {
		dest += LOBYTE(LOWORD(c));
		dest += HIBYTE(LOWORD(c));
		dest += LOBYTE(HIWORD(c));
		dest += HIBYTE(HIWORD(c));
	});
	return dest;
}

std::string ToUtf32BeBytes(std::string_view ascii)
{
	std::string dest;
	dest.reserve(ascii.size() * 4);
	std::ranges::for_each(ascii, [&dest](uchar32_t c) {
		dest += HIBYTE(HIWORD(c));
		dest += LOBYTE(HIWORD(c));
		dest += HIBYTE(LOWORD(c));
		dest += LOBYTE(LOWORD(c));
	});
	return dest;
}

std::string ToUtf32BeBytes(std::wstring_view wide)
{
	std::string dest;
	dest.reserve(wide.size() * 2);
	std::ranges::for_each(wide, [&dest](uchar32_t c) {
		dest += HIBYTE(HIWORD(c));
		dest += LOBYTE(HIWORD(c));
		dest += HIBYTE(LOWORD(c));
		dest += LOBYTE(LOWORD(c));
	});
	return dest;
}

/*!
 * @brief MIMEヘッダーデコードテストのパラメーター
 *
 * @param eCodeType 文字コードセット種別
 * @param input デコードする文字列
 * @param optExpected デコードされたバイト列の期待値
 */
using MIMEHeaderDecodeTestParam = std::tuple<ECodeType, std::string_view, std::optional<std::string>>;

//! MIMEヘッダーデコードテスト
struct MIMEHeaderDecodeTest : public ::testing::TestWithParam<MIMEHeaderDecodeTestParam> {};

TEST_P(MIMEHeaderDecodeTest, DoDecode)
{
	const auto  eCodeType   = std::get<0>(GetParam());
	const auto  input       = std::get<1>(GetParam());
	const auto& optExpected = std::get<2>(GetParam());

	CMemory m;
	const auto result = CCodeBase::MIMEHeaderDecode(std::data(input), std::size(input), &m, eCodeType);

	EXPECT_THAT((bool)result, optExpected.has_value());

	if (optExpected.has_value()) {
		const std::string_view decoded{ LPCSTR(m.GetRawPtr()), size_t(m.GetRawLength()) };
		EXPECT_THAT(decoded, StrEq(*optExpected));
	}
}

INSTANTIATE_TEST_SUITE_P(
	MIMEHeaderCases,
	MIMEHeaderDecodeTest,
	::testing::Values(
		MIMEHeaderDecodeTestParam{ CODE_JIS,  "From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC?=",       "From: $B%5%/%i(B" },							// Base64 JIS
		MIMEHeaderDecodeTestParam{ CODE_UTF8, "From: =?utf-8?B?44K144Kv44Op?=",                 "From: \xe3\x82\xb5\xe3\x82\xaf\xe3\x83\xa9" },		// Base64 UTF-8
		MIMEHeaderDecodeTestParam{ CODE_UTF8, "From: =?utf-8?Q?=E3=82=B5=E3=82=AF=E3=83=A9!?=", "From: \xe3\x82\xb5\xe3\x82\xaf\xe3\x83\xa9!" },	// Quoted Printable UTF-8
		MIMEHeaderDecodeTestParam{ CODE_UTF8, "From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC?=",       "From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC?=" },		// 引数の文字コードとヘッダー内の文字コードが異なる場合は変換しない
		MIMEHeaderDecodeTestParam{ CODE_UTF7, "From: =?utf-7?B?+MLUwrzDp-",                     "From: =?utf-7?B?+MLUwrzDp-" },						// 対応していない文字コードなら変換しない
		MIMEHeaderDecodeTestParam{ CODE_UTF8, "From: =?iso-2022-jp?X?GyRCJTUlLyVpGyhC?=",       "From: =?iso-2022-jp?X?GyRCJTUlLyVpGyhC?=" },		// 謎の符号化方式が指定されていたら何もしない
		MIMEHeaderDecodeTestParam{ CODE_JIS,  "From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC",         "From: =?iso-2022-jp?B?GyRCJTUlLyVpGyhC" }			// 末尾の ?= がなければ変換しない
	));

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeSJis)
{
	const auto eCodeType = CODE_SJIS;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（Shift-JIS仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "\xB6\xC5\x82\xA9\x82\xC8\x83\x4A\x83\x69\x8A\xBF\x8E\x9A";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());

	// Unicodeから変換できない文字（Shift-JIS仕様）
	// 1. SJIS⇒Unicode変換ができても、元に戻せない文字は変換失敗と看做す。
	//    該当するのは NEC選定IBM拡張文字 と呼ばれる約400字。
	// 2. 先行バイトが範囲外
	//    (ch1 >= 0x81 && ch1 <= 0x9F) ||
	//    (ch1 >= 0xE0 && ch1 <= 0xFC)
	// 3. 後続バイトが範囲外
	//    ch2 >= 0x40 &&  ch2 != 0xFC &&
	//    ch2 <= 0x7F
	constexpr const auto& mbsCantConvSJis =
		"\x87\x40\xED\x40\xFA\x40"					// "①纊ⅰ" NEC拡張、NEC選定IBM拡張、IBM拡張
		"\x80\x40\xFD\x40\xFE\x40\xFF\x40"			// 第1バイト不正
		"\x81\x0A\x81\x7F\x81\xFD\x81\xFE\x81\xFF"	// 第2バイト不正
		;
	constexpr const auto& wcsCantConvSJis =
		L"①\xDCED\xDC40ⅰ"
		L"\xDC80@\xDCFD@\xDCFE@\xDCFF@"
		L"\xDC81\n\xDC81\x7F\xDC81\xDCFD\xDC81\xDCFE\xDC81\xDCFF"
		;

	auto result3 = CCodeFactory::LoadFromCode(eCodeType, mbsCantConvSJis);
	EXPECT_THAT(result3.destination, StrEq(wcsCantConvSJis));
	EXPECT_THAT(result3, IsTrue());	//👈 仕様バグ。変換できないので true が返るべき。

	// Unicodeから変換できない文字（Shift-JIS仕様）
	constexpr const auto& wcsOGuy = L"森鷗外";
	constexpr const auto& mbsOGuy = "\x90\x58\x3F\x8A\x4F"; //森?外

	const auto cresult4 = CCodeFactory::ConvertToCode(eCodeType, wcsOGuy);
	EXPECT_THAT(cresult4.destination, StrEq(mbsOGuy));
	EXPECT_THAT(cresult4, IsFalse());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeJis)
{
	const auto eCodeType = CODE_JIS;

	// 7bit ASCII範囲（ISO-2022-JP仕様）
	constexpr const auto& mbsAscii = "\x1\x2\x3\x4\x5\x6\a\b\t\n\v\f\r\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x1\x2\x3\x4\x5\x6\a\b\t\n\v\f\r\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\xDC1B\xDC1C\xDC1D\xDC1E\xDC1F\xDC20!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	// 不具合1: 不正なエスケープシーケンスをエラーバイナリに格納してない
	// 不具合2: C0領域の文字 DEL(0x7F) を取り込めてない
	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(L"\x1\x2\x3\x4\x5\x6\a\b\t\n\v\f\r\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A???!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~?"));
	EXPECT_THAT(result1, IsFalse());	// 👈 不正なエスケープシーケンスを検出してるので戻り値は false で正しい。

	result1.destination = wcsAscii;

	// 不具合3: エラーバイナリを復元してない
	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq("\x1\x2\x3\x4\x5\x6\a\b\t\n\v\f\r\xE\xF\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A??????!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F"));
	EXPECT_THAT(cresult1, IsFalse());	// 👈 エラーバイナリの復元はエラーではないので true を返すべき。

	// かな漢字の変換（ISO-2022-JP仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "\x1B(I6E\x1B$B$+$J%+%J4A;z\x1B(B";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());

	// JIS範囲外（ありえない値を使う。Windows拡張があるため境界テスト不可。）
	EXPECT_THAT(CCodeFactory::LoadFromCode(eCodeType, "\x1B$B\xFF\xFF\x1B(B").result, RESULT_LOSESOME);

	// 不正なエスケープシーケンス（JIS X 0212には非対応。）
	EXPECT_THAT(CCodeFactory::LoadFromCode(eCodeType, "\x1B(D33\x1B(B").result, RESULT_LOSESOME);

	// Shift-Jisに変換できない文字
	EXPECT_THAT(CCodeFactory::ConvertToCode(eCodeType, L"\u9DD7").result, RESULT_LOSESOME);	// 森鴎外の鷗
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeEucJp)
{
	const auto eCodeType = CODE_EUC;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（EUC-JP仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "\x8E\xB6\x8E\xC5\xA4\xAB\xA4\xCA\xA5\xAB\xA5\xCA\xB4\xC1\xBB\xFA";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());

	// Unicodeから変換できない文字（EUC-JP仕様）
	// （保留）
	constexpr const auto& mbsCantConvEucJp =
		""	// 第1バイト不正
		""	// 第2バイト不正
		;
	constexpr const auto& wcsCantConvEucJp =
		L""
		L""
		;

	auto result3 = CCodeFactory::LoadFromCode(eCodeType, mbsCantConvEucJp);
	//ASSERT_THAT(result3.destination, StrEq(wcsCantConvEucJp));
	//ASSERT_THAT(result3.losesome, IsFalse());

	// Unicodeから変換できない文字（EUC-JP仕様）
	constexpr const auto& wcsOGuy = L"森鷗外";
	constexpr const auto& mbsOGuy = "\xBF\xB9\x3F\xB3\xB0"; //森?外

	// 本来のEUC-JPは「森鷗外」を正確に表現できるため、不具合と考えられる。
	//constexpr const auto& wcsOGuy = L"森鷗外";
	//constexpr const auto& mbsOGuy = "\xBF\xB9\x8F\xEC\x3F\xB3\xB0";

	const auto cresult4 = CCodeFactory::ConvertToCode(eCodeType, wcsOGuy);
	EXPECT_THAT(cresult4.destination, StrEq(mbsOGuy));
	EXPECT_THAT(cresult4, IsFalse());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeLatin1)
{
	const auto eCodeType = CODE_LATIN1;

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// Latin1はかな漢字変換非サポートなので、0x80以上の変換できる文字をチェックする
	// 符号位置 81, 8D, 8F, 90, および 9D は未使用だが、そのまま出力される。
	constexpr const auto& wcsLatin1ExtChars =
		L"€\x81‚ƒ„…†‡ˆ‰Š‹Œ\x8DŽ\x8F"
		L"\x90‘’“”•–—˜™š›œ\x9DžŸ"
		L"\xA0¡¢£¤¥¦§¨©ª«¬\xAD®¯"
		L"°±²³´µ¶·¸¹º»¼½¾¿"
		L"ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏ"
		L"ÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
		L"àáâãäåæçèéêëìíîï"
		L"ðñòóôõö÷øùúûüýþÿ"
		;
	constexpr const auto& mbsLatin1ExtChars =
		"\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
		"\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
		"\xA0\xA1\xA2\xA3\xA4\xA5\xA6\xA7\xA8\xA9\xAA\xAB\xAC\xAD\xAE\xAF"
		"\xB0\xB1\xB2\xB3\xB4\xB5\xB6\xB7\xB8\xB9\xBA\xBB\xBC\xBD\xBE\xBF"
		"\xC0\xC1\xC2\xC3\xC4\xC5\xC6\xC7\xC8\xC9\xCA\xCB\xCC\xCD\xCE\xCF"
		"\xD0\xD1\xD2\xD3\xD4\xD5\xD6\xD7\xD8\xD9\xDA\xDB\xDC\xDD\xDE\xDF"
		"\xE0\xE1\xE2\xE3\xE4\xE5\xE6\xE7\xE8\xE9\xEA\xEB\xEC\xED\xEE\xEF"
		"\xF0\xF1\xF2\xF3\xF4\xF5\xF6\xF7\xF8\xF9\xFA\xFB\xFC\xFD\xFE\xFF"
		;

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, mbsLatin1ExtChars);
	EXPECT_THAT(result2.destination, StrEq(wcsLatin1ExtChars));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(mbsLatin1ExtChars));
	EXPECT_THAT(cresult2, IsTrue());

	// Unicodeに変換できない文字はない（Latin1仕様）

	// Unicodeから変換できない文字（Latin1仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "????????";

	const auto cresult4 = CCodeFactory::ConvertToCode(eCodeType, wcsKanaKanji);
	EXPECT_THAT(cresult4.destination, StrEq(mbsKanaKanji));
	EXPECT_THAT(cresult4, IsFalse());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf8)
{
	const auto eCodeType = CODE_UTF8;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-8仕様）
	constexpr const auto& mbsKanaKanji = u8"ｶﾅかなカナ漢字";
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, (LPCSTR)mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq((LPCSTR)mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf8_OracleImplementation)
{
	const auto eCodeType = CODE_CESU8;
	auto pCodeBase = CCodeFactory::CreateCodeBase( eCodeType );

	// 7bit ASCII範囲（等価変換）
	constexpr const auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-8仕様）
	constexpr const auto& mbsKanaKanji = u8"ｶﾅかなカナ漢字";
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, (LPCSTR)mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq((LPCSTR)mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf7)
{
	const auto eCodeType = CODE_UTF7;

	// 7bit ASCII範囲（UTF-7仕様）
	constexpr const auto& mbsAscii = "+AAEAAgADAAQABQAGAAcACA-\t\n+AAsADA-\r+AA4ADwAQABEAEgATABQAFQAWABcAGAAZABoAGwAcAB0AHgAf- +ACEAIgAjACQAJQAm-'()+ACoAKw-,-./0123456789:+ADsAPAA9AD4-?+AEA-ABCDEFGHIJKLMNOPQRSTUVWXYZ+AFsAXABdAF4AXwBg-abcdefghijklmnopqrstuvwxyz+AHsAfAB9AH4Afw-";
	constexpr const auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	auto result1 = CCodeFactory::LoadFromCode(eCodeType, mbsAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(mbsAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-7仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";
	constexpr const auto& mbsKanaKanji = "+/3b/hTBLMGowqzDKbyJbVw-";

	auto result2 = CCodeFactory::LoadFromCode(eCodeType, mbsKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(mbsKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());

	// UTF-7仕様
	constexpr const auto& wcsPlusPlus = L"C++";
	constexpr const auto& mbsPlusPlus = "C+-+-";

	auto result5 = CCodeFactory::LoadFromCode(eCodeType, mbsPlusPlus);
	EXPECT_THAT(result5.destination, StrEq(wcsPlusPlus));
	EXPECT_THAT(result5, IsTrue());

	auto cresult5 = CCodeFactory::ConvertToCode(eCodeType, result5.destination);
	EXPECT_THAT(cresult5.destination, StrEq(mbsPlusPlus));
	EXPECT_THAT(cresult5, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf16Le)
{
	const auto eCodeType = CODE_UTF16LE;

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	const auto bytesAscii = ToUtf16LeBytes(mbsAscii);
	auto result1 = CCodeFactory::LoadFromCode(eCodeType, bytesAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(bytesAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-16LE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	const auto bytesKanaKanji = ToUtf16LeBytes(wcsKanaKanji);
	auto result2 = CCodeFactory::LoadFromCode(eCodeType, bytesKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(bytesKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf16Be)
{
	const auto eCodeType = CODE_UTF16BE;

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	const auto bytesAscii = ToUtf16BeBytes(mbsAscii);
	auto result1 = CCodeFactory::LoadFromCode(eCodeType, bytesAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(bytesAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-16BE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	const auto bytesKanaKanji = ToUtf16BeBytes(wcsKanaKanji);
	auto result2 = CCodeFactory::LoadFromCode(eCodeType, bytesKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(bytesKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf32Le)
{
	const auto eCodeType = CODE_UTF32LE;

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	const auto bytesAscii = ToUtf32LeBytes(mbsAscii);
	auto result1 = CCodeFactory::LoadFromCode(eCodeType, bytesAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(bytesAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-32LE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	const auto bytesKanaKanji = ToUtf32LeBytes(wcsKanaKanji);
	auto result2 = CCodeFactory::LoadFromCode(eCodeType, bytesKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(bytesKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

/*!
 * @brief 文字コード変換のテスト
 */
TEST(CCodeBase, codeUtf32Be)
{
	const auto eCodeType = CODE_UTF32BE;

	// 7bit ASCII範囲（等価変換）
	constexpr auto& mbsAscii = "\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";
	constexpr auto& wcsAscii = L"\x01\x02\x03\x04\x05\x06\a\b\t\n\v\f\r\x0E\x0F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\x7F";

	const auto bytesAscii = ToUtf32BeBytes(mbsAscii);
	auto result1 = CCodeFactory::LoadFromCode(eCodeType, bytesAscii);
	EXPECT_THAT(result1.destination, StrEq(wcsAscii));
	EXPECT_THAT(result1, IsTrue());

	auto cresult1 = CCodeFactory::ConvertToCode(eCodeType, result1.destination);
	EXPECT_THAT(cresult1.destination, StrEq(bytesAscii));
	EXPECT_THAT(cresult1, IsTrue());

	// かな漢字の変換（UTF-32BE仕様）
	constexpr const auto& wcsKanaKanji = L"ｶﾅかなカナ漢字";

	const auto bytesKanaKanji = ToUtf32BeBytes(wcsKanaKanji);
	auto result2 = CCodeFactory::LoadFromCode(eCodeType, bytesKanaKanji);
	EXPECT_THAT(result2.destination, StrEq(wcsKanaKanji));
	EXPECT_THAT(result2, IsTrue());

	auto cresult2 = CCodeFactory::ConvertToCode(eCodeType, result2.destination);
	EXPECT_THAT(cresult2.destination, StrEq(bytesKanaKanji));
	EXPECT_THAT(cresult2, IsTrue());
}

//! EOLテストのためのフィクスチャクラス
class EolTest : public ::testing::TestWithParam<ECodeType> {};

/*!
 * @brief GetEol代替関数のテスト
 */
TEST_P(EolTest, test)
{
	const auto eCodeType = GetParam();
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	auto map = pCodeBase->GetEolDefinitions();
	for( const auto&[t,bin] : map ){
		CMemory m;
		pCodeBase->GetEol( &m, t );
		EXPECT_EQ(0, memcmp(m.GetRawPtr(), bin.data(), bin.length()));
		EXPECT_EQ(m.GetRawLength(), bin.length());
	}
}

/*!
 * @brief パラメータテストをインスタンス化する
 */
INSTANTIATE_TEST_CASE_P(ParameterizedTestEol
	, EolTest
	, ::testing::Values(
		CODE_SJIS,
		CODE_JIS,
		CODE_EUC,
		CODE_UNICODE,
		CODE_UTF8,
		CODE_UTF7,
		CODE_UNICODEBE,
		(ECodeType)12000,	// UTF-32LE
//		(ECodeType)12001,	// UTF-32BE実装は機能していないため除外
		CODE_CESU8,
		CODE_LATIN1
	)
);

//! BOMテストのためのパラメータ型
using BomTestParamType = std::tuple<ECodeType, std::string_view>;

//! BOMテストのためのフィクスチャクラス
class BomTest : public ::testing::TestWithParam<BomTestParamType> {};

/*!
 * @brief GetBom代替関数のテスト
 */
TEST_P(BomTest, test) {
	const auto eCodeType = std::get<0>(GetParam());
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	const auto str = std::get<1>(GetParam());
	BinarySequenceView expected(reinterpret_cast<const std::byte*>(str.data()), str.length());

	const auto actual = pCodeBase->GetBomDefinition();

	ASSERT_EQ(expected, actual);

	CMemory m;
	pCodeBase->GetBom( &m );
	EXPECT_EQ(0, memcmp(m.GetRawPtr(), actual.data(), actual.length()));
	EXPECT_EQ(m.GetRawLength(), actual.length());
}

/*!
 * @brief パラメータテストをインスタンス化する
 */
INSTANTIATE_TEST_CASE_P(ParameterizedTestBom
	, BomTest
	, ::testing::Values(
		BomTestParamType{ CODE_SJIS,		{} },				// 非Unicodeなので実施する意味はない
		BomTestParamType{ CODE_JIS,			{} },				// 非Unicodeなので実施する意味はない
		BomTestParamType{ CODE_EUC,			{} },				// 非Unicodeなので実施する意味はない
		BomTestParamType{ CODE_UNICODE,		"\xFF\xFE" },
		BomTestParamType{ CODE_UTF8,		"\xEF\xBB\xBF" },
		BomTestParamType{ CODE_UTF7,		"+/v8-" },			// 対象外なので実施する意味はない
		BomTestParamType{ CODE_UNICODEBE,	"\xFE\xFF" },
		BomTestParamType{ CODE_LATIN1,		{} },				// 非Unicodeなので実施する意味はない
		BomTestParamType{ CODE_CESU8,		"\xEF\xBB\xBF" }
	)
);

} // namespace  convert

//! 表示用16進変換テストのためのフィクスチャクラス
class CodeToHexTest : public ::testing::TestWithParam<ECodeType> {};

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST_P(CodeToHexTest, test)
{
	const auto eCodeType = GetParam();
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// Unicodeコードポイントを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = true;
	sStatusbar.m_bDispUniInJis = true;
	sStatusbar.m_bDispUniInEuc = true;
	sStatusbar.m_bDispUtf8Codepoint = true;
	sStatusbar.m_bDispSPCodepoint = true;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"U+3042", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"U+1F6B9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());
}

/*!
 * @brief パラメータテストをインスタンス化する
 */
INSTANTIATE_TEST_CASE_P(ParameterizedTestToHex
	, CodeToHexTest
	, ::testing::Values(
		CODE_SJIS,
		CODE_JIS,
		CODE_EUC,
		CODE_UNICODE,
		CODE_UTF8,
		CODE_UTF7,
		CODE_UNICODEBE,
		(ECodeType)12000,
		(ECodeType)12001,
		CODE_CESU8,
		CODE_LATIN1
	)
);

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST(CCodeBase, SjisToHex)
{
	const auto eCodeType = CODE_SJIS;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"82A0", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"D83DDEB9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());
}

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST(CCodeBase, JisToHex)
{
	const auto eCodeType = CODE_JIS;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"2422", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"D83DDEB9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());
}

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST(CCodeBase, EucToHex)
{
	const auto eCodeType = CODE_EUC;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"A4A2", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"D83DDEB9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());
}

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST(CCodeBase, Utf8ToHex)
{
	const auto eCodeType = CODE_UTF8;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"E38182", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"F09F9AB9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());

	// IVS(Ideographic Variation Sequence) 「葛󠄀」（葛󠄀城市の葛󠄀、下がヒ）
	EXPECT_STREQ(L"E8919BF3A08480", pCodeBase->CodeToHex(L"\U0000845B\U000E0100"/*葛󠄀*/, sStatusbar).c_str());
}

/*!
 * @brief UnicodeToHex代替関数のテスト
 */
TEST(CCodeBase, Latin1ToHex)
{
	const auto eCodeType = CODE_LATIN1;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	// 日本語 ひらがな「あ」（文字セットがサポートしない文字でも統一仕様）
	EXPECT_STREQ(L"U+3042", pCodeBase->CodeToHex(L"あ", sStatusbar).c_str());

	// カラー絵文字「男性のシンボル」（サロゲートペア）
	EXPECT_STREQ(L"D83DDEB9", pCodeBase->CodeToHex(L"\U0001F6B9", sStatusbar).c_str());
}

TEST(CCodeBase, UnicodeToHex)
{
	const auto eCodeType = CODE_UNICODE;
	auto pCodeBase = CCodeFactory::CreateCodeBase(eCodeType);

	// 特定コードのマルチバイトを表示する設定
	CommonSetting_Statusbar sStatusbar;
	sStatusbar.m_bDispUniInSjis = false;
	sStatusbar.m_bDispUniInJis = false;
	sStatusbar.m_bDispUniInEuc = false;
	sStatusbar.m_bDispUtf8Codepoint = false;
	sStatusbar.m_bDispSPCodepoint = false;

	sStatusbar.m_bDispSPCodepoint = true;
	EXPECT_STREQ(L"845B, U+E0100", pCodeBase->CodeToHex(L"\U0000845B\U000E0100"/*葛󠄀*/, sStatusbar).c_str());

	sStatusbar.m_bDispSPCodepoint = false;
	EXPECT_STREQ(L"845B, DB40DD00", pCodeBase->CodeToHex(L"\U0000845B\U000E0100"/*葛󠄀*/, sStatusbar).c_str());
}
