#include "CFileLinesDB.h"
#include "CParser2.h"
#include "CScannerThread.h"


void RichTextToASCII (string *Result, cTextString *RichText)
{
    guard
    {
        int i;

        Result->resize (RichText->size());

        for (i = 0; i < RichText->size(); i++)
            (*Result)[i] = (*RichText)[i].Char;

        return;
    } unguard;
}


CFileLinesDB::CFileLinesDB (CFastFile *TheFile,
                            CParser2 *Parser,
                            width_t LineWidth, 
                            uint32 SeekGranularityLog2, 
                            uint32 CacheChunks,
                            uint32 TabSize)
: CLinesDB (TheFile),
  TextCache (CacheChunks)
{
    guard
    {
        CFileLinesDB::Parser = Parser->Clone ();
        MaxWidth = LineWidth;
        CFileLinesDB::SeekGranularityLog2 = SeekGranularityLog2;
        CFileLinesDB::SeekGranularityModAnd = (1 << SeekGranularityLog2) - 1;
        CFileLinesDB::TabSize = TabSize;
        CFileLinesDB::CacheChunks = CacheChunks;

        LastComputedSeekLine = 0;
        LastComputedSeekPoint = 0;
        LineCacheMemUsage = 0;

        LastTCElement = -1;
        LastTCArray = NULL;

        DoneScanning = true;
        ScannerThread = NULL;
        ScanFromLine (0);

        return;
    } unguard;
}


CFileLinesDB::~CFileLinesDB ()
{
    guard
    {
        delete ScannerThread;

        ClearCache ();

        delete Parser;
        Parser = NULL;

        return;
    } unguard;
}


bool CFileLinesDB::ScanFromLine (uint64 Line, 
                                 CSTCompletionCallback CallbackFn, 
                                 void *CallbackContext)
{
    guard
    {
        bool ReturnVal = false;

        Lock ();

        if (IsDoneScanning())
        {
            if (ScannerThread != NULL)
                delete ScannerThread;

            File->ResetFileSize ();
            File->ResetFileTime ();
            File->ResetCache ();
            DoneScanning = false;
            StopScanning = false;
            ScannerThread = new CScannerThread (this, Line, CallbackFn, CallbackContext);
            ScannerThread->RunThread ();
            ReturnVal = true;
        }

        Unlock ();
        return (ReturnVal);
    } unguard;
}


void CFileLinesDB::StopScanningThread (void)
{
    guard
    {
        StopScanning = true;
        return;
    } unguard;
}


void CFileLinesDB::ClearCache (void)
{
    guard
    {
        Lock ();

        while (!TextCache.IsEmpty())
            RemoveCacheElement (TextCache.GetHeadKey() << SeekGranularityLog2);

        LastTCArray = NULL;
        LastTCElement = -1;

        Unlock ();
        return;
    } unguard;
}


// Takes a CParser2::TextLine and converts it to a cTextString
void TextLineToRichText (cTextString *RichDst, CParser2::TextLine *Src, cFormat &Default)
{
    guard
    {
        int i;
        int j;

        RichDst->resize (Src->Chars);

        //
        /*
        Default.Foreground = 
            Default.Background = 
                0x00ffffff;
        */
        //

        // Copy the text
        for (i = 0; i < Src->Chars; i++)
        {
            (*RichDst)[i].Char =  Src->Text[i];
            (*RichDst)[i].Format = Default;
            (*RichDst)[i].UseFormat = true;
        }

        // Copy the formatting
        for (i = 0; i < Src->Formatting.size(); i++)
        {
            int begin;
            int end;
            CParser2::FormatExtent &Ext = Src->Formatting[i];
            cFormat &Fmt = Ext.Format;

            begin = Ext.Begin;
            end = begin + Ext.Length;

            for (j = begin; j < end; j++)
            {
                if (j >= 0  &&  j < RichDst->size())
                    (*RichDst)[j].Format = Fmt;
            }
        }

        return;
    } unguard;
}


bool CFileLinesDB::GetLine (uint64 Line, CParser2::TextLine **TextLine)
{
    guard
    {
        return (GetLine (Line, TextLine, 0));
    } unguard;
}


