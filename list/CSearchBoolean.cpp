#include "CSearchBoolean.h"
#include "ListMisc.h"
#include <algorithm>
#include "guard.h"
#include <shlwapi.h>
#pragma comment (lib,"shlwapi.lib")


CSearchBoolean::CSearchBoolean (bool MatchCase)
: CSearch (MatchCase)
{
    guard
    {
        Contexts = NULL;
        ClearMachine ();
        return;
    } guard_rethrow (CompilerError) unguard;
}


CSearchBoolean::~CSearchBoolean ()
{
    guard
    {
        ClearMachine ();
        return;
    } guard_rethrow (CompilerError) unguard;
}


bool CSearchBoolean::CompileSearch (const wchar_t *Pattern)
{
    guard
    {
        return (Compile (std::wstring(Pattern)));
    } guard_rethrow (CompilerError) unguard;
}


bool CSearchBoolean::MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult)
{
    guard
    {
        bool Result;

        Result = ExecuteProgram (String, StringLength);

        if (Result && ExtentsResult != NULL)
        {
            ExtentsResult->clear ();
            *ExtentsResult = GetMatchExtents (String, StringLength);
        }

        return (Result);
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::S (void)
{
    guard
    {
        if (LastSymbol != SymbolString)
            EmitError ("Expected a string.");

        EmitString (LastString);    
        NextSymbol ();
        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::A (void)
{
    guard
    {
        if (LastSymbol != SymbolLeftParen)
        {
            S ();
        }
        else
        {
            NextSymbol ();
            E ();

            if (LastSymbol != SymbolRightParen)
                EmitError ("Expected ')'.");

            NextSymbol ();
        }

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::U (void)
{
    guard
    {
        if (LastSymbol != SymbolUnaryOp)
        {
            A ();
        }
        else
        {
            StackOp Op;

            Op = LastStackOp;
            NextSymbol ();
            U ();
            EmitStackOp (Op);
        }

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::I (void)
{
    guard
    {
        U ();
        Ip ();
        return;
    } guard_rethrow (CompilerError) unguard;
}

void CSearchBoolean::Ip (void)
{
    guard
    {
        if (LastSymbol != SymbolInfixOp)
        {
            return;
        }
        else
        {
            StackOp Op;

            Op = LastStackOp;
            NextSymbol ();
            U ();
            EmitStackOp (Op);
            Ip ();
        }

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::T (void)
{
    guard
    {
        I ();
        Tp ();
        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::Tp (void)
{
    guard
    {
        if (LastSymbol != SymbolQuestionMark)
            return;
        else
        {
            NextSymbol ();
            T ();

            if (LastSymbol != SymbolColon)
                EmitError ("Expected a ':'.");

            NextSymbol ();
            T ();

            EmitStackOp (OpSelect);
        }

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::E (void)
{
    guard
    {
        T ();

        /*
        if (LastSymbol != SymbolEOL)
            E ();
            */

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::EmitStackOp (StackOp Op)
{
    guard
    {
        StackInstruction I;

        I.Op = Op;
        I.argument = -1;
        Code.push_back (I);

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::EmitString (std::wstring &String)
{
    guard
    {
        int indice;
        KMPContext *Context;
        StackInstruction I;
        
        indice = NextStringIndice;
        NextStringIndice++;

        Context = new KMPContext;
        KMPCompile (Context, String.c_str(), String.length(), MatchCase);
        KMPMachines.push_back (Context);
        // Contexts[indice] will be filled in when we are done compiling the line

        I.Op = OpPush;
        I.argument = indice;
        Code.push_back (I);

        return;
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::EmitError (std::string ErrorText)
{
    guard
    {
        CompilerError CE;

        //CE.ErrorText = ErrorText;
        CE.ErrorText = std::string("Syntax error.");
        throw (CE);
    } guard_rethrow (CompilerError) unguard;
}


std::wstring CSearchBoolean::GetString (void)
{
    guard
    {
        bool QuoteBegin = false;
        bool StringEnd = false;
        std::wstring TheString;
        
        TheString = std::wstring(L"");
        
        if (Line[Pos] == L'"')
        {
            QuoteBegin = true;
            Pos++;
        }

        while (Pos < Line.length()  &&  !StringEnd)
        {
            switch (Line[Pos])
            {
                case L'\\':
                    TheString += Line[Pos + 1];
                    Pos += 2;
                    break;

                case L')':
                case L' ':
                    if (!QuoteBegin)
                        StringEnd = true;
                    else
                    {
                        TheString += Line[Pos];
                        Pos++;
                    }

                    break;

                case L'"':
                    if (QuoteBegin)
                    {
                        StringEnd = true;
                        Pos++;
                        break;
                    }
                    
                    // else fall through

                default:
                    TheString += Line[Pos];
                    Pos++;
                    break;
            }
        }

        return (TheString);
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::NextSymbol (void)
{
    guard
    {
        wchar_t c;
        wchar_t d;
        std::wstring Str;
        std::wstring StrCopy;

        while (iswspace(Line[Pos]))
            Pos++;

        c = Line[Pos];
        switch (towlower(c))
        {
            default:
                LastString = GetString();
                LastSymbol = SymbolString;
                break;

            case L'a':
            case L'o':
            case L'x':
            case L'e':
            case L'n':
                Str = GetString ();
                StrCopy = Str;
                std::for_each (Str.begin(), Str.end(), towlower);

                if (Str == L"and")
                {
                    LastSymbol = SymbolInfixOp;
                    LastStackOp = OpAnd;
                }
                else
                if (Str == L"or")
                {
                    LastSymbol = SymbolInfixOp;
                    LastStackOp = OpOr;
                }
                else
                if (Str == L"xor")
                {
                    LastSymbol = SymbolInfixOp;
                    LastStackOp = OpXor;
                }
                else
                if (Str == L"eqv")
                {
                    LastSymbol = SymbolInfixOp;
                    LastStackOp = OpEqv;
                }
                else
                if (Str == L"not")
                {
                    LastSymbol = SymbolUnaryOp;
                    LastStackOp = OpNot;
                }
                else
                {
                    LastString = StrCopy;      // retain case, Str = strlwr(StrCopy) essentially
                    LastSymbol = SymbolString;
                }

                break;

            case L'\0':
                LastSymbol = SymbolEOL;
                break;

            case L'~':
                LastSymbol = SymbolUnaryOp;
                LastStackOp = OpNot;
                Pos++;
                break;

            case L'!':
                d = Line[Pos + 1];

                if (d == L'=')
                {
                    LastSymbol = SymbolInfixOp;
                    LastStackOp = OpNeq;
                    Pos += 2;
                }
                else
                {
                    LastSymbol = SymbolUnaryOp;
                    LastStackOp = OpNot;
                    Pos++;
                }

                break;

            case L'&':
                Pos++;
                d = Line[Pos];

                if (d == L'&')
                    Pos++;

                LastSymbol = SymbolInfixOp;
                LastStackOp = OpAnd;
                break;

            case L'|':
                Pos++;
                d = Line[Pos];

                if (d == L'|')
                    Pos++;

                LastSymbol = SymbolInfixOp;
                LastStackOp = OpOr;
                break;

            case L'=':
                Pos++;
                d = Line[Pos];

                if (d == L'=')
                    Pos++;

                LastSymbol = SymbolInfixOp;
                LastStackOp = OpEqv;
                break;

            case L'^':
                LastSymbol = SymbolInfixOp;
                LastStackOp = OpXor;
                Pos++;
                break;

            case L'?':
                LastSymbol = SymbolQuestionMark;
                Pos++;
                break;

            case L':':
                LastSymbol = SymbolColon;
                Pos++;
                break;

            case L'(':
                LastSymbol = SymbolLeftParen;
                Pos++;
                break;

            case L')':
                LastSymbol = SymbolRightParen;
                Pos++;
                break;
        }

        return;
    } guard_rethrow (CompilerError) unguard;
}


bool CSearchBoolean::Compile (std::wstring Text)
{
    guard
    {
        int i;

        Line = Text;
        Pos = 0;
        ClearMachine ();

        try
        {
            NextSymbol ();

            while (LastSymbol != SymbolEOL)
            {
                E ();
            }
        }

        catch (CompilerError CE)
        {
            SetLastError (CE.ErrorText);
            ClearMachine ();
            return (false);
        }

        Contexts = new KMPContext* [KMPMachines.size()];
        for (i = 0; i < KMPMachines.size(); i++)
            Contexts[i] = KMPMachines[i];

        // Debug
    #ifndef NDEBUG
        FILE *out = fopen ("c:/temp/code.txt", "wt");

        if (out != NULL)
        {
            fputs ("Boolean expression:\n", out);
            fprintf (out, "%s\n\n", Text.c_str());

            fputs ("String table:\n", out);
            for (i = 0; i < KMPMachines.size(); i++)
            {
                fprintf (out, "[%d]: \"%s\"\n", i, KMPMachines[i]->Pattern.c_str());
            }

            fputs ("\n", out);

            fputs ("Stack-machine instructions:\n", out);
            fputs ("(push true)\n", out);
            for (i = 0; i < Code.size(); i++)
            {
                switch (Code[i].Op)
                {
                    case OpNot:
                        fputs ("not\n", out);
                        break;   

                    case OpPush:   
                        fprintf (out, "push [%d]\n", Code[i].argument);
                        break;   

                    case OpPop:
                        fputs ("pop\n", out);
                        break;   

                    case OpAnd:
                        fputs ("and\n", out);
                        break;   

                    case OpOr:
                        fputs ("or\n", out);
                        break;   

                    case OpXor:
                        fputs ("xor\n", out);
                        break;   

                    case OpEqv:
                        fputs ("eqv\n", out);
                        break;   

                    case OpNeq:
                        fputs ("neq\n", out);
                        break;   

                    case OpSelect:
                        fputs ("select\n", out);
                        break;   
                }
            }

            fclose (out);
        }
    #endif

        return (true);
    } guard_rethrow (CompilerError) unguard;
}


void CSearchBoolean::ClearMachine (void)
{
    guard
    {
        int i;

        if (Contexts != NULL)
            delete Contexts;

        IP = 0;

        Code.clear ();
        Stack.clear ();
        NextStringIndice = 0;
        LastSymbol = SymbolInvalid;

        for (i = 0; i < KMPMachines.size(); i++)
        {
            KMPFree (KMPMachines[i]);
            delete KMPMachines[i];
            KMPMachines[i] = NULL;
        }

        KMPMachines.clear ();

        return;
    } guard_rethrow (CompilerError) unguard;
}


bool CSearchBoolean::ExecuteProgram (const wchar_t *String, const int Length)
{
    guard
    {
        int Value;
        int LHS;
        int RHS;

        Stack.clear ();
        Stack.push_back (1);
        IP = 0;

        KMPEvaluateMany (Contexts, KMPMachines.size(), String, Length);

        for (IP = 0; IP < Code.size(); IP++)
        {
            switch (Code[IP].Op)
            {
                case OpNot:
                    Value = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (!Value);
                    break;

                case OpPush:
                    Stack.push_back ((KMPMachines[Code[IP].argument]->Result == -1) ? 0 : 1);
                    break;

                case OpPop:
                    Stack.pop_back ();
                    break;

                case OpAnd:
                    LHS = Stack.back ();
                    Stack.pop_back ();
                    RHS = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (LHS && RHS);
                    break;

                case OpOr:
                    LHS = Stack.back ();
                    Stack.pop_back ();
                    RHS = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (LHS || RHS);
                    break;

                case OpXor:
                    LHS = Stack.back ();
                    Stack.pop_back ();
                    RHS = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (LHS ^ RHS);
                    break;

                case OpEqv:
                    LHS = Stack.back ();
                    Stack.pop_back ();
                    RHS = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (LHS == RHS);
                    break;

                case OpNeq:
                    LHS = Stack.back ();
                    Stack.pop_back ();
                    RHS = Stack.back ();
                    Stack.pop_back ();
                    Stack.push_back (LHS != RHS);
                    break;

                case OpSelect:
                    int A, B, C;

                    C = Stack.back ();
                    Stack.pop_back ();
                    B = Stack.back ();
                    Stack.pop_back ();
                    A = Stack.back ();
                    Stack.pop_back ();

                    Stack.push_back ((A && B) || (!A && C));
                    break;

    #ifdef DEBUG
                default:
                    LHS = 5;
                    break;
    #endif
            }
        }

        while (Stack.size() > 1)
        {
            LHS = Stack.back ();
            Stack.pop_back ();
            RHS = Stack.back ();
            Stack.pop_back ();
            Stack.push_back (LHS && RHS);
        }

        return (bool(Stack.back()));
    } guard_rethrow (CompilerError) unguard;
}


CSearch::MatchExtentGroup CSearchBoolean::GetMatchExtents (const wchar_t *String, const int StringLength)
{
    guard
    {
        MatchExtentGroup extents;
        int matchindice;
        const wchar_t *p;
        const wchar_t *s;
        int i;

        s = String;

        while (s < String + StringLength)
        {
            for (i = 0; i < KMPMachines.size(); i++)
            {
                if (!MatchCase)
                    p = StrStrIW (s, KMPMachines[i]->Pattern.c_str());
                else
                    p = StrStrW (s, KMPMachines[i]->Pattern.c_str());

                if (p != NULL)
                {
                    extents.push_back (make_pair (int(p - String), int(KMPMachines[i]->Pattern.length())));
                    s = p + 1;
                }
            }

            s++;
        }

        return (extents);

#if 0
        MatchExtentGroup ret;
        MatchExtentGroup ind;
        int matchindice;
        int i;

        if (StringLength != 0)
        {
            ind = GetMatchExtents (String + 1, StringLength - 1);
            AdjustMatchGroup (ind);
        }

        wstring wString (String);

        for (i = 0; i < KMPMachines.size(); i++)
        {
            wstring::const_iterator ptr;

            if (!MatchCase)
                ptr = searchi (wString, KMPMachines[i]->Pattern);
            else
                ptr = search (wString, KMPMachines[i]->Pattern);

            if (ptr != wString.end())
            {
                matchindice = ptr - wString.end();
                ret.push_back (std::make_pair (matchindice, int(KMPMachines[i]->Pattern.length())));
            }
        }

        MergeMatchGroups (ret, ind);
        return (ret);
#endif
    } unguard;
}

