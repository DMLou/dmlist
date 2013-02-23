#include "CHexLinesDB.h"

CHexLinesDB::CHexLinesDB (CFastFile *TheFile, width_t TextWidth, uint64 MaxCacheSize, uint8 WordSize, bool LittleEndian)
: CLinesDB (TheFile), HexLineCache (MaxCacheSize)
{
    guard
    {
        CHexLinesDB::TextWidth = TextWidth;
        DisplayWidth = TextWidth;
        CHexLinesDB::WordSize = WordSize;
        CHexLinesDB::LittleEndian = LittleEndian;
        int CharsPerWord;
        int BytesPerRow;

        if (TheFile->GetFileSize() > 0xffffffff)
            LargeOffset = true;
        else
            LargeOffset = false;

        SetLongestLineWidth (TextWidth);   

        // oooooooo ww ww ww ww ww ww ww ww ww ww ww ww ww ww ww ww bbbbbbbbbbbbbbbb.
        // |       ||                                              ||              |
        // |8,16 +1|| (((2 * WordSize) + 1) * WordCount) + 1       ||WordSize*WordCount
        //
        CharsPerWord = (3 * WordSize) + 1;
        // Calculate how many words will be on each line
        // Start with full text width
        // Subtract the length of the offset on the far left side. 8 or 16, plus 1 for a space, 1 more for space on right side
        HexWordsPerRow = TextWidth - (LargeOffset ? 16 : 8) - 1;
        // Leave 1 space on the far right
        HexWordsPerRow -= 2;
        // Divide by how many characters go into a word
        HexWordsPerRow /= CharsPerWord;

        BytesPerRow = HexWordsPerRow * WordSize;
 
        SetTotalLines ((TheFile->GetFileSize() + (BytesPerRow - 1)) / BytesPerRow);

        // Calculate what the adjusted display width is
        DisplayWidth = (LargeOffset ? 16 : 8) + 1;
        DisplayWidth += 2;
        DisplayWidth += HexWordsPerRow * CharsPerWord;

        HexReadBuffer = new uint8[BytesPerRow];
        SetDoneScanning (true);
        return;    
    } unguard;
}


CHexLinesDB::~CHexLinesDB ()
{
    guard
    {
        delete HexReadBuffer;

        while (!HexLineCache.IsEmpty())
        {
            CParser2::TextLine **Entry;

            Entry = HexLineCache.GetHead ();
            delete [] (*Entry)->Text;
            delete *Entry;
            HexLineCache.RemoveHead ();
        }

        return;
    } unguard;
}


const char SHexArray[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };


template<class IT>
void SPrintHex (wchar_t **it, IT Number, int Digits)
{
    guard
    {
        IT Mask;

        // Some weird bug if we try to do these two lines in 1 line of code ... oh well.
        Mask = 0xf;
        Mask <<= ((Digits - 1) * 4);

        while (Digits > 0)
        {
            **it = SHexArray[(Number & Mask) >> ((Digits - 1) * 4)];
            Mask >>= 4;
            ++(*it);
            Digits--;
        }

        return;
    } unguard;
}


bool CHexLinesDB::GetLine (uint64 Line, CParser2::TextLine **TextLine)
{
    guard
    {
        uint64 AmountRead;
        int i;
        CParser2::TextLine *NewText = NULL;
        CParser2::TextLine **CacheGet = NULL;
        int WordsThisRow;
        wchar_t *q;

        Lock ();

        CacheGet = HexLineCache.Get (Line);
        if (CacheGet != NULL)
        {
            *TextLine = *CacheGet;
            Unlock ();
            return (true);
        }

        File->Lock ();
        fill (HexReadBuffer, HexReadBuffer + (HexWordsPerRow * WordSize), 0);
        File->Seek (Line * HexWordsPerRow * WordSize);
        File->ReadData (HexReadBuffer, HexWordsPerRow * WordSize, &AmountRead);
        WordsThisRow = AmountRead / WordSize;
        File->Unlock ();

        NewText = new CParser2::TextLine;
        NewText->Text = new wchar_t [DisplayWidth + 1];
        NewText->Chars = DisplayWidth;
        NewText->Bytes = HexWordsPerRow * WordSize;
        NewText->Dependent = false;
        fill (NewText->Text, NewText->Text + NewText->Chars + 1, L'\0');

        q = &NewText->Text[0];
        SPrintHex (&q, Line * HexWordsPerRow * WordSize, LargeOffset ? 16 : 8);
        *q = L' ';
        *(q + 1) = L' ';
        q += 2;

        // Print hex bytes in "xx " form
        for (i = 0; i < HexWordsPerRow; i++)
        {
            if (i < WordsThisRow)
            {
                for (int w = WordSize - 1; w >= 0; w--) // little-endian
                //for (int w = 0; w < WordSize; w++) // big-endian
                    SPrintHex (&q, HexReadBuffer[(i * WordSize) + w], 2);

                *q = ' ';
                q++;
            }
            else
            {
                for (int w = 0; w < WordSize; w++)
                {
                    *q = ' ';
                    *(q + 1) = ' ';
                    q += 2;
                }

                *q = ' ';
                q++;
            }
        }

        // Print space
        *q = ' ';
        q++;

        // Print hex bytes in raw form
        for (i = 0; i < HexWordsPerRow * WordSize; i++)
        {
            char c;

            if (i >= AmountRead)
                c = 0;
            else
                c = HexReadBuffer[i];

            switch (c)
            {
                //case '\n':
                //case '\r':
                //case '\t':
                case '\0':
                    c = ' ';
                    break;
            }

            *q = wchar_t(c);
            q++;
        }

        *q = L'\0';
        NewText->Chars = q - NewText->Text;

        while (HexLineCache.IsFull())
        {
            CParser2::TextLine **Entry;

            Entry = HexLineCache.GetHead();
            delete [] (*Entry)->Text;
            delete *Entry;
            HexLineCache.RemoveHead ();
        }

        HexLineCache.Push (Line, NewText);
        *TextLine = NewText;

        Unlock ();
        return (true);
    } unguard;
}