bool CFileLinesDB::GetLine (uint64 Line, CParser2::TextLine **TextLine, int Depth)
{
    guard
    {
        CParser2::TextLine *LinePtr = NULL;

        Lock ();

    #ifdef CLINESDB_STATS
        Stats.TotalLineReads++;
    #endif

        // If they are requesting a line that might be out of bounds, but we
        // don't know yet because we're not done scanning, return false.
        if (!DoneScanning && ((TotalLines == 0) || (Line >= TotalLines)))
        {
            Unlock ();
            return (false);
        }

        // If they are requesting a line that is "out of bounds", return NAY
        if ((Line >= TotalLines) && DoneScanning)
        {
            Unlock ();
//            guard_throw (domain_error ("line # is out of bounds for CFileLinesDB::GetLine"));
            return (false);
        }

        // If it's not in the cache, then make it be in the cache
        if (SearchLineCache (Line, &LinePtr))
        {
            if (TextLine != NULL)
                *TextLine = LinePtr;

    #ifdef CLINESDB_STATS
            Stats.CacheHits++;
    #endif

            Unlock ();
            return (true);
        }
        else
        {
    #ifdef CLINESDB_STATS        
            Stats.CacheMisses++;
    #endif
            // Line is not in cache! Make it be there!
            bool ReturnVal;

            FillWidthCacheLine (Line);
            ReturnVal = GetLine (Line, TextLine, Depth + 1);

            Unlock ();
            return (ReturnVal);
        }

        // Hrmm couldn't get it.
        Unlock ();
        return (false);
    } unguard;
}


bool CFileLinesDB::GetLineFormatted (uint64 Line, cTextString *RichText)
{
    guard
    {
        CParser2::TextLine *Data;

        Lock ();

        if (!GetLine (Line, &Data))
        {
            Unlock ();
            guard_throw (runtime_error ("could not force line into string cache"));
            return (false);
        }

        TextLineToRichText (RichText, Data, Parser->GetDefaultFormat());

        Unlock ();
        return (true);
    } unguard;
}


bool CFileLinesDB::GetLineUnicode (uint64 Line, wstring *Text)
{
    guard
    {
        CParser2::TextLine *Data;

        Lock ();

        if (!GetLine (Line, &Data))
        {
            Unlock ();
            guard_throw (runtime_error ("could not force line into string cache"));
            return (false);
        }

        Text->resize (Data->Chars);

        copy (Data->Text, Data->Text + Data->Chars, Text->begin());

        Unlock ();

        return (true);
    } unguard;
}


bool CFileLinesDB::GetLineASCII (uint64 Line, string *Text)
{
    guard
    {
        CParser2::TextLine *Data;

        Lock ();

        if (!GetLine (Line, &Data))
        {
            Unlock ();
            guard_throw (runtime_error ("could not force line into string cache"));
            return (false);
        }

        Text->resize (Data->Chars);
        transform (Data->Text, Data->Text + Data->Chars, Text->begin(), typecast_fn<char, wchar_t>());

        Unlock ();
        return (true);
    } unguard;
}


bool CFileLinesDB::IsLineInCache (uint64 Line)
{
    guard
    {
        return (SearchLineCache (Line, NULL));
    } unguard;
}


bool CFileLinesDB::SearchLineCache (uint64 Line, CParser2::TextLine **DataResult)
{
    guard
    {
        CParser2::TextLine ***Ptr;

        Lock ();

        if (LastTCElement == (Line >> SeekGranularityLog2))
        {
            if (DataResult != NULL)
                *DataResult = LastTCArray[Line & SeekGranularityModAnd];

            Unlock ();
            return (true);
        }

        Ptr = TextCache.Get (Line >> SeekGranularityLog2);

        if (Ptr == NULL)
        {
            Unlock ();
            return (false);
        }

        if (DataResult != NULL)
            *DataResult = (*Ptr)[Line & SeekGranularityModAnd];

        LastTCElement = Line >> SeekGranularityLog2;
        LastTCArray = *Ptr;

        Unlock ();
        return (true);
    } unguard;
}


