//  A based on a SimpleLexer.cxx

#include <windows.h>
#include <stdlib.h>
#include <assert.h>

#include <set>
#include <string>
#include <sstream>
#include <string_view>

#include "ILexer.h"
#include "Scintilla.h"
#include "SciLexer.h"
// Lexilla.h should not be included here as it declares statically linked functions without the __declspec( dllexport )

#include "WordList.h"
#include "PropSetSimple.h"
#include "LexAccessor.h"
#include "Accessor.h"
#include "StyleContext.h"
#include "CharacterSet.h"
#include "LexerModule.h"
#include "LexerBase.h"

#if defined(_WIN32)
#define EXPORT_FUNCTION extern "C" __declspec(dllexport) 
#define CALLING_CONVENTION __stdcall
#else
#define EXPORT_FUNCTION __attribute__((visibility("default")))
#define CALLING_CONVENTION
#endif

#ifdef _DEBUG
#define TRACE( s )            \
{                             \
   std::wostringstream os_;    \
   os_ << s;                   \
   OutputDebugStringW( os_.str().c_str() );  \
}
#else
#define TRACE( s )            
#endif


using namespace Scintilla;
using namespace Lexilla;

// Lexer XML defs
const int LANG_INDEX_INSTR = 0;
const int LANG_INDEX_INSTR2 = 1;
const int LANG_INDEX_TYPE = 2;
const int LANG_INDEX_TYPE2 = 3;
const int LANG_INDEX_TYPE3 = 4;
const int LANG_INDEX_TYPE4 = 5;
const int LANG_INDEX_TYPE5 = 6;
const int LANG_INDEX_TYPE6 = 7;
const int LANG_INDEX_TYPE7 = 8;
const int LANG_INDEX_SUBSTYLE1 = 9;
const int LANG_INDEX_SUBSTYLE2 = 10;
const int LANG_INDEX_SUBSTYLE3 = 11;
const int LANG_INDEX_SUBSTYLE4 = 12;
const int LANG_INDEX_SUBSTYLE5 = 13;
const int LANG_INDEX_SUBSTYLE6 = 14;
const int LANG_INDEX_SUBSTYLE7 = 15;
const int LANG_INDEX_SUBSTYLE8 = 16;

// These StyleID values from Lexer.xml
const int LXX_DEFAULT = 0;
const int LXX_WORD = 1;
const int LXX_METHODS = 2;
const int LXX_NAMESPACE = 3;
const int LXX_REGAOBJ = 4;
const int LXX_DECLARATION = 5;
const int LXX_COMMENT = 6;
const int LXX_STRINGCONSTANT = 7;
const int LXX_STATEMENTS = 8;
const int LXX_OPERATOR = 9;
const int LXX_CONSTANTS = 10;
const int LXX_CONSTANTS_OTHER = 11;
const int LXX_REGAMETHODS = 12;
const int LXX_VARIABLES = 13;
const int LXX_OPERATOR2 = 14;

constexpr int inactiveFlag = 0x40;

class LexerHomematic : public LexerBase
{
public:
	LexerHomematic()
	{
	}
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
	Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
	static ILexer5* LexerFactoryHomematic();

	constexpr static int MaskActive(int style) noexcept {
		return style & ~inactiveFlag;
	}

private: 
private:
	WordList wlMethods,
			 wlStatements,
			 wlRegaObj,
			 wlNamespace,
			 wlDeclaration,
			 wlRegaMethods,
			 wlConstants,
			 wlConstantsOther;

	std::set<std::string> setVariables;
};


static const char *lexerName = "Homematic";


EXPORT_FUNCTION int CALLING_CONVENTION GetLexerCount()
{
	return 1;
}

EXPORT_FUNCTION void CALLING_CONVENTION GetLexerName(unsigned int index, char* name, int buflength)
{
	*name = 0;
	if ((index == 0) && (buflength > static_cast<int>(strlen(lexerName)))) {
		strcpy(name, lexerName);
	}
}

EXPORT_FUNCTION LexerFactoryFunction CALLING_CONVENTION GetLexerFactory(unsigned int index)
{
	if (index == 0)
		return LexerHomematic::LexerFactoryHomematic;
	else
		return 0;
}

EXPORT_FUNCTION Scintilla::ILexer5* CALLING_CONVENTION CreateLexer(const char* name)
{
	if (0 == strcmp(name, lexerName)) {
		return LexerHomematic::LexerFactoryHomematic();
	}
	return nullptr;
}

EXPORT_FUNCTION const char* CALLING_CONVENTION GetNameSpace()
{
	return "";
}

