
#include "pch.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace	Loquaty ;

namespace LoquatyTest
{
	TEST_CLASS(LoquatyIntegerTest)
	{
	public:
		TEST_METHOD(ToString)
		{
			LString	str ;
			str.SetIntegerOf( 1234567890 ) ;
			Assert::IsTrue( str == L"1234567890" ) ;

			str.SetIntegerOf( -987654321 ) ;
			Assert::IsTrue( str == L"-987654321" ) ;

			str.SetIntegerOf( 1234, 7 ) ;
			Assert::IsTrue( str == L"0001234" ) ;

			str.SetIntegerOf( -1234, 7 ) ;
			Assert::IsTrue( str == L"-001234" ) ;

			str.SetIntegerOf( 0x123456789ABCDEF, 0, 16 ) ;
			Assert::IsTrue( str == L"123456789ABCDEF" ) ;

			str.SetIntegerOf( -0x123456789ABCDEF, 0, 16 ) ;
			Assert::IsTrue( str == L"-123456789ABCDEF" ) ;

			LPointerObj::ExprIntAsString( str, 123 ) ;
			Assert::IsTrue( str == L"123" ) ;
			LPointerObj::ExprIntAsString( str, 128 ) ;
			Assert::IsTrue( str == L"0x80" ) ;
			LPointerObj::ExprIntAsString( str, -128 ) ;
			Assert::IsTrue( str == L"-128" ) ;
			LPointerObj::ExprIntAsString( str, 256 ) ;
			Assert::IsTrue( str == L"0x100" ) ;
		}
	};

	TEST_CLASS(LoquatyDoubleTest)
	{
	public:
		TEST_METHOD(ToString)
		{
			LString	str ;
			str.SetNumberOf( std::nanf("") ) ;
			Assert::IsTrue( str == L"nan" ) ;

			str.SetNumberOf( std::numeric_limits<double>::infinity() ) ;
			Assert::IsTrue( str == L"nan" ) ;

			str.SetNumberOf( 12345.0 ) ;
			Assert::IsTrue( str == L"12345." ) ;

			str.SetNumberOf( -12345.678 ) ;
			Assert::IsTrue( str == L"-12345.678" ) ;

			str.SetNumberOf( -123.456, 2 ) ;
			Assert::IsTrue( str == L"-123.46" ) ;

			str.SetNumberOf( 1.23456789e+16, 0, true ) ;
			Assert::IsTrue( str == L"1.23456789E+16" ) ;

			str.SetNumberOf( -1.23456789e-16, 0, true ) ;
			Assert::IsTrue( str == L"-1.23456789E-16" ) ;
		}
	};

	TEST_CLASS(LoquatyStringTest)
	{
	public:
		TEST_METHOD(Middle)
		{
			LString	str = L"abcdefg" ;
			Assert::IsTrue( str.Middle(1,3) == L"bcd" ) ;
			Assert::IsTrue( str.Middle(4,5) == L"efg" ) ;
		}

		TEST_METHOD(Find)
		{
			LString	str = L"abcdefgabc" ;
			Assert::IsTrue( str.Find(L"abc") == 0 ) ;
			Assert::IsTrue( str.Find(L"abc",1) == 7 ) ;
			Assert::IsTrue( str.Find(L"cde") == 2 ) ;
			Assert::IsTrue( str.Find(L"cde",3) == -1 ) ;
			Assert::IsTrue( str.Find(L"h") == -1 ) ;
		}

		TEST_METHOD(Replace)
		{
			LString	str = L"abc%1def%1" ;
			Assert::IsTrue( str.Replace(L"%2",L"XXXX") == L"abc%1def%1" ) ;
			Assert::IsTrue( str.Replace(L"%1",L"XXXX") == L"abcXXXXdefXXXX" ) ;

			str = L"abcXXXXdXXXXXef" ;
			Assert::IsTrue( str.Replace(L"XXXX",L"%") == L"abc%d%Xef" ) ;
		}

		TEST_METHOD(AsInteger)
		{
			LStringObj	strObj( nullptr ) ;
			LLong	val ;
			strObj.m_string = L" 1234" ;
			strObj.AsInteger( val ) ;
			Assert::IsTrue( val == 1234 ) ;

			strObj.m_string = L" + 12345a6" ;
			strObj.AsInteger( val ) ;
			Assert::IsTrue( val == 12345 ) ;

			strObj.m_string = L"- 12" ;
			strObj.AsInteger( val ) ;
			Assert::IsTrue( val == -12 ) ;

			strObj.m_string = L" -12 34" ;
			strObj.AsInteger( val ) ;
			Assert::IsTrue( val == -12 ) ;

			strObj.m_string = L" a" ;
			Assert::IsFalse( strObj.AsInteger( val ) ) ;
			Assert::IsTrue( val == 0 ) ;
		}

		TEST_METHOD(AsDouble)
		{
			LStringObj	strObj( nullptr ) ;
			LDouble	val ;
			strObj.m_string = L" 1234" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( val == 1234.0 ) ;

			strObj.m_string = L" + 12345." ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( val == 12345.0 ) ;

			strObj.m_string = L"- 12" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( val == -12.0 ) ;

			strObj.m_string = L" -12 34" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( val == -12.0 ) ;

			strObj.m_string = L" + 123.45.6" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - 123.45) < 1.0e-12 ) ;

