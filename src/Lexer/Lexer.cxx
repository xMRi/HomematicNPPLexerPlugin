//  A based on a SimpleLexer.cxx

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <string>
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


using namespace Scintilla;
using namespace Lexilla;

class LexerSimple : public LexerBase
{
public:
	LexerSimple()
	{
	}
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
	static ILexer5* LexerFactorySimple();
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
		return LexerSimple::LexerFactorySimple;
	else
		return 0;
}

EXPORT_FUNCTION Scintilla::ILexer5* CALLING_CONVENTION CreateLexer(const char* name)
{
	if (0 == strcmp(name, lexerName)) {
		return LexerSimple::LexerFactorySimple();
	}
	return nullptr;
}

EXPORT_FUNCTION const char* CALLING_CONVENTION GetNameSpace()
{
	return "";
}

inline void SCI_METHOD LexerSimple::Fold([[maybe_unused]] Sci_PositionU startPos, [[maybe_unused]] Sci_Position length, [[maybe_unused]] int initStyle, [[maybe_unused]] IDocument* pAccess)
{
}

inline void SCI_METHOD LexerSimple::Lex(Sci_PositionU startPos, Sci_Position length, [[maybe_unused]] int initStyle, [[maybe_unused]] IDocument* pAccess)
{

	try {
		Accessor astyler(pAccess, &props);
		if (length > 0) {
			astyler.StartAt(startPos);
			astyler.StartSegment(startPos);
			for (unsigned int k = 0; k<length; k++) {
				astyler.ColourTo(startPos+k, (startPos+k)%2);
			}
		}
		astyler.Flush();
	}
	catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
		pAccess->SetErrorStatus(SC_STATUS_FAILURE);
	}
}

inline ILexer5* LexerSimple::LexerFactorySimple()
{
	try {
		return new LexerSimple();
	}
	catch (...) {
		// Should not throw into caller as may be compiled with different compiler or options
		return nullptr;
	}
}
