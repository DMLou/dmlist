#include "List.h"
#include "Search.h"

// Note: std::search was found to be much faster than strstr

// Brute force substring search
wchar_t *StringSearch (const wchar_t *String, int StrLen, const wchar_t *SubString, int SubLen)
{
    guard
    {
        wchar_t *Result;

        Result = const_cast<wchar_t *>(search (String, String + StrLen, SubString, SubString + SubLen));

        if (Result == String + StrLen)
            return (NULL);

        return (Result);
    } unguard;
}


// Function that takes two wide characters and compares them without regard to case
// depends on initialization of external ToLowerTable
bool ignorecase (const wchar_t A, const wchar_t B)
{
    guard
    {
        return (ToLowerTable[A] == ToLowerTable[B]);
    } unguard;
}


// Brute force substring search (case insensitive)
wchar_t *StringSearchI (const wchar_t *String, int StrLen, const wchar_t *SubString, int SubLen)
{
    guard
    {
        wchar_t *Result;

        Result = const_cast<wchar_t *> (search (String, String + StrLen, SubString, SubString + SubLen, ignorecase));

        if (Result == String + StrLen)
            return (NULL);

        return (Result);
    } unguard;
}


// Case-sensitive
// Designed to quickly go through and *eliminate* a string
// Substring must be >= 2 chars in length to benefit
/*
char *StringSearchMMX (const char *String, int StrLen, const char *SubString, int SubLen)
{
    uint64 BitMask1;
    uint32 BitMask3;
    uint64 MatchResult = 0;

    if (StrLen < SubLen)
        return (NULL);

    if (SubLen == 0)
        return (NULL);

    if (SubLen == 1)
        return (StringSearch (String, StrLen, SubString, SubLen));

    BitMask1 = (SubString[1] << 8) | SubString[0];
    BitMask3 = (uint32)BitMask1;
    BitMask1 = BitMask1 | (BitMask1 << 16) | (BitMask1 << 32) | (BitMask1 << 48);

    // mm0 = temp for reading
    // mm1 = temp for copying mm0,mm2-4 into
    // mm2 = temp for reading
    // mm3 = temp for reading
    // mm4 = temp for reading
    // mm5 = result mask. 0 = no match (good), 1 = possible match (bad)
    // mm6 = bit mask #1
    // mm7 = bit mask #2
    __asm
    {
        pxor mm6, mm6
        movq mm7, BitMask1
        mov  esi, String
        mov  eax, StrLen
        mov  ecx, eax
        mov  ebx, BitMask3
        shr  ecx, 5
        jz SSMMXLoop2Setup

    SSMMXLoop1:
        prefetchnta [esi + 128]

        movq mm0, [esi]
        movq mm1, [esi + 6]
        movq mm2, [esi + 12]
        movq mm3, [esi + 18]

        movq mm4, mm0
        psrlq mm4, 8
        pcmpeqw mm0, mm7
        pcmpeqw mm4, mm7
        por mm6, mm0
        por mm6, mm4

        movq mm4, mm1
        psllq mm4, 8
        pcmpeqw mm1, mm7
        pcmpeqw mm4, mm7
        por mm6, mm1
        por mm6, mm4

        movq mm4, mm2
        psllq mm4, 8
        pcmpeqw mm2, mm7
        pcmpeqw mm4, mm7
        por mm6, mm2
        por mm6, mm4

        movq mm4, mm3
        psllq mm4, 8
        pcmpeqw mm3, mm7
        pcmpeqw mm4, mm7
        por mm6, mm3
        por mm6, mm4

        movq mm0, [esi + 24]
        movq mm1, [esi + 28]

        movq mm4, mm0
        psllq mm4, 8
        pcmpeqw mm0, mm7
        pcmpeqw mm4, mm7
        por mm6, mm0
        por mm6, mm4

        movq mm4, mm1
        psllq mm4, 8
        pcmpeqw mm1, mm7
        pcmpeqw mm4, mm7
        por mm6, mm1
        por mm6, mm4

        add esi, 32
        sub ecx, 1
        jnz SSMMXLoop1        

    SSMMXLoop2Setup:
        mov ecx, eax   // ecx = string length
        xor eax, eax   // eax = 0
        and ecx, 31 
        jz SSMMXDone

    SSMMXLoop2:
        xor edx, edx    // edx = 0
        mov al, [esi]   // load into al the next string byte
        cmp al, bl      // al == bl ?
        cmove edx, eax  // (al == bl) ? (edx = eax) : (edx = 0)
                        // assumes that al != 0, and since this is a *string* (only \0 should be at the end), this is valid to do for our purposes
        movd mm0, edx
        por mm6, mm0    // mm6 |= edx

        add esi, 1
        sub ecx, 1
        jnz SSMMXLoop2

    SSMMXDone:
        movq MatchResult, mm6
        emms
    }

    if (MatchResult != 0)
        return (StringSearch (String, StrLen, SubString, SubLen));

    return (NULL);
}
*/