uint32 CFileLinesDB::GetTabSize (void)
{
    guard
    {
        return (TabSize);
    } unguard;
}


void CFileLinesDB::SetMaxWidth (width_t NewMaxWidth)
{
    guard
    {
        if (NewMaxWidth > CLINESDB_MAXWIDTH)
            NewMaxWidth = CLINESDB_MAXWIDTH;

        MaxWidth = NewMaxWidth;
        return;
    } unguard;
}


width_t CFileLinesDB::GetMaxWidth (void)
{
    guard
    {
        return (MaxWidth);
    } unguard;
}


void CFileLinesDB::AddSeekPoint (uint64 Line, uint64 SeekPoint, bool Dependent)
{
    guard
    {
        if ((Line & SeekGranularityModAnd) == 0)
        {
            uint64 Indice;

            Lock ();

            Indice = Line >> SeekGranularityLog2;

            if (SeekPoints.size() < Indice + 1)
                SeekPoints.resize(Indice + 1);
            else
            if (SeekPoints[Indice].Offset != SeekPoint) // if they are *updating* data, then clear out the old cacheline
                RemoveCacheElement (Line);

            SeekPoints[Indice].Offset = SeekPoint;
            SeekPoints[Indice].Dependent = Dependent;

            Unlock ();
        }

        return;
    } unguard;
}


bool CFileLinesDB::GetSeekPoint (uint64 Line, uint64 &SeekResult)
{
    guard
    {
        Lock ();

        if (Line == 0)
        {
            Unlock ();
            SeekResult = 0;
            return (true);
        }

        if (Line >= TotalLines)
        {
            Unlock ();
            //guard_throw (domain_error ("domain_error: invalid line # sent to CFileLinesDB::GetSeekPoint()"));
            return (false);
        }

        // First see if we can use our "last line" cached values
        if (Line == LastComputedSeekLine)
        {
            SeekResult = LastComputedSeekPoint;
        }
        else
        if (Line == LastComputedSeekLine + 1)
        {
            SeekResult = LastComputedSeekPoint + GetLineWidth(LastComputedSeekLine);
            LastComputedSeekLine = Line;
            LastComputedSeekPoint = SeekResult;
        }
        else
        if (Line == LastComputedSeekLine - 1)
        {
            SeekResult = LastComputedSeekPoint - GetLineWidth(LastComputedSeekLine - 1);
            LastComputedSeekLine = Line;
            LastComputedSeekPoint = SeekResult;
        }
        else
        {
            uint64 i;
            uint64 First;

            First = Line - (Line & SeekGranularityModAnd);
            SeekResult = SeekPoints[Line >> SeekGranularityLog2].Offset;

            for (i = First; i < Line; i++)
                SeekResult += GetLineWidth(i);

            LastComputedSeekLine = Line;
            LastComputedSeekPoint = SeekResult;
        }

        Unlock ();
        return (true);
    } unguard;
}


width_t CFileLinesDB::GetLineWidth (uint64 Line, bool *Dependence)
{
    guard
    {
        CParser2::TextLine ***TextArray;
        width_t ReturnVal = 0;
        uint64 LDivSG;
        uint64 LModSG;

        Lock ();

        LDivSG = Line >> SeekGranularityLog2;
        LModSG = Line & SeekGranularityLog2;

        if (LastTCElement == LDivSG)
        {
            ReturnVal = LastTCArray[LModSG]->Bytes;

            if (Dependence != NULL)
                *Dependence = LastTCArray[LModSG]->Dependent;
        }
        else
        {
            // Check to see if it's in the cache
            TextArray = TextCache.Get (LDivSG);

            // If not, make it be in the cache!
            if (TextArray == NULL)
            {
                FillWidthCacheLine (Line);
                TextArray = TextCache.Get (LDivSG);
            }

            ReturnVal = (*TextArray)[LModSG]->Bytes;

            if (Dependence != NULL)
                *Dependence = (*TextArray)[LModSG]->Dependent;

            LastTCArray = *TextArray;
            LastTCElement = LDivSG;
            TextCache.BumpElement (LDivSG);
        }

        Unlock ();

        // Give it to them baby
        return (ReturnVal);
    } unguard;
}


