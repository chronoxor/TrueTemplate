struct TIndent : TCollectionItem
{
	String		mask, relative;
	wchar_t		immChar;
	intptr_t	indent[2], start;
	size_t		bracketLink;
	int				BracketsMode;
};

struct TBracket : TCollectionItem
{
	String	open, close;
};

struct TComment : TCollectionItem
{
	String	mask;
};

struct TDefine : TCollectionItem
{
	String	name, value;
	TDefine(const wchar_t *aName, const wchar_t *aValue)
		: name(aName), value(aValue)
	{
	}
};

enum TJumpType
{
	jtNone,
	jtFirst,
	jtMenu,
	jtSmart
};
enum TSaveType
{
	saNone,
	saAll,
	saCurrent
};
enum TCDType
{
	cdNone,
	cdFile,
	cdBase
};