			strObj.m_string = L"-12.34+5.6" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - (-12.34)) < 1.0e-12 ) ;

			strObj.m_string = L"a-12.34" ;
			Assert::IsFalse( strObj.AsDouble( val ) ) ;
			Assert::IsTrue( val == 0.0 ) ;

			strObj.m_string = L"-123E+2" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - (-12300.0)) < 1.0e-12 ) ;

			strObj.m_string = L"123E+3.6" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - (123000.0)) < 1.0e-12 ) ;

			strObj.m_string = L"-123e-3 6" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - (-0.123)) < 1.0e-12 ) ;

			strObj.m_string = L"+123e-3E6" ;
			strObj.AsDouble( val ) ;
			Assert::IsTrue( fabs(val - (0.123)) < 1.0e-12 ) ;
		}

		TEST_METHOD(Convert)
		{
			LString	strOrg = L"いろはにほへと" ;

			std::vector<std::uint8_t>	utf8 ;
			strOrg.ToUTF8( utf8 ) ;

			LString	strCvt8 ;
			strCvt8.FromUTF8( utf8 ) ;
			Assert::IsTrue( strOrg == strCvt8 ) ;

			std::vector<std::uint16_t>	utf16 ;
			strOrg.ToUTF16( utf16 ) ;
	
			LString	strCvt16 ;
			strCvt16.FromUTF16( utf16 ) ;
			Assert::IsTrue( strOrg == strCvt16 ) ;

			std::vector<std::uint32_t>	utf32 ;
			strOrg.ToUTF32( utf32 ) ;
	
			LString	strCvt32 ;
			strCvt32.FromUTF32( utf32 ) ;
			Assert::IsTrue( strOrg == strCvt32 ) ;
		}

		TEST_METHOD(Reverse)
		{
			LString	strOrg = L"いろはに♡ほへと" ;
			LString	strRev = strOrg.Reverse() ;
			LString	strRet = strRev.Reverse() ;

			Assert::IsTrue( strRev == L"とへほ♡にはろい" ) ;
			Assert::IsTrue( strRet == strOrg ) ;
			Assert::IsTrue( strOrg * 0 == L"" ) ;
			Assert::IsTrue( strOrg * -2 == strRev + strRev ) ;
			Assert::IsTrue( strOrg * 3 == strOrg + strOrg + strOrg ) ;
		}

		TEST_METHOD(Slice)
		{
			LString	strTest = L"あいうえお,かきくけこ," ;

			std::vector<LString>	vSliced ;
			strTest.Slice( vSliced, L"," ) ;

			Assert::IsTrue( vSliced.size() == 2 ) ;
			Assert::IsTrue( vSliced.at(0) == L"あいうえお" ) ;
			Assert::IsTrue( vSliced.at(1) == L"かきくけこ" ) ;

			strTest = L"あいうえおかきくけこ" ;

			vSliced.clear() ;
			strTest.Slice( vSliced, L"::" ) ;
			Assert::IsTrue( vSliced.size() == 1 ) ;
			Assert::IsTrue( vSliced.at(0) == strTest ) ;

			strTest = L"あいうえお::かきくけこ" ;

			vSliced.clear() ;
			strTest.Slice( vSliced, L"::" ) ;
			Assert::IsTrue( vSliced.size() == 2 ) ;
			Assert::IsTrue( vSliced.at(0) == L"あいうえお" ) ;
			Assert::IsTrue( vSliced.at(1) == L"かきくけこ" ) ;
		}
	};

	TEST_CLASS(LoquatyStringParserTest)
	{
	public:
		TEST_METHOD(SpaceChar)
		{
			Assert::IsTrue( LStringParser::IsCharSpace( L'\t' ) ) ;
			Assert::IsTrue( LStringParser::IsCharSpace( L'\r' ) ) ;
			Assert::IsTrue( LStringParser::IsCharSpace( L'\n' ) ) ;
			Assert::IsTrue( LStringParser::IsCharSpace( L' ' ) ) ;
			Assert::IsFalse( LStringParser::IsCharSpace( L'0' ) ) ;
			Assert::IsFalse( LStringParser::IsCharSpace( L'A' ) ) ;
			Assert::IsFalse( LStringParser::IsCharSpace( L'*' ) ) ;
		}

		TEST_METHOD(NumberChar)
		{
			Assert::IsTrue( LStringParser::IsCharNumber( L'0' ) ) ;
			Assert::IsTrue( LStringParser::IsCharNumber( L'1' ) ) ;
			Assert::IsTrue( LStringParser::IsCharNumber( L'2' ) ) ;
			Assert::IsTrue( LStringParser::IsCharNumber( L'3' ) ) ;
			Assert::IsFalse( LStringParser::IsCharNumber( L'A' ) ) ;
			Assert::IsFalse( LStringParser::IsCharNumber( L'a' ) ) ;
			Assert::IsFalse( LStringParser::IsCharNumber( L'_' ) ) ;
			Assert::IsFalse( LStringParser::IsCharNumber( L'*' ) ) ;
		}

		TEST_METHOD(PunctuationChar)
		{
			wchar_t	wszMark[] = L" !\"#$%&\'()*+,-./:;<=>[\\]^`{|}~" ;
			for ( int i = 0; wszMark[i] != 0; i ++ )
			{
				Assert::IsTrue( LStringParser::IsPunctuation( wszMark[i] ) ) ;
			}
			Assert::IsFalse( LStringParser::IsPunctuation( L'A' ) ) ;
			Assert::IsFalse( LStringParser::IsPunctuation( L'a' ) ) ;
			Assert::IsFalse( LStringParser::IsPunctuation( L'_' ) ) ;
			Assert::IsFalse( LStringParser::IsPunctuation( L'@' ) ) ;
		}

		TEST_METHOD(SpecialMarkChar)
		{
			wchar_t	wszMark[] = L"\"\'(),;<>[]{}" ;
			for ( int i = 0; wszMark[i] != 0; i ++ )
			{
				Assert::IsTrue( LStringParser::IsSpecialMark( wszMark[i] ) ) ;
			}
			Assert::IsFalse( LStringParser::IsSpecialMark( L'A' ) ) ;
			Assert::IsFalse( LStringParser::IsSpecialMark( L'a' ) ) ;
			Assert::IsFalse( LStringParser::IsSpecialMark( L'_' ) ) ;
			Assert::IsFalse( LStringParser::IsSpecialMark( L'@' ) ) ;
		}

		TEST_METHOD(ValidAsSymbol)
		{
			Assert::IsTrue( LStringParser::IsValidAsSymbol( L"ABC" ) ) ;
			Assert::IsTrue( LStringParser::IsValidAsSymbol( L"ABC123" ) ) ;
			Assert::IsTrue( LStringParser::IsValidAsSymbol( L"abc" ) ) ;
			Assert::IsTrue( LStringParser::IsValidAsSymbol( L"_abc" ) ) ;
			Assert::IsTrue( LStringParser::IsValidAsSymbol( L"@abc_123" ) ) ;

			Assert::IsFalse( LStringParser::IsValidAsSymbol( L" Abc" ) ) ;
			Assert::IsFalse( LStringParser::IsValidAsSymbol( L"1ABC" ) ) ;
			Assert::IsFalse( LStringParser::IsValidAsSymbol( L"abc " ) ) ;
			Assert::IsFalse( LStringParser::IsValidAsSymbol( L"@abc.123" ) ) ;
		}

		TEST_METHOD(EncodeStringLiteral)
		{
			Assert::IsTrue( LStringParser::EncodeStringLiteral( L" 123abc" ) == L" 123abc" ) ;
			Assert::IsTrue( LStringParser::EncodeStringLiteral( L"\n 123abc" ) == L"\\n 123abc" ) ;
			Assert::IsTrue( LStringParser::EncodeStringLiteral( L" 123abc\\" ) == L" 123abc\\\\" ) ;
			Assert::IsTrue( LStringParser::EncodeStringLiteral( L"\" \x1b,12\r3a\"bc\'" ) == L"\\\" \\x1B,12\\r3a\\\"bc\\\'" ) ;
		}

		TEST_METHOD(DecodeStringLiteral)
		{
			Assert::IsTrue( LStringParser::DecodeStringLiteral( L" 123abc" ) == L" 123abc" ) ;
			Assert::IsTrue( LStringParser::DecodeStringLiteral( L"\\n 123abc" ) == L"\n 123abc" ) ;
			Assert::IsTrue( LStringParser::DecodeStringLiteral( L" 123abc\\\\" ) == L" 123abc\\" ) ;
			Assert::IsTrue( LStringParser::DecodeStringLiteral( L"\\\" \\x1B\\x1b,12\\r3a\\\"bc\\\'" ) == L"\" \x1b\x1b,12\r3a\"bc\'" ) ;
		}

		TEST_METHOD(StringParser)
		{
			LStringParser	parser = L"0a \t\n [@ ABC 123 abc+=def>>=ghi.*[j>-- " ;
			LString	token ;
			Assert::IsTrue( parser.NextChar() == L'0' ) ;
			Assert::IsTrue( parser.CurrentChar() == L'a' ) ;
			Assert::IsTrue( parser.NextChar() == L'a' ) ;
			Assert::IsFalse( parser.IsEndOfString() ) ;
			Assert::IsTrue( parser.PassSpace() ) ;
			Assert::IsTrue( parser.HasNextChars(L"]>a") == 0 ) ;
			Assert::IsTrue( parser.HasNextChars(L"<[@") == L'[' ) ;
			Assert::IsTrue( parser.HasNextChars(L"<[@") == L'@' ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenSymbol ) ;
			Assert::IsTrue( token == L"ABC" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenNumber ) ;
			Assert::IsTrue( token == L"123" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenSymbol ) ;
			Assert::IsTrue( token == L"abc" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L"+=" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenSymbol ) ;
			Assert::IsTrue( token == L"def" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L">>=" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenSymbol ) ;
			Assert::IsTrue( token == L"ghi" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L".*" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L"[" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenSymbol ) ;
			Assert::IsTrue( token == L"j" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L">" ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenMark ) ;
			Assert::IsTrue( token == L"--" ) ;
			Assert::IsFalse( parser.PassSpace() ) ;
			Assert::IsTrue( parser.NextToken(token) == LStringParser::tokenNothing ) ;
		}

		TEST_METHOD(SourceComment)
		{
			LSourceFile	source =
				L" abc // line comment\n"
				L"  // comment1\n"
				L"	//comment2\n"
				L"	def\n"
				L"	///////////\n"
				L"	// comment3\n"
				L"	///////////\n"
				L"\n"
				L"	ghi\n"
				L"	/******** comment4\n"
				L"comment5*************/\n"
				L"	jkl\n"
				L"	/* comment5 */\n"
				L"\n"
				L"	mno\n" ;
			Assert::IsTrue( source.HasNextToken( L"abc" ) ) ;
			source.PassSpaceInLine() ;
			Assert::IsTrue( source.GetCommentBefore() == L"line comment\n" ) ;
			source.ClearComment() ;
			Assert::IsTrue( source.HasNextToken( L"def" ) ) ;
			Assert::IsTrue( source.GetCommentBefore() == L"comment1\ncomment2\n" ) ;
			Assert::IsTrue( source.HasNextToken( L"ghi" ) ) ;
			Assert::IsTrue( source.GetCommentBefore() == L"comment3\n" ) ;
			Assert::IsTrue( source.HasNextToken( L"jkl" ) ) ;
			Assert::IsTrue( source.GetCommentBefore() == L"comment4\ncomment5" ) ;
			Assert::IsTrue( source.HasNextToken( L"mno" ) ) ;
			Assert::IsTrue( source.GetCommentBefore() == L"comment5" ) ;
		}
	};

	TEST_CLASS(LoquatyXMLDocParserTest)
	{
		TEST_METHOD(EncodeXMLString)
		{
			Assert::IsTrue
				( LXMLDocParser::EncodeXMLString( L" abc  <>  &\n test " )
						== L"&#32;abc &#32;&lt;&gt; &#32;&amp;&#10; test&#32;" ) ;
			Assert::IsTrue
				( LXMLDocParser::EncodeXMLString( L" abc    <>  &\n   test ", L"\t" )
						== L"&#32;abc &#32; &#32;&lt;&gt; &#32;&amp;&#10;\r\n\t&#32; &#32;test&#32;" ) ;
		}

		TEST_METHOD(DecodeXMLString)
		{
			Assert::IsTrue
				( LXMLDocParser::DecodeXMLString
					( L"  &#32;abc &#32; &#32;&lt;&gt; &#32;&amp;&#10;\r\n\t&#32; &#32;test&#32;  " )
						== L" abc    <>  &\n   test " ) ;
			Assert::IsTrue
				( LXMLDocParser::DecodeXMLString
					( L"  &#32;abc &#32; &#32;&lt;&gt; &#32;&amp;&#10;\r\n\t&#32; &#32;test&#32;  ", true )
						== L"   abc    <>  &\n\r\n\t   test   " ) ;
		}

		TEST_METHOD(LoadFile)
		{
			LXMLDocParser	xmlParser ;
			Assert::IsTrue( xmlParser.LoadTextFile( L"..\\..\\..\\LoquatyTest\\test\\test.xml" ) ) ;

			LXMLDocPtr	pDoc = xmlParser.ParseDocument() ;
			Assert::IsTrue( pDoc != nullptr ) ;
			Assert::IsTrue( xmlParser.GetErrorCount() == 0 ) ;

			LXMLDocPtr	pTag = pDoc->GetTagPathAs( L"html>head>title" ) ;
			Assert::IsTrue( pTag != nullptr ) ;
			Assert::IsTrue( pTag->GetTextElement() == L"Affine" ) ;

			Assert::IsTrue( xmlParser.SaveToFile
				( L"..\\..\\..\\LoquatyTest\\test\\test_save.xml", *pDoc, 60, L"" ) ) ;
		}
	};

	TEST_CLASS(LoquatySymbolTest)
	{
		TEST_METHOD(TestReservedWord)
		{
			for ( int i = 0; i < Symbol::rwiReservedWordCount; i ++ )
			{
				Assert::IsTrue( Symbol::s_ReservedWordDescs[i].rwIndex == i ) ;
			}
		}

		TEST_METHOD(TestOperator)
		{
			for ( int i = 0; i < Symbol::opOperatorCount; i ++ )
			{
				Assert::IsTrue( Symbol::s_OperatorDescs[i].opIndex == i ) ;
			}
		}

		TEST_METHOD(TestExceptionClass)
		{
			for ( int i = 0; i < errorExceptionCount; i ++ )
			{
				ErrorMessageIndex	err = (ErrorMessageIndex) i ;
				Assert::IsTrue( Symbol::s_ExceptionDesc[i].index == err ) ;
			}
		}

		TEST_METHOD(TestErrorMessage)
		{
			for ( int i = 0; i < errorMessageTotalCount; i ++ )
			{
				ErrorMessageIndex	err = (ErrorMessageIndex) i ;
				Assert::IsTrue( Symbol::s_ErrorDesc[i].index == err ) ;
			}
		}
	};

	TEST_CLASS(LoquatyTypeTest)
	{
	public:
		TEST_METHOD(AsPrimitive)
		{
			Assert::IsTrue( LType::AsPrimitive( L"boolean" ) == LType::typeBoolean ) ;
			Assert::IsTrue( LType::AsPrimitive( L"byte" ) == LType::typeInt8 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"short" ) == LType::typeInt16 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"int" ) == LType::typeInt32 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"uint" ) == LType::typeUint32 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"long" ) == LType::typeInt64 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"float" ) == LType::typeFloat ) ;
			Assert::IsTrue( LType::AsPrimitive( L"double" ) == LType::typeDouble ) ;
			Assert::IsTrue( LType::AsPrimitive( L"int8" ) == LType::typeInt8 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"int16" ) == LType::typeInt16 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"int32" ) == LType::typeInt32 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"int64" ) == LType::typeInt64 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"uint8" ) == LType::typeUint8 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"uint16" ) == LType::typeUint16 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"uint32" ) == LType::typeUint32 ) ;
			Assert::IsTrue( LType::AsPrimitive( L"uint64" ) == LType::typeUint64 ) ;
			Assert::IsTrue( LType::AsPrimitive( nullptr ) == LType::typeObject ) ;
			Assert::IsTrue( LType::AsPrimitive( L"Object" ) == LType::typeObject ) ;
		}

		TEST_METHOD(ParseType)
		{
			LVirtualMachine	vm ;
			vm.Initialize() ;

			LStringParser	spIntArr56 = L"int[5][6]" ;
			LType			typeIntArr56 ;
			spIntArr56.NextTypeExpr( typeIntArr56, true, vm, nullptr ) ;
			Assert::IsTrue( typeIntArr56.GetTypeName() == L"int[5][6]" ) ;
			Assert::IsTrue( typeIntArr56.GetBaseDataType().GetTypeName() == L"int" ) ;
			Assert::IsTrue( typeIntArr56.GetRefElementType().GetTypeName() == L"int[6]" ) ;

			LStringParser	spIntArr6Ptr = L"int[6]*" ;
			LType			typeIntArr6Ptr ;
			spIntArr6Ptr.NextTypeExpr( typeIntArr6Ptr, true, vm, nullptr ) ;
			Assert::IsTrue( typeIntArr6Ptr.GetTypeName() == L"int[6]*" ) ;
			Assert::IsTrue( typeIntArr6Ptr.GetBaseDataType().GetTypeName() == L"int" ) ;
			Assert::IsTrue( typeIntArr6Ptr.GetRefElementType().GetTypeName() == L"int[6]" ) ;

			LStringParser	spIntPtrArr = L"int*[]" ;
			LType			typeIntPtrArr ;
			spIntPtrArr.NextTypeExpr( typeIntPtrArr, true, vm, nullptr ) ;
			Assert::IsTrue( typeIntPtrArr.GetTypeName() == L"int*[]" ) ;
			Assert::IsTrue( typeIntPtrArr.GetBaseDataType().GetTypeName() == L"int*[]" ) ;
			Assert::IsTrue( typeIntPtrArr.GetRefElementType().GetTypeName() == L"int*" ) ;

			LStringParser	spIntObjPtr = L"Integer*[]" ;
			LType			typeIntObjPtr ;
			spIntObjPtr.NextTypeExpr( typeIntObjPtr, true, vm, nullptr ) ;
			Assert::IsTrue( typeIntObjPtr.GetTypeName() == L"Integer" ) ;

			LStringParser	spIntObjArrPtr = L"Integer[]*" ;
			LType			typeIntObjArrPtr ;
			spIntObjArrPtr.NextTypeExpr( typeIntObjArrPtr, true, vm, nullptr ) ;
			Assert::IsTrue( typeIntObjArrPtr.GetTypeName() == L"Integer[]" ) ;

			LStringParser	spMapIntObjArr = L"Map<Integer[]>" ;
			LType			typeMapIntObjArr ;
			spMapIntObjArr.NextTypeExpr( typeMapIntObjArr, true, vm, nullptr ) ;
			Assert::IsTrue( typeMapIntObjArr.GetTypeName() == L"Map<Integer[]>" ) ;

			LStringParser	spMapIntArr = L"Map<int[10]>" ;
			LType			typeMapIntArr ;
			Assert::IsFalse( spMapIntArr.NextTypeExpr( typeMapIntArr, true, vm, nullptr ) ) ;

			LStringParser	spMapIntArrPtr = L"Map<int[10]*>" ;
			LType			typeMapIntArrPtr ;
			Assert::IsTrue( spMapIntArrPtr.NextTypeExpr( typeMapIntArrPtr, true, vm, nullptr ) ) ;
			Assert::IsTrue( typeMapIntArrPtr.GetTypeName() == L"Map<int[10]*>" ) ;
		}
	};

	TEST_CLASS(LoquatyArrayBufferTest)
	{
	public:
		TEST_METHOD(Pointer)
		{
			std::shared_ptr<LArrayBufStorage>
				pBuf1 = std::make_shared<LArrayBufStorage>( 0x100 ) ;
			std::shared_ptr<LArrayBufStorage>
				pBuf2 = std::make_shared<LArrayBufStorage>( 0x100 ) ;

			LPointerObj	ptr1( nullptr ) ;
			LPointerObj	ptr2( nullptr ) ;
			LPointerObj	ptr3( nullptr ) ;
			ptr1.SetPointer( pBuf1, 0x80, 0x10 ) ;
			ptr2.SetPointer( pBuf1, 0x40, 0x10 ) ;
			ptr3.SetPointer( pBuf2, 0x40, 0x10 ) ;
			Assert::IsTrue( ptr1.GetPointer() == ptr2.GetPointer() + 0x40 ) ;
			Assert::IsTrue( ptr1.GetPointer(0,0x10) == pBuf1->data() + 0x80 ) ;
			Assert::IsTrue( ptr1.GetPointer(0,0x11) == nullptr ) ;

			pBuf1->resize( 0x60 ) ;
			pBuf1->NotifyBufferReallocation() ;
			Assert::IsTrue( ptr1.GetPointer() == nullptr ) ;

			pBuf1->resize( 0x100 ) ;
			pBuf1->NotifyBufferReallocation() ;
			Assert::IsTrue( ptr2.GetPointer() == pBuf1->data() + 0x40 ) ;
			Assert::IsTrue( ptr1.GetPointer() == nullptr ) ;

			ptr1.SetPointer( pBuf2, 0x80, 0x10 ) ;
			Assert::IsTrue( ptr1.GetPointer() == ptr3.GetPointer() + 0x40 ) ;

			pBuf1->resize( 0 ) ;
			pBuf1->NotifyBufferReallocation() ;
			pBuf2->resize( 0xA0 ) ;
			pBuf2->NotifyBufferReallocation() ;
			Assert::IsTrue( ptr1.GetPointer() == pBuf2->data() + 0x80 ) ;
			Assert::IsTrue( ptr2.GetPointer() == nullptr ) ;
			Assert::IsTrue( ptr3.GetPointer() == pBuf2->data() + 0x40 ) ;

			pBuf2->resize( 0 ) ;
			pBuf2->NotifyBufferReallocation() ;
			Assert::IsTrue( ptr1.GetPointer() == nullptr ) ;
			Assert::IsTrue( ptr3.GetPointer() == nullptr ) ;
		}

		TEST_METHOD(LoadMem)
		{
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeBoolean] == &LPointerObj::LoadBooleanAsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeInt8] == &LPointerObj::LoadInt8AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeUint8] == &LPointerObj::LoadUint8AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeInt16] == &LPointerObj::LoadInt16AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeUint16] == &LPointerObj::LoadUint16AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeInt32] == &LPointerObj::LoadInt32AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeUint32] == &LPointerObj::LoadUint32AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeInt64] == &LPointerObj::LoadInt64AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeUint64] == &LPointerObj::LoadUint64AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeFloat] == &LPointerObj::LoadFloatAsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsLong[LType::typeDouble] == &LPointerObj::LoadDoubleAsLong ) ;

			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeBoolean] == &LPointerObj::LoadBooleanAsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeInt8] == &LPointerObj::LoadInt8AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeUint8] == &LPointerObj::LoadUint8AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeInt16] == &LPointerObj::LoadInt16AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeUint16] == &LPointerObj::LoadUint16AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeInt32] == &LPointerObj::LoadInt32AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeUint32] == &LPointerObj::LoadUint32AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeInt64] == &LPointerObj::LoadInt64AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeUint64] == &LPointerObj::LoadUint64AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeFloat] == &LPointerObj::LoadFloatAsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfLoadAsDouble[LType::typeDouble] == &LPointerObj::LoadDoubleAsDouble ) ;
		}

		TEST_METHOD(StoreMem)
		{
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeBoolean] == &LPointerObj::StoreBooleanAsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeInt8] == &LPointerObj::StoreInt8AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeUint8] == &LPointerObj::StoreUint8AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeInt16] == &LPointerObj::StoreInt16AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeUint16] == &LPointerObj::StoreUint16AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeInt32] == &LPointerObj::StoreInt32AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeUint32] == &LPointerObj::StoreUint32AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeInt64] == &LPointerObj::StoreInt64AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeUint64] == &LPointerObj::StoreUint64AsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeFloat] == &LPointerObj::StoreFloatAsLong ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsLong[LType::typeDouble] == &LPointerObj::StoreDoubleAsLong ) ;

			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeBoolean] == &LPointerObj::StoreBooleanAsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeInt8] == &LPointerObj::StoreInt8AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeUint8] == &LPointerObj::StoreUint8AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeInt16] == &LPointerObj::StoreInt16AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeUint16] == &LPointerObj::StoreUint16AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeInt32] == &LPointerObj::StoreInt32AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeUint32] == &LPointerObj::StoreUint32AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeInt64] == &LPointerObj::StoreInt64AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeUint64] == &LPointerObj::StoreUint64AsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeFloat] == &LPointerObj::StoreFloatAsDouble ) ;
			Assert::IsTrue( LPointerObj::s_pnfStoreAsDouble[LType::typeDouble] == &LPointerObj::StoreDoubleAsDouble ) ;
		}
	};

	TEST_CLASS(LoquatyMapTest)
	{
		TEST_METHOD(Map)
		{
			LMapObj	map(nullptr) ;
			map.SetElementAs( L"c", new LIntegerObj( nullptr, 1 ) ) ;
			map.SetElementAs( L"b", new LIntegerObj( nullptr, 2 ) ) ;
			map.SetElementAs( L"a", new LIntegerObj( nullptr, 3 ) ) ;
			map.SetElementAs( L"d", new LIntegerObj( nullptr, 4 ) ) ;

			Assert::IsTrue( map.FindElementAs( L"c" ) == 0 ) ;
			Assert::IsTrue( map.FindElementAs( L"b" ) == 1 ) ;
			Assert::IsTrue( map.FindElementAs( L"a" ) == 2 ) ;
			Assert::IsTrue( map.FindElementAs( L"d" ) == 3 ) ;

			map.RemoveElementAs( L"b" ) ;

			Assert::IsTrue( map.FindElementAs( L"c" ) == 0 ) ;
			Assert::IsTrue( map.FindElementAs( L"b" ) == -1 ) ;
			Assert::IsTrue( map.FindElementAs( L"a" ) == 1 ) ;
			Assert::IsTrue( map.FindElementAs( L"d" ) == 2 ) ;
		}
	};

	TEST_CLASS(LoquatyExprValueArrayTest)
	{
	public:
		TEST_METHOD(Temporary)
		{
			LExprValueArray	xva ;
			LExprValuePtr	x1 = xva.PushLong( 1 ) ;
			LExprValuePtr	x2 = xva.PushLong( 2 ) ;
			x2->SetOption( LExprValue::exprRefByIndex, xva.PushLong( 3 ), nullptr ) ;
			LExprValuePtr	x4 = xva.PushLong( 4 ) ;

			Assert::IsTrue( xva.CountBackTemporaries() == 0 ) ;

			TestMove( xva, std::move(x4), 1 ) ;

			x2 = nullptr ;
			Assert::IsTrue( xva.CountBackTemporaries() == 3 ) ;

			TestMove( xva, std::move(x1), 4 ) ;
		}

		void TestMove( LExprValueArray& xva, LExprValuePtr p, size_t n )
		{
			Assert::IsTrue( xva.CountBackTemporaries() == n - 1 ) ;

			p = nullptr ;
			Assert::IsTrue( xva.CountBackTemporaries() == n ) ;
		}

	};

	TEST_CLASS(LoquatyMnemonicTest)
	{
		TEST_METHOD(MnemonicTable)
		{
			for ( int i = 0; i < LCodeBuffer::codeInstructionCount; i ++ )
			{
				if ( LCodeBuffer::s_MnemonicInfos[i].code != i )
				Assert::IsTrue( LCodeBuffer::s_MnemonicInfos[i].code == i ) ;

				LCodeBuffer::Word	word ;
				word.code = (LCodeBuffer::InstructionCode) i ;
				word.sop1 = 0 ;
				word.sop2 = 0 ;
				word.op3 = 0 ;
				word.imm = 0 ;
				word.imop.value.longValue = 0 ;

				if ( LCodeBuffer::FormatMnemonic
						( LCodeBuffer::s_MnemonicInfos[i].mnemonic, word ).Find( L"?" ) >= 0 )
				Assert::IsTrue
					( LCodeBuffer::FormatMnemonic
						( LCodeBuffer::s_MnemonicInfos[i].mnemonic, word ).Find( L"?" ) < 0 ) ;
				if ( LCodeBuffer::FormatMnemonic
						( LCodeBuffer::s_MnemonicInfos[i].operands, word ).Find( L"?" ) >= 0 )
				Assert::IsTrue
					( LCodeBuffer::FormatMnemonic
						( LCodeBuffer::s_MnemonicInfos[i].operands, word ).Find( L"?" ) < 0 ) ;
			}
		}
	};

	TEST_CLASS(LoquatyDateTimeTest)
	{
		TEST_METHOD(LeapYear)
		{
			Assert::IsFalse( LDateTime::IsLeapYear( 1 ) ) ;
			Assert::IsTrue( LDateTime::IsLeapYear( 4 ) ) ;
			Assert::IsFalse( LDateTime::IsLeapYear( 100 ) ) ;
			Assert::IsTrue( LDateTime::IsLeapYear( 400 ) ) ;
			Assert::IsTrue( LDateTime::IsLeapYear( 2000 ) ) ;
			Assert::IsTrue( LDateTime::IsLeapYear( 2004 ) ) ;
		}

		TEST_METHOD(DaysOfMonth)
		{
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 1 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 2 ) == 29 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 3 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 4 ) == 30 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 5 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 6 ) == 30 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 7 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 8 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 9 ) == 30 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 10 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 11 ) == 30 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2000, 12 ) == 31 ) ;
			Assert::IsTrue( LDateTime::DaysOfMonth( 2001, 2 ) == 28 ) ;
		}

		TEST_METHOD(DayOfWeek)
		{
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 10, 27 ) == LDateTime::Sunday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 10, 28 ) == LDateTime::Monday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 10, 29 ) == LDateTime::Tuesday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 10, 30 ) == LDateTime::Wednesday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 10, 31 ) == LDateTime::Thursday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 11, 1 ) == LDateTime::Friday ) ;
			Assert::IsTrue( LDateTime::DayOfWeek( 2024, 11, 2 ) == LDateTime::Saturday ) ;
		}

		TEST_METHOD(DayNextToBoundary)
		{
			static const int	years[] = { 1900, 2000, 2004, 2022, 2024, 2026, 2100 } ;
			for ( int i = 0; i < sizeof(years)/sizeof(years[0]); i ++ )
			{
				LDateTime	dt ;
				std::int64_t	days = LDateTime::NumberOfDays( years[i] - 1, 12, 31 ) ;
				dt.SetNumberOfDays( days ) ;
				Assert::IsTrue( (dt.year == (years[i] - 1)) && (dt.month == 12) && (dt.day == 31) ) ;

				days = LDateTime::NumberOfDays( years[i], 1, 1 ) ;
				dt.SetNumberOfDays( days ) ;
				Assert::IsTrue( (dt.year == years[i]) && (dt.month == 1) && (dt.day == 1) ) ;

				days = LDateTime::NumberOfDays( years[i], 12, 31 ) ;
				dt.SetNumberOfDays( days ) ;
				Assert::IsTrue( (dt.year == years[i]) && (dt.month == 12) && (dt.day == 31) ) ;

				days = LDateTime::NumberOfDays( years[i] + 1, 1, 1 ) ;
				dt.SetNumberOfDays( days ) ;
				Assert::IsTrue( (dt.year == (years[i] + 1)) && (dt.month == 1) && (dt.day == 1) ) ;
			}
		}

		TEST_METHOD(TimeZone)
		{
			LDateTime	dtLocal ;
			LDateTime	dtUTC ;

			do
			{
				dtLocal.GetLocalTime() ;
				dtUTC.GetGMTime() ;
			}
			while ( dtLocal.second != dtUTC.second ) ;	// ※たまたま秒の境界だったらもう一度

			std::int64_t	diffSec = dtUTC.GetSecond() - dtLocal.GetSecond() ;
			Assert::IsTrue( diffSec == LDateTime::GetTimeZone() ) ;
		}
	} ;

	TEST_CLASS(LoquatyFileTest)
	{
		void TestFile( LDirectoryPtr pDir )
		{
			pDir->DeleteFile( L"test.bin" ) ;
			Assert::IsFalse( pDir->IsExisting( L"test.bin" ) ) ;

			LFilePtr	file = pDir->OpenFile( L"test.bin", LDirectory::modeCreate ) ;
			Assert::IsTrue( file != nullptr ) ;

			std::uint32_t	data = 0x01234567 ;
			file->Write( &data, sizeof(data) ) ;
			file = nullptr ;

			LDirectory::State	state ;
			Assert::IsTrue( pDir->QueryFileState( state, L"test.bin" ) ) ;
			Assert::IsTrue( state.fileSizeInBytes == sizeof(data) ) ;

			file = pDir->OpenFile( L"test.bin", LDirectory::modeReadWrite ) ;
			Assert::IsTrue( file != nullptr ) ;
			file->Read( &data, sizeof(data) ) ;
			Assert::IsTrue( data == 0x01234567 ) ;

			data *= 2 ;
			file->Write( &data, sizeof(data) ) ;
			data *= 2 ;
			file->Write( &data, sizeof(data) ) ;
			file->Seek( 2 ) ;
			data = 0xFFFFFFFF ;
			file->Write( &data, sizeof(data) ) ;
			Assert::IsTrue( file->GetPosition() == sizeof(data)+2 ) ;
			Assert::IsTrue( file->GetLength() == sizeof(data)*3 ) ;
			file = nullptr ;

			Assert::IsTrue( pDir->QueryFileState( state, L"test.bin" ) ) ;
			Assert::IsTrue( state.fileSizeInBytes == sizeof(data)*3 ) ;

			std::vector<LString>	files ;
			pDir->ListFiles( files ) ;
			ssize_t	iTestBin = -1 ;
			for ( size_t i = 0; i < files.size(); i ++ )
			{
				if ( files.at(i) == L"test.bin" )
				{
					iTestBin = (ssize_t) i ;
					break ;
				}
			}
			Assert::IsTrue( iTestBin >= 0 ) ;

			file = pDir->OpenFile( L"test.bin", LDirectory::modeRead ) ;
			Assert::IsTrue( file != nullptr ) ;
			file->Read( &data, sizeof(data) ) ;
			Assert::IsTrue( data == 0xFFFF4567 ) ;
			file->Read( &data, sizeof(data) ) ;
			Assert::IsTrue( data == ((0x01234567*2) | 0xFFFF) ) ;

			pDir->CreateDirectory( L"test_dir1\\test_dir2" ) ;
			Assert::IsTrue( pDir->IsExisting( L"test_dir1" ) ) ;
			Assert::IsTrue( pDir->IsExisting( L"test_dir1\\test_dir2" ) ) ;

			pDir->DeleteDirectory( L"test_dir1\\test_dir2" ) ;
			pDir->DeleteDirectory( L"test_dir1" ) ;
			Assert::IsFalse( pDir->IsExisting( L"test_dir1" ) ) ;
			Assert::IsFalse( pDir->IsExisting( L"test_dir1\\test_dir2" ) ) ;
		}

#ifdef	_LOQUATY_USES_CPP_FILESYSTEM
		TEST_METHOD(CppFile)
		{
			LDirectoryPtr	pDir =
				std::make_shared<LSubDirectory>
					( std::make_shared<LCppStdFileDirectory>(),
								L"..\\..\\..\\LoquatyTest\\test" ) ;
			TestFile( pDir ) ;
		}
#endif
		TEST_METHOD(StdFile)
		{
			LDirectoryPtr	pDir =
				std::make_shared<LSubDirectory>
					( std::make_shared<LStdFileDirectory>(),
								L"..\\..\\..\\LoquatyTest\\test" ) ;
			TestFile( pDir ) ;
		}
	} ;

	TEST_CLASS(LoquatyContextTest)
	{
		#define	TestCode(x)	Assert::IsTrue( LContext::s_instruction[LCodeBuffer::code##x] == &TestContext::instruction_##x )
		#define	TestLoadStack(x,y)	Assert::IsTrue( LContext::s_pfnLoadLocal[LType::type##x] == &TestContext::load_stack_##y )
		#define	TestStoreStack(x,y)	Assert::IsTrue( LContext::s_pfnStoreLocal[LType::type##x] == &TestContext::store_stack_##y )
		#define	TestLoadPtr(x,y)	Assert::IsTrue( LContext::s_pfnLoadFromPointer[LType::type##x] == &TestContext::load_ptr_##y )
		#define	TestStorePtr(x,y)	Assert::IsTrue( LContext::s_pfnStoreIntoPointer[LType::type##x] == &TestContext::store_ptr_##y )

		class	TestContext	: public LContext
		{
		public:
			TestContext( LVirtualMachine& vm )
				: LContext( vm )
			{
				TestCode(NoOperation) ;
				TestCode(EnterFunc) ;
				TestCode(LeaveFunc) ;
				TestCode(EnterTry) ;
				TestCode(CallFinally) ;
				TestCode(RetFinally) ;
				TestCode(LeaveFinally) ;
				TestCode(LeaveTry) ;
				TestCode(Throw) ;
				TestCode(AllocStack) ;
				TestCode(FreeStack) ;
				TestCode(FreeStackLocal) ;
				TestCode(FreeLocalObj) ;
				TestCode(PushObjChain) ;
				TestCode(MakeObjChain) ;
				TestCode(Synchronize) ;
				TestCode(Return) ;
				TestCode(SetRetValue) ;
				TestCode(MoveAP) ;
				TestCode(Call) ;
				TestCode(CallVirtual) ;
				TestCode(CallIndirect) ;
				TestCode(ExImmPrefix) ;
				TestCode(ClearException) ;
				TestCode(LoadRetValue) ;
				TestCode(LoadException) ;
				TestCode(LoadArg) ;
				TestCode(LoadImm) ;
				TestCode(LoadLocal) ;
				TestCode(LoadLocalOffset) ;
				TestCode(LoadStack) ;
				TestCode(LoadFetchAddr) ;
				TestCode(LoadFetchAddrOffset) ;
				TestCode(LoadFetchLAddr) ;
				TestCode(CheckLPtrAlign) ;
				TestCode(CheckPtrAlign) ;
				TestCode(LoadLPtr) ;
				TestCode(LoadLPtrOffset) ;
				TestCode(LoadByPtr) ;
				TestCode(LoadByLAddr) ;
				TestCode(LoadByAddr) ;
				TestCode(LoadLocalPtr) ;
				TestCode(LoadObjectPtr) ;
				TestCode(LoadLObjectPtr) ;
				TestCode(LoadObject) ;
				TestCode(NewObject) ;
				TestCode(AllocBuf) ;
				TestCode(RefObject) ;
				TestCode(FreeObject) ;
				TestCode(Move) ;
				TestCode(StoreLocalImm) ;
				TestCode(StoreLocal) ;
				TestCode(ExchangeLocal) ;
				TestCode(StoreByPtr) ;
				TestCode(StoreByLPtr) ;
				TestCode(StoreByLAddr) ;
				TestCode(StoreByAddr) ;
				TestCode(BinaryOperate) ;
				TestCode(UnaryOperate) ;
				TestCode(CastObject) ;
				TestCode(Jump) ;
				TestCode(JumpConditional) ;
				TestCode(JumpNonConditional) ;
				TestCode(GetElement) ;
				TestCode(GetElementAs) ;
				TestCode(GetElementName) ;
				TestCode(SetElement) ;
				TestCode(SetElementAs) ;
				TestCode(ObjectToInt) ;
				TestCode(ObjectToFloat) ;
				TestCode(ObjectToString) ;
				TestCode(IntToObject) ;
				TestCode(FloatToObject) ;
				TestCode(IntToFloat) ;
				TestCode(UintToFloat) ;
				TestCode(FloatToInt) ;
				TestCode(FloatToUint) ;

				TestLoadStack(Boolean,bool) ;
				TestLoadStack(Int8,int8) ;
				TestLoadStack(Uint8,uint8) ;
				TestLoadStack(Int16,int16) ;
				TestLoadStack(Uint16,uint16) ;
				TestLoadStack(Int32,int32) ;
				TestLoadStack(Uint32,uint32) ;
				TestLoadStack(Int64,int64) ;
				TestLoadStack(Uint64,uint64) ;
				TestLoadStack(Float,float) ;
				TestLoadStack(Double,double) ;

				TestStoreStack(Boolean,bool) ;
				TestStoreStack(Int8,int8) ;
				TestStoreStack(Uint8,uint8) ;
				TestStoreStack(Int16,int16) ;
				TestStoreStack(Uint16,uint16) ;
				TestStoreStack(Int32,int32) ;
				TestStoreStack(Uint32,uint32) ;
				TestStoreStack(Int64,int64) ;
				TestStoreStack(Uint64,uint64) ;
				TestStoreStack(Float,float) ;
				TestStoreStack(Double,double) ;

				TestLoadPtr(Boolean,bool) ;
				TestLoadPtr(Int8,int8) ;
				TestLoadPtr(Uint8,uint8) ;
				TestLoadPtr(Int16,int16) ;
				TestLoadPtr(Uint16,uint16) ;
				TestLoadPtr(Int32,int32) ;
				TestLoadPtr(Uint32,uint32) ;
				TestLoadPtr(Int64,int64) ;
				TestLoadPtr(Uint64,uint64) ;
				TestLoadPtr(Float,float) ;
				TestLoadPtr(Double,double) ;

				TestStorePtr(Boolean,bool) ;
				TestStorePtr(Int8,int8) ;
				TestStorePtr(Uint8,uint8) ;
				TestStorePtr(Int16,int16) ;
				TestStorePtr(Uint16,uint16) ;
				TestStorePtr(Int32,int32) ;
				TestStorePtr(Uint32,uint32) ;
				TestStorePtr(Int64,int64) ;
				TestStorePtr(Uint64,uint64) ;
				TestStorePtr(Float,float) ;
				TestStorePtr(Double,double) ;
			}
		} ;
		#undef	TestCode

		TEST_METHOD(InstructionCode)
		{
			LVirtualMachine	vm ;
			vm.Initialize() ;

			TestContext	test( vm ) ;
		}
	};

	TEST_CLASS(LoquatyCompilerTest)
	{
		TEST_METHOD(LiteralInt)
		{
			LVirtualMachine	vm ;
			vm.Initialize() ;

			LCompiler			compiler( vm ) ;
			LCompiler::Current	current( compiler ) ;

			LSourceFile		src1 = L"123" ;
			LExprValuePtr	xval1 = compiler.EvaluateExpression( src1 ) ;
			Assert::IsTrue( xval1->AsInteger() == 123 ) ;

			LSourceFile		src2 = L"0o123" ;
			LExprValuePtr	xval2 = compiler.EvaluateExpression( src2 ) ;
			Assert::IsTrue( xval2->AsInteger() == 0123 ) ;

			LSourceFile		src3 = L"0x123" ;
			LExprValuePtr	xval3 = compiler.EvaluateExpression( src3 ) ;
			Assert::IsTrue( xval3->AsInteger() == 0x123 ) ;

			LSourceFile		src4 = L"0x1aF" ;
			LExprValuePtr	xval4 = compiler.EvaluateExpression( src4 ) ;
			Assert::IsTrue( xval4->AsInteger() == 0x1af ) ;

			LSourceFile		src5 = L"- 0x1aF" ;
			LExprValuePtr	xval5 = compiler.EvaluateExpression( src5 ) ;
			Assert::IsTrue( xval5->AsInteger() == -0x1af ) ;

			LSourceFile		src6 = L"0b1001100" ;
			LExprValuePtr	xval6 = compiler.EvaluateExpression( src6 ) ;
			Assert::IsTrue( xval6->AsInteger() == 0x4c ) ;

			LSourceFile		src7 = L"-0t123" ;
			LExprValuePtr	xval7 = compiler.EvaluateExpression( src7 ) ;
			Assert::IsTrue( xval7->AsInteger() == -123 ) ;

			LSourceFile		src8 = L"0" ;
			LExprValuePtr	xval8 = compiler.EvaluateExpression( src8 ) ;
			Assert::IsTrue( xval8->AsInteger() == 0 ) ;

			LSourceFile		src9 = L"-0" ;
			LExprValuePtr	xval9 = compiler.EvaluateExpression( src9 ) ;
			Assert::IsTrue( xval9->AsInteger() == 0 ) ;
			Assert::IsTrue( compiler.GetWarningCount() == 0 ) ;

			LSourceFile		src10 = L"00" ;
			LExprValuePtr	xval10 = compiler.EvaluateExpression( src10 ) ;
			Assert::IsTrue( xval10->AsInteger() == 0 ) ;
			Assert::IsTrue( compiler.GetWarningCount() == 1 ) ;
			Assert::IsTrue( compiler.GetErrorCount() == 0 ) ;

			LSourceFile		src11 = L"0o45678" ;
			LExprValuePtr	xval11 = compiler.EvaluateExpression( src11 ) ;
			Assert::IsTrue( compiler.GetErrorCount() >= 1 ) ;
		}

		TEST_METHOD(LiteralFloat)
		{
			LVirtualMachine	vm ;
			vm.Initialize() ;

			LCompiler			compiler( vm ) ;
			LCompiler::Current	current( compiler ) ;

			LSourceFile		src1 = L"123.456" ;
			LExprValuePtr	xval1 = compiler.EvaluateExpression( src1 ) ;
			Assert::IsTrue( fabs( xval1->AsDouble() - 123.456 ) < 1.0e-12 ) ;

			LSourceFile		src2 = L"0x123.45e6" ;
			LExprValuePtr	xval2 = compiler.EvaluateExpression( src2 ) ;
			Assert::IsTrue( fabs( xval2->AsDouble() - (double)0x12345e6/0x10000 ) < 1.0e-12 ) ;

			LSourceFile		src3 = L"-123.456e+3 " ;
			LExprValuePtr	xval3 = compiler.EvaluateExpression( src3 ) ;
			Assert::IsTrue( fabs( xval3->AsDouble() - -123.456e+3 ) < 1.0e-12 ) ;

			LSourceFile		src4 = L"12345.6E-10 " ;
			LExprValuePtr	xval4 = compiler.EvaluateExpression( src4 ) ;
			Assert::IsTrue( fabs( xval4->AsDouble() - 12345.6e-10 ) < 1.0e-16 ) ;

			LSourceFile		src5 = L"0.0123456e11 " ;
			LExprValuePtr	xval5 = compiler.EvaluateExpression( src5 ) ;
			Assert::IsTrue( fabs( xval5->AsDouble() - 0.0123456e+11 ) < 1.0e-12 ) ;

			Assert::IsTrue( compiler.GetWarningCount() == 0 ) ;
			Assert::IsTrue( compiler.GetErrorCount() == 0 ) ;
		}

		TEST_METHOD(LiteralChar)
		{
			LVirtualMachine	vm ;
			vm.Initialize() ;

			LCompiler			compiler( vm ) ;
			LCompiler::Current	current( compiler ) ;

			LSourceFile		src1 = L"\'a\'" ;
			LExprValuePtr	xval1 = compiler.EvaluateExpression( src1 ) ;
			Assert::IsTrue( xval1->AsInteger() == L'a' ) ;

			LSourceFile		src2 = L"\'\\\'\'" ;
			LExprValuePtr	xval2 = compiler.EvaluateExpression( src2 ) ;
			Assert::IsTrue( xval2->AsInteger() == L'\'' ) ;

			LSourceFile		src3 = L"\'\\\"\'" ;
			LExprValuePtr	xval3 = compiler.EvaluateExpression( src3 ) ;
			Assert::IsTrue( xval3->AsInteger() == L'\"' ) ;

			LSourceFile		src4 = L"\'\\123\'" ;
			LExprValuePtr	xval4 = compiler.EvaluateExpression( src4 ) ;
			Assert::IsTrue( xval4->AsInteger() == L'\123' ) ;

			LSourceFile		src5 = L"\'\\012\'" ;
			LExprValuePtr	xval5 = compiler.EvaluateExpression( src5 ) ;
			Assert::IsTrue( xval5->AsInteger() == L'\012' ) ;

			LSourceFile		src6 = L"\'\\x123\'" ;
			LExprValuePtr	xval6 = compiler.EvaluateExpression( src6 ) ;
			Assert::IsTrue( xval6->AsInteger() == L'\x123' ) ;

			Assert::IsTrue( compiler.GetWarningCount() == 0 ) ;
			Assert::IsTrue( compiler.GetErrorCount() == 0 ) ;
		}
	};

	TEST_CLASS(LoquatyVirtualMachineTest)
	{
	public:
		TEST_METHOD(ActualFunctionName)
		{
			Assert::IsTrue( LVirtualMachine::GetActualFunctionName(L"Hoge_hoge") == L"Hoge.hoge" ) ;
			Assert::IsTrue( LVirtualMachine::GetActualFunctionName(L"_Hoge__get_hoge") == L"Hoge.get_hoge" ) ;
			Assert::IsTrue( LVirtualMachine::GetActualFunctionName(L"Hoge_operator_add") == L"Hoge.operator +" ) ;
			Assert::IsTrue( LVirtualMachine::GetActualFunctionName(L"_Foo__Hoge__operator__at") == L"Foo.Hoge.operator []" ) ;
		}

		TEST_METHOD(VirtualMachine)
		{
			_CrtMemState	s1 ;
			_CrtMemCheckpoint( &s1 ) ;
			{
				LVirtualMachine	vm ;
				vm.Initialize() ;

				LVirtualMachine	vm2 ;
				vm2.InitializeRef( vm ) ;
			}
			_CrtMemState	s2 ;
			_CrtMemCheckpoint( &s2 ) ;

			_CrtMemState	sd ;
			int	d = _CrtMemDifference( &sd, &s1, &s2 ) ;

			_CrtDumpMemoryLeaks() ;
			_CrtMemDumpStatistics( &sd ) ;

			Assert::IsFalse( d ) ;

		}

		TEST_METHOD(SimpleRun)
		{
			_CrtMemState	s1 ;
			_CrtMemCheckpoint( &s1 ) ;
			{
				LVirtualMachine	vm ;
				vm.Initialize() ;

				LCompiler	compiler( vm ) ;
				LSourceFile	source = L"int main() { return 1; }" ;

				compiler.DoCompile( &source ) ;

				Assert::IsTrue( compiler.GetErrorCount() == 0 ) ;
				Assert::IsTrue( compiler.GetWarningCount() == 0 ) ;

				const LFunctionVariation *
					pFuncVar = vm.Global()->GetLocalStaticFunctionsAs( L"main" ) ;
				Assert::IsTrue( pFuncVar != nullptr ) ;
				Assert::IsTrue( pFuncVar->size() == 1 ) ;

				LPtr<LFunctionObj>	pFuncMain = pFuncVar->at(0) ;
				Assert::IsTrue( pFuncMain != nullptr ) ;

				LContext	context( vm ) ;
				auto [valRet, pExcept] =
						context.SyncCallFunction( pFuncMain.Ptr(), nullptr, 0 ) ;

				Assert::IsTrue( pExcept == nullptr ) ;
				Assert::IsTrue( valRet.AsInteger() == 1 ) ;
			}
			_CrtMemState	s2 ;
			_CrtMemCheckpoint( &s2 ) ;

			_CrtMemState	sd ;
			int	d = _CrtMemDifference( &sd, &s1, &s2 ) ;

			_CrtDumpMemoryLeaks() ;
			_CrtMemDumpStatistics( &sd ) ;

			Assert::IsFalse( d ) ;
		}

	};

}
