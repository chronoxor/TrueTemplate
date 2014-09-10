class TLang : public TCollectionItem
{
public:
	TLang();
	String			mask, desc, imm, immExp, blockcomment;
	bool				ignoreCase;
	int					setCP;
	TExec				defExec;
	TCollection macroColl, indentColl, bracketColl, commentColl, execColl, compilerColl, defineColl, navyColl,
		formatColl;
};

TLang::TLang ()
{
	defineColl.insert (new TDefine (L"lt", L"<"));
	defineColl.insert (new TDefine (L"gt", L">"));
	defineColl.insert (new TDefine (L"quot", L"\""));
	defineColl.insert (new TDefine (L"amp", L"&"));
	defineColl.insert (new TDefine (L"nbsp", L" "));
}

class TLangCollection : public TCollection
{
	public:
		TLangCollection () :
			TCollection(5, 5)
		{
		}
};

static TLangCollection	langColl;
