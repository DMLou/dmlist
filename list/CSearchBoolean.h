/*****************************************************************************

  CSearchBoolean

  Boolean expression pattern matching. Based off of a simple stack-maching
  style language parser/compiler

  The following grammar is implemented:
    S  -> "S" | string
    A  -> S | (E)
    U  -> uOp U | A
    I  -> U I' 
    I' -> iOp U I' | (eps)
    T  -> I T'
    T' -> ? T : T | (eps)
    E  -> T E | (eps)

    iOp -> and , or , xor , eqv , & , && , | , || , ^ , = , == , !=
    uOp -> not , ~  , !

    In the second production for E (expression) it is noted that you can
    put many strings together that are not glued together by operations.

    When this is done, "S E" is equivalent to "S and E", so evaluating:
        bob charly may sue
    is equivalent to evaluating:
        bob and charly and may and sue
    Likewise:
        bob and charly may ? sue jorge : taco beef
    Is equivalent to:
        bob and charly and may ? sue and jorge : taco and beef
    For clarity we can write this as:
        bob and charly and (may ? (sue and jorge) : (taco and beef))

    The way this is handled is that in the execution stage, if at the end
    of executing all the instructions the stack has more than 1 element
    in it, then we perform an "and" operation until we have only 1 element.

    For strings, you may atomize strings with spaces by enclosing them in
    quotation marks. Thus 'bob charly' is not the same as '"bob charly"'.
    The first would recognize something like "bob talked to charly" while
    the second would not.

    Each term of an expression to be evaluated must be separated by a space.
    For example:
        bob&sue&charly
    Will not match the same strings as:
        bob & sue & charly

******************************************************************************/


#ifndef _CSEARCHBOOLEAN_H
#define _CSEARCHBOOLEAN_H


#include "CSearch.h"
#include "Search.h"
#include <vector>


class CSearchBoolean : public CSearch
{
public:
    CSearchBoolean (bool MatchCase);
    ~CSearchBoolean ();

    bool CompileSearch (const wchar_t *Pattern);
    bool MatchPattern (const wchar_t *String, const int StringLength, MatchExtentGroup *ExtentsResult);
    MatchExtentGroup GetMatchExtents (const wchar_t *String, const int StringLength);

protected:
    typedef enum
    {
        OpNot,    // Push(!TopAndPop())
        OpPush,   // Push((KMPMachines[argument]->Result == -1) ? false : true)
        OpPop,    // Pop()
        OpAnd,    // Push(TopAndPop() && TopAndPop())
        OpOr,     // Push(TopAndPop() || TopAndPop())
        OpXor,    // Push(TopAndPop() ^  TopAndPop())
        OpEqv,    // Push(TopAndPop() == TopAndPop())
        OpNeq,    // Push(TopAndPop() != TopAndPop())
        OpSelect, // x = ((TopMinus2() && TopMinus1()) || (TopMinus2() && Top)); Pop(); Pop(); Pop(); Push(x);
    } StackOp;

    typedef struct
    {
        StackOp Op;
        int argument;
    } StackInstruction;

    typedef std::vector<StackInstruction> StackProgram;
    StackProgram Code;
    int IP; // instruction pointer, IP = 0; while (IP < Code.size()) { Execute(Code[IP]); IP++; } Result = Top();
    
    std::vector<int> Stack;     // We use a vector<> as a stack (push_back, pop_back)

    std::vector<KMPContext *> KMPMachines;
    KMPContext **Contexts; // for when we call KMPEvaluateMany
                           // When we compile, Contexts[i] = KMPMachines[i]
    int NextStringIndice;

    // Compiler
    typedef enum
    {
        SymbolQuestionMark, // ?
        SymbolColon,        // :
        SymbolString,       // stored in LastString
        SymbolInfixOp,      // iOp, stored in LastStackOp
        SymbolUnaryOp,      // uOp, stored in LastStackOp
        SymbolLeftParen,    // (
        SymbolRightParen,   // )
        SymbolEOL,
        SymbolInvalid
    } SymbolTypes;

    // Throw an error as an exception
    typedef struct
    {
        std::string ErrorText;
    } CompilerError;

    std::wstring Line;
    int Pos;      // Line[Pos] ... Pos++;
    std::wstring LastString;
    StackOp LastStackOp;
    SymbolTypes LastSymbol;

    void S  (void);
    void A  (void);
    void U  (void);
    void I  (void);
    void Ip (void);
    void T  (void);
    void Tp (void);
    void E  (void);

    void EmitStackOp (StackOp Op);
    void EmitString (std::wstring &String);
    void EmitError (std::string ErrorText);
    std::wstring GetString (void);
    void NextSymbol (void);
    bool Compile (std::wstring Text);

    void ClearMachine (void); // clears out all the variables and structures used after compiling. 
                              // used to prepare for compiling another pattern

    bool ExecuteProgram (const wchar_t *String, const int Length);
};


#endif // _CSEARCHBOOLEAN_H