inline void SCI_METHOD LexerHomematic::Fold([[maybe_unused]] Sci_PositionU startPos, [[maybe_unused]] Sci_Position length, [[maybe_unused]] int initStyle, [[maybe_unused]] IDocument* pAccess)
{
	// Code copied from file in NPP project, removed all options
	// notepad-plus-plus\lexilla\lexers\LexCPP.cxx
	LexAccessor styler(pAccess);

	const Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	bool inLineComment = false;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = FoldLevelStart(styler.LevelAt(lineCurrent-1));
	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent+1);
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = MaskActive(styler.StyleAt(startPos));
	int style = MaskActive(initStyle);
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		// const int stylePrev = style;
		style = styleNext;
		styleNext = MaskActive(styler.StyleAt(i + 1));
		const bool atEOL = i == (lineStartNext-1);
		if (style==LXX_OPERATOR2) {
			if (ch == '{') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}') {
				levelNext--;
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			// Allow to fold on else.
			levelUse = levelMinCurrent;
			
			const int lev = FoldLevelForCurrentNext(levelUse, levelNext) |
				FoldLevelFlags(levelUse, levelNext, visibleChars == 0);
			styler.SetLevelIfDifferent(lineCurrent, lev);
			lineCurrent++;
			lineStartNext = styler.LineStart(lineCurrent+1);
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, FoldLevelForCurrent(levelCurrent) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
			inLineComment = false;
		}
	}
}

Sci_Position SCI_METHOD LexerHomematic::WordListSet(int n, const char* wl) 
{
	// Try and load
	Sci_Position firstModification = -1;
	struct {
		const int id;
		WordList* wl;
	} const ar[] = {
		LANG_INDEX_INSTR,	&wlMethods, 
		LANG_INDEX_INSTR2,	&wlStatements, 
		LANG_INDEX_TYPE,	&wlNamespace,
		LANG_INDEX_TYPE2,	&wlRegaObj,
		LANG_INDEX_TYPE3,	&wlDeclaration, 
		LANG_INDEX_TYPE4,	&wlRegaMethods, 
		LANG_INDEX_TYPE5,	&wlConstants, 
		LANG_INDEX_TYPE6,	&wlConstantsOther, 
	};
	auto it = std::find_if(std::begin(ar), std::end(ar), [n](const auto& item) { return item.id==n; });
	if (it==std::end(ar)) 
		// not found it! 
		return firstModification;

	WordList* wordListN = it->wl;

	// Check if the wordlist changed
	WordList wlNew;
	wlNew.Set(wl);
	if (*wordListN!=wlNew) 
	{
		wordListN->Set(wl);
		firstModification = 0;
	}
	return firstModification;
}