void CFileLinesDB::RemoveCacheElement (uint64 Line)
{
    guard
    {
        CParser2::TextLine ***CacheArrayPtr;
        uint64 Key;

        Lock ();

        Key = Line >> SeekGranularityLog2;
        CacheArrayPtr = TextCache.Get (Key);

        if (CacheArrayPtr != NULL)
        {
            FreeCacheLine (*CacheArrayPtr);
            TextCache.Remove (Key);

            if (Key == LastTCElement)
                LastTCElement = -1;
        }

        Unlock ();
        return;
    } unguard;
}


CParser2::TextLine **CFileLinesDB::AllocateCacheLine (void)
{
    guard
    {
        CParser2::TextLine **Return;
        int len;
        int i;

        len = 1 << SeekGranularityLog2;
        Return = new CParser2::TextLine* [len];

        for (i = 0; i < len; i++)
            Return[i] = NULL;

        return (Return);
    } unguard;
}


void CFileLinesDB::FreeCacheLine (CParser2::TextLine **Array)
{
    guard
    {
        int len;
        int i;

        len = 1 << SeekGranularityLog2;

        for (i = 0; i < len; i++)
        {
            if (Array[i] != NULL)
            {
                if (Array[i]->Text != NULL)
                {
                    delete Array[i]->Text;
                    Array[i]->Text = NULL;
                }

                delete Array[i];
                Array[i] = NULL;
            }
        }

        delete Array;
        return;
    } unguard;
}


