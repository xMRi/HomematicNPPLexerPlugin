//  A based on a SimpleLexer.cxx

#include <windows.h>
#include <stdlib.h>
#include <assert.h>

#include <algorithm>
#include <map>
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
const int LXX_ERROR = 15;

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

	// Map of variables and the declaration position
	std::map<std::string,Sci_PositionU> mapVariables;
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
	// Modification -1 indicates no change, otherwise we have the rescan position (0 normally)
	Sci_Position firstModification = -1;

	// Try and load, 
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
		// not found! 
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
		TRACE("startPos=" << startPos << " length=" << length << " sumn=" << (startPos+length) << "\n");
		LexAccessor styler(pAccess);

		// Clear list on a full scan
		if (startPos==0)
		{
			mapVariables.clear();
		}
		else
		{
			// Remove all map entries behind the startPos, they will be all recaclulated.
			// With this trick, we get an error display, when a variable ist used without or before
			// its declaration!
			for (auto it =mapVariables.begin(); it!=mapVariables.end(); )
			{
				if (it->second>=startPos)
					it = mapVariables.erase(it);
				else
					++it;
			}
		}

		// 
		StyleContext sc(startPos, length, LXX_DEFAULT, styler);

		// Start states
		std::string word;
		bool commentAllowed = true;	

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
				commentAllowed = true;
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
					// But if we detected a variable with same name, we use the variable instead 
					// of the reserved name. 
					if (mapVariables.find(word)!=mapVariables.end())
					{
						// a known variable, so show it.
						sc.ChangeState(LXX_VARIABLES);
					}
					else
					{
						// We might get here for a word a second time, that is not a variable.
						// It would stay as LXX_WORD.
						// But anyhow, we just recheck all word lists for language defined words.
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
						bool parseForVariable = false;
						for (auto& e : ar)
						{
							if (e.wl->InList(word.c_str()))
							{
								// Set previous section to the state we found
								sc.ChangeState(e.id);
								parseForVariable = e.id==LXX_DECLARATION;
								break;
							}
						}

						// If we didn't found a reserved word, and we didn't found it in the variable list
						// we stop here with an error.
						if (sc.state==LXX_WORD)
							// No reserved word, not avariable, must be an error.
							sc.ChangeState(LXX_ERROR);
						else if (parseForVariable)
						{
							// It might be a variable, so we look ahead!
						
							// Skip whitespace
							int c = 0, i = 0;
							while (isspace(c=sc.GetRelativeChar(i)))
								++i;
							if (isalpha(c=sc.GetRelativeCharacter(i)) || c=='_')
							{
								word.clear();
								do
								{
									word += c;
									++i;
								} while (iswalnum(c=sc.GetRelativeCharacter(i)) || c=='_');

								// Now skip blanks and search for a semicolon
								while (isspace(c=sc.GetRelativeChar(i)))
									++i;

								// Either end of statement or assignment
								if (c==';' || c=='=')
								{
									// If we have a word that is inside a variable declaration, 
									// we set it into the list
									if (word.length()>0)
									{
										// We found a variable.
										mapVariables.try_emplace(word,sc.currentPos);
									}
								}
							}

							// We just continue at the parser position
						}
					}

					// Continue normal, either the word is now in the variable list, or it is 
					// treated a known variable, or it is not known and declared. In the last case
					// we have an error.
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
				else if (commentAllowed && sc.Match('!'))
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
					commentAllowed = false;
					word = sc.ch;				
				}
				else if (sc.Match(';'))
				{
					commentAllowed = true;
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
								commentAllowed = true;
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