extern wchar_t ToLowerTable[65536];
extern wchar_t IdentityTable[65536];


// KMP search algorithm implementation
bool KMPCompile (KMPContext *Result, const wchar_t *Pattern, const int Length, bool MatchCase)
{
    guard
    {
        int i;
        int j;
        wchar_t *Table;

        Result->JumpVector = new int[Length + 1];

        if (Result->JumpVector == NULL)
            return (false);

        Result->MatchCase = MatchCase;

        if (MatchCase)
            Table = IdentityTable;
        else
            Table = ToLowerTable;

        i = 0;
        j = -1;
        Result->JumpVector[0] = -1;

        while (i < Length)
        {
            while (j >= 0  &&  Table[Pattern[i]] != Table[Pattern[j]])
                j = Result->JumpVector[j];

            i++;
            j++;

            if (Table[Pattern[i]] == Table[Pattern[j]])
                Result->JumpVector[i] = Result->JumpVector[j];
            else
                Result->JumpVector[i] = j;
        }

        Result->Pattern = wstring (Pattern);
        Result->Result = -1;
        Result->j = 0;
        return (true);
    } unguard;
}


void KMPFree (KMPContext *FreeMe)
{
    guard
    {
        if (FreeMe->JumpVector != NULL)
            delete (FreeMe->JumpVector);

        return;
    } unguard;
}


void KMPEvaluate (KMPContext *Context, const wchar_t *String, const int Length)
{
    guard
    {
        int i;
        wchar_t *Table;

        Context->j = 0;
        Context->Result = -1;

        if (Context->MatchCase)
            Table = IdentityTable;
        else
            Table = ToLowerTable;

        for (i = 0; i < Length  &&  Context->j < (int)Context->Pattern.length(); i++)
        {
            while (Context->j >= 0  &&  Table[String[i]] != Table[Context->Pattern[Context->j]])
                Context->j = Context->JumpVector[Context->j];

            Context->j++;
        }

        if (Context->j == Context->Pattern.length())
            Context->Result = Context->j;

        return;
    } unguard;
}


// Runs many KMP "machines" in parallel
// NOTE that this will reorder the contents of the Contexts array!
// This is done for performance reasons ...
void KMPEvaluateMany (KMPContext **Contexts, int KMPCount, const wchar_t *String, const int Length)
{
    guard
    {
        int i;
        int k;

        for (k = 0; k < KMPCount; k++)
        {
            Contexts[k]->j = 0;
            Contexts[k]->Result = -1;

            if (Contexts[k]->MatchCase)
                Contexts[k]->Table = IdentityTable;
            else
                Contexts[k]->Table = ToLowerTable;
        }

        for (i = 0; i < Length  &&  KMPCount > 0; i++)
        {
            for (k = 0; k < KMPCount; k++)
            {
                KMPContext *Context;
                wchar_t *Table;

                // Cache these values
                Context = Contexts[k];
                Table = Context->Table;

                if (Context->j == Context->Pattern.length()) // this KMP context is done! we found a match!
                {   // So as to save CPU time we will swap this pointer with the last one in the array
                    // and then decrement KMPCount, effectively hiding this KMP machine. To keep our 
                    // loop running correctly we will also decrement k
                    swap (Contexts[k], Contexts[KMPCount - 1]);
                    KMPCount--;
                    k--;

                    if (Context->j == Context->Pattern.length())
                        Context->Result = Context->j;
                }
                else
                {
                    while (Context->j >= 0  &&  Table[String[i]] != Table[Context->Pattern[Context->j]])
                        Context->j = Context->JumpVector[Context->j];

                    Context->j++;
                }
            }
        }

        return;
    } unguard;
}    