inline void SCI_METHOD LexerHomematic::Lex(Sci_PositionU startPos, Sci_Position length, [[maybe_unused]] int initStyle, [[maybe_unused]] IDocument* pAccess)
{
	try 
	{
		LexAccessor styler(pAccess);
		StyleContext sc(startPos, length, LXX_DEFAULT, styler);

		std::string word;
		bool commenAllowed = true;	

		// sc.More() checked at loop bottom
		while(sc.More())
		{
			if (sc.state==LXX_STRINGCONSTANT)
			{
				if (sc.Match("\\\""))
					// Skip backslash and the next \"
					sc.ForwardBytes(2);
				else if (sc.Match('\"'))
					// Found the end.
					sc.ForwardSetState(LXX_DEFAULT);
				else
					// Skip character
					sc.Forward();
				continue;
			}
			else if (sc.state==LXX_COMMENT)
			{
				// Skip comments until end of line
				if (sc.atLineEnd)
					// Found the end.
					sc.ForwardSetState(LXX_DEFAULT);
				else
					// Skip character
					sc.Forward();
				continue;
			}

			// Remember a line change here
			if (sc.atLineStart)
			{
				// On a line end, we stop with comments
				commenAllowed = true;
				sc.SetState(LXX_DEFAULT);
			}

			if (sc.state==LXX_WORD)
			{
				if (iswalnum(sc.ch) || sc.ch=='_')
				{
					word += sc.ch;
				}
				else
				{
					// Check all word lists
					struct {
						const int id;
						WordList* wl;
					} const ar[] = {
						LXX_METHODS,		 &wlMethods, 
						LXX_STATEMENTS,		 &wlStatements,
						LXX_DECLARATION,	 &wlDeclaration, 
						LXX_NAMESPACE,	 	 &wlNamespace,
						LXX_REGAOBJ,	 	 &wlRegaObj,
						LXX_REGAMETHODS,	 &wlRegaMethods,
						LXX_CONSTANTS,		 &wlConstants,
						LXX_CONSTANTS_OTHER, &wlConstantsOther,
					};
					bool parseForVariable=false;
					bool found = false;
					for (auto& e : ar)
					{
						if (e.wl->InList(word.c_str()))
						{
							// Set previous section to the state we found
							sc.ChangeState(e.id);
							found = true;
							parseForVariable = e.id==LXX_DECLARATION;
							break;
						}
					}

					// It might be a variable
					if (parseForVariable)
					{
						auto curpos = sc.currentPos;

						// Skip whitespace
						while (sc.More() &&  isspace(sc.ch))
							sc.Forward();
						if (sc.More() && (isalpha(sc.ch) || sc.ch=='_'))
						{
							word.clear();
							do
							{
								word += sc.ch;
								sc.Forward();
							} while (sc.More() && (iswalnum(sc.ch) || sc.ch=='_'));

							// If we have a word, we set it into the list
							if (word.length()>0)
								setVariables.emplace(word);
						}

						// Get back to previous position:
						sc.currentPos = curpos;
						sc.chPrev = sc.GetRelativeCharacter(-1);
						sc.ch = sc.GetRelativeCharacter(0);
						sc.chNext = sc.GetRelativeCharacter(1);
					}
					else if (!found)
					{
						// Check if we know this variable name
						if (setVariables.find(word)!=setVariables.end())
							// We found the variable, so we can color it.
							sc.ChangeState(LXX_VARIABLES);
					}
					
					// Continue normal.
					sc.SetState(LXX_DEFAULT);
					continue;
				}
			} 
			else if (sc.state==LXX_DEFAULT) 
			{
				if (isspace(sc.ch))
				{
					// Skip blanks fast
					;
				}
				else if (sc.Match('\"'))
				{
					sc.SetState(LXX_STRINGCONSTANT);
				}
				else if (commenAllowed && sc.Match('!'))
				{
					sc.SetState(LXX_COMMENT);
				}
				else if (sc.ch>='0' && sc.ch<='9')
				{
					sc.SetState(LXX_CONSTANTS);
					// Skip normal digits
					do {
						sc.Forward();
					}
					while (sc.ch>='0' && sc.ch<='9');
					if (sc.ch=='.')
					{
						// Skip for reals
						do {
							sc.Forward();
						}
						while (sc.ch>='0' && sc.ch<='9');
					}
					sc.SetState(LXX_DEFAULT);
					continue;
				}
				else if (sc.ch=='@')
				{
					sc.SetState(LXX_CONSTANTS);
					// Skip Digits colons and hyphens
					do {
						sc.Forward();
					}
					while ((sc.ch>='0' && sc.ch<='9') || sc.ch=='-' || sc.ch==':' || sc.ch==' ');
					if (sc.ch=='@')
					{
						sc.Forward();
					}
					sc.SetState(LXX_DEFAULT);
					continue;
				}
				else if (isalpha(sc.ch) || sc.ch=='_')
				{
					// Start collecting the next word.
					sc.SetState(LXX_WORD);
					commenAllowed = false;
					word = sc.ch;				
				}
				else if (sc.Match(';'))
				{
					commenAllowed = true;
				}
				else
				{
					{
						// Check for operator
						static char const* ops[] = {
							"==", "!=", "<>",
							"=",
							"<=", "<", ">=", ">",
							"+", "-", "*", "/", "%",
							"&&", "||",
							"!", "&", "|", "#", ".",
						};
						auto it = std::find_if(std::begin(ops), std::end(ops), [&sc](const auto& item) { return sc.Match(item); });
						if (it!=std::end(ops))
						{
							sc.SetState(LXX_OPERATOR);
							sc.ForwardBytes(strlen(*it)-1);
							sc.ForwardSetState(LXX_DEFAULT);
							continue;
						}
					}
					{
						static char const* ops2[] = {
							"{", "}", "(", ")",
						};
						auto it = std::find_if(std::begin(ops2), std::end(ops2), [&sc](const auto& item) { return sc.Match(item); });
						if (it!=std::end(ops2))
						{
							if (**it=='{' || **it=='}')
								commenAllowed = true;
							sc.SetState(LXX_OPERATOR2);
							sc.ForwardBytes(strlen(*it)-1);
							sc.ForwardSetState(LXX_DEFAULT);
							continue;
						}
					}
				}
			}

			// Eat one byte
			sc.Forward();
		}
		
		sc.Complete();
	}
	catch (...) 
	{
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

inline ILexer5* LexerHomematic::LexerFactoryHomematic()
{
	try {
		return new LexerHomematic();
	}
	catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
		return nullptr;
	}
}
