struct TMacro;
struct TCompiler;
struct TNavy;
struct TFormat;

struct TLang : TCollectionItem
{
	TLang();
	String			mask, desc, imm, immExp, blockcomment;
	bool				ignoreCase;
	int					setCP;
	TExec				defExec;
	TCollection<TMacro>			macroColl;
	TCollection<TIndent>		indentColl;
	TCollection<TBracket>		bracketColl;
	TCollection<TComment>		commentColl;
	TCollection<TExec>			execColl;
	TCollection<TCompiler>	compilerColl;
	TCollection<TDefine>		defineColl;
	TCollection<TNavy>			navyColl;
	TCollection<TFormat>		formatColl;
};

TLang::TLang ()
{
	defineColl.insert (new TDefine (L"lt", L"<"));
	defineColl.insert (new TDefine (L"gt", L">"));
	defineColl.insert (new TDefine (L"quot", L"\""));
	defineColl.insert (new TDefine (L"amp", L"&"));
	defineColl.insert (new TDefine (L"nbsp", L" "));
}

static TCollection<TLang>	langColl;