void CFileLinesDB::FillWidthCacheLine (uint64 Line)
{
    guard
    {
        CParser2::TextLine ***CacheArrayPtr;
        CParser2::TextLine **CacheArray;
        int Indice;
        bool Result;
        uint64 OldSeekPoint;
        uint8 Buffer[4096];
        uint64 BufferLength;
        int Cursor;
        uint64 LDivSG;
        int EndIndice;
        bool OldPOFlag;

        Lock ();

        LDivSG = Line >> SeekGranularityLog2;

        // Is it already in the cache?
        CacheArrayPtr = TextCache.Get (LDivSG);

        if (CacheArrayPtr != NULL)
        {   // Yes. Do nothing! Sorta.
            TextCache.BumpElement (LDivSG);
            Unlock ();
            return;
        }

        // Ok. Let's make our array.
        CacheArray = AllocateCacheLine ();

        File->Lock ();
        OldSeekPoint = File->GetFilePos ();
        Result = File->Seek (SeekPoints[LDivSG].Offset);

        Indice = 0;
        EndIndice = 1 << SeekGranularityLog2; // 1 more than last indice #
        OldPOFlag = Parser->GetParseOnlyFlag ();
        Parser->SetParseOnlyFlag (false);
        Parser->Reset (false);

        // If our first line is *dependent* then we must search backward
        // until we find the first line that is NOT dependent. Then we must
        // parse from there onward. This is necessary so that the proper
        // *state* of the parser is maintained that could possibly affect
        // any formatting in a spatial or colorful manner.
        // The inductive guarantee of stopping is that line #0 is always
        // non-dependent.
        uint64 SeekHere;

        SeekHere = SeekPoints[LDivSG].Offset;
        if (SeekPoints[LDivSG].Dependent)
        {
            uint64 FirstLine;
            width_t Width;
            bool D = true;

            FirstLine = (LDivSG << SeekGranularityLog2);
            SeekHere = SeekPoints[LDivSG].Offset;

            // SeekHere points to file offset for FirstLine
            do
            {
                FirstLine--;
                Indice--;
                Width = GetLineWidth (FirstLine, &D);
                SeekHere -= Width;
            } while (D == true);

            File->Seek (SeekHere);
        }

        //
        Result = File->ReadData (Buffer, sizeof(Buffer), &BufferLength);
        if (!Result)
        {
            File->Unlock ();
            Unlock ();
            return;
        }

        while (Indice < EndIndice)
        {   
            bool EndOfFile = false;

            // feed the parser
            for (Cursor = 0; Cursor < BufferLength; Cursor++)
                Parser->FeedByte (Buffer[Cursor]);

            // Should we FLUSH? Well if we just fed the last possible buffer, then yes
            if (BufferLength < sizeof(Buffer))
            {
                EndOfFile = true;
                Parser->Flush ();
            }

            // get the data from the parser
            while (Parser->GetLineCount() > 0  &&     // while there are still lines left in the accumulator ...
                Indice < EndIndice)                // and while we still give a damn ...
            {
                if (Indice < 0) // these are lines that we processed just to get the proper state of the parser ... we throw them away :(
                    Parser->RemoveFirstLine (true);
                else
                {   // yay! cache this shit!
                    CParser2::TextLine *TL;

                    TL = Parser->GetFirstLine ();

                    // We want to reallocate the text since it may be using the full MaxWidth # of bytes
                    // what if a line has only 1 character? then it's wasting massive memories! whoa!
                    // However we only do this if we'd save more than 128 bytes
                    if (MaxWidth - TL->Chars >= 64)
                    {
                        wchar_t *WText;

                        WText = new wchar_t[TL->Chars + 1];
                        StreamCopy (WText, TL->Text, TL->Chars * sizeof (wchar_t));
                        WText[TL->Chars] = L'\0';
                        delete TL->Text;
                        TL->Text = WText;
                    }

                    CacheArray[Indice] = TL;
                    Parser->RemoveFirstLine (false);
                }

                Indice++;
            }

            // read the next block of data
            Result = File->ReadData (Buffer, sizeof (Buffer), &BufferLength);

            if (!Result)
                EndOfFile = true;

            if (EndOfFile)
                break;
        }

        // Get rid of any excess stuff
        while (Parser->GetLineCount() > 0)
            Parser->RemoveFirstLine (true);

        // Is the cache full? If so, remove the oldest elements
        while (TextCache.IsFull())
            RemoveCacheElement (TextCache.GetHeadKey() << SeekGranularityLog2);

        // Any left over elements at the end (i.e. very last chunk of file) ?
        for (Indice = EndIndice - 1; Indice >= 0 && CacheArray[Indice] == NULL; Indice--)
        {
            CParser2::TextLine *TL;

            TL = new CParser2::TextLine;
            TL->Bytes = 0;
            TL->Chars = 0;
            TL->Dependent = false;
            TL->Formatting.clear ();
            TL->LastChar = Parser->InvalidChar;
            TL->Text = new wchar_t[1];
            TL->Text[0] = L'\0';

            CacheArray[Indice] = TL;
        }

        TextCache.Push (LDivSG, CacheArray);
        LastTCElement = LDivSG;
        LastTCArray = CacheArray;

        Parser->SetParseOnlyFlag (OldPOFlag);
        Parser->Reset ();

        File->Seek (OldSeekPoint);
        File->Unlock ();
        Unlock ();

        return;
    } unguard;
}


// Given a seek point, this function finds the corresponding line #
// Best for when you have NO IDEA where in the file a given seek point is
bool CFileLinesDB::SeekPointToLine (uint64 SeekPoint, uint64 &LineResult)
{
    guard
    {
        uint64 a;
        uint64 b;
        uint64 mid;
        uint64 total;

        Lock ();

        total = GetTotalLines ();

        if (total == 0)
        {
            Unlock ();

            if (SeekPoint = 0)
            {
                LineResult = 0;
                return (true);
            }

            return (false);
        }

        a = 0;
        b = total - 1;

        while (a <= b)
        {
            uint64 SPlo;
            uint64 SPhi;

            mid = (a + b) / 2;

            if (!GetSeekPoint (mid, SPlo))
            {
                Unlock ();
                return (false); // couldn't find ...
            }

            if (!GetSeekPoint (mid + 1, SPhi))
                SPhi = SPlo;

            if (SPlo <= SeekPoint  &&  SeekPoint <= SPhi)
            {
                LineResult = mid;
                Unlock ();
                return (true);
            }
            else
            if (SeekPoint < SPlo)
                b = mid - 1;
            else
            if (SeekPoint >= SPhi)
                a = mid + 1;
        }

        Unlock ();
        return (false); // couldn't find ...
    } unguard;
}

