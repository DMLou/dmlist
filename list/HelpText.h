/*****************************************************************************

  HelpText.h

  This file contains all of the strings for the help system.

*****************************************************************************/

#ifndef _HELPTEXT_H
#define _HELPTEXT_H


// Define the language? Not really a v1.0 feature ya know
#define LIST_ENGLISH
//#define LIST_SPANISH
//#define LIST_FRENCH


// Help pages
// ----------
// All help pages should be formatted to fit 80x23, although the last line should
// not exceed 79 characters

const char ListHelpHeader[] = 
    " " LISTTITLE " " LISTVERSION " by Rick Brewster (rbrewster@wsu.edu) (" __DATE__ " @ " __TIME__ ")\n";

const char ListEnglishHelpPage1[] = 
    " Keyboard and mouse commands                                                   \n"
    " Up,Down,Left,Right,Page Up,Page Down,Home,End = These work as expected        \n" 
    " Esc  Press twice to quit. Pressing once will cancel any in-progress search    \n" 
    "  F1  Toggle this help display, or go to the next help page. Esc closes help.  \n" 
    "  w   Toggle line wrapping                      *      Toggle ASCII junk       \n" 
    "  \\   Search for text (case insensitive)      Ctrl+F   Search for text         \n"
    "  |   Search for text (case sensitive)          a      Toggle animated search  \n"
    "  n   Find next occurence of text               c      Toggle case sensitive   \n" 
    "  N   Find previous occurence of text           e      Edit search string      \n" 
    "  r   Reset next search to beginning of file    d      Toggle search direction \n" 
    "  t   Reset next search to the current line    s/F3    Perform search again    \n" 
    "  h   Toggle hexadecimal view                   g      Toggle search type      \n" 
    "  +   Type # lines to jump forward            l/b/x    Set search type         \n" 
    "  -   Type # lines to jump backward             #      Type line # to jump to  \n"
    "  h   Toggle hexadecimal view                  Tab     Toggle file info/stats  \n"
    "  q   Clear all caches                         Ctrl+T  Toggle tailing          \n"
    "                                               Space   Pagedown                \n"
    "                                                                               \n"
    "                                         Press F1 for the next page of help -> \n";
    /*
    "  u      Mark top line                          < >    Go to next/prev bookmark\n"
    "  j      Mark bottom line                                                      \n"
    "  m      Mark middle line                                                      \n"*/;

const char ListEnglishHelpPage2[] =
    " Copy and paste help:                                                          \n"
    " Highlight a rectangular region by holding down the left mouse button and      \n" 
    " dragging. Highlight a a 'line-wrap' region by holding down Shift or Ctrl with \n" 
    " the left mouse button. Then you can press the right mouse button to copy the  \n" 
    " selection to the clipboard. Press Shift with the right mouse button to paste  \n" 
    " from the clipboard. Use the mousewheel to scroll up or down. Optionally hold  \n" 
    " down shift or control to scroll by pages.                                     \n"
    "                                                                               \n"
    "                                                                               \n"
    "                                         Press F1 for the next page of help -> \n";

const char ListEnglishHelpPage3[] = 
    " Boolean Search Help:                                                          \n" 
    " The following operators may be used in a Boolean search expression:           \n" 
    "     and or not xor eqv xnor & && | || ^ = == != ~ !                           \n" 
    " Strings not separated with Boolean operators are implicitely separated using  \n" 
    " the 'and' operator. & and && are synonyms for 'and', | and || are synonyms    \n" 
    " for 'or', ^ and != are synonyms for 'xor', = and == are synonyms for 'eqv',   \n" 
    " ~ and ! are synonyms for 'not'.                                               \n" 
    " The way Boolean expressions are evaluated is that each search string is first \n" 
    " matched against a given line of text from the file. The string evaluates to   \n" 
    " 'true' if it present, or 'false' if it is not. Then the true and false terms  \n" 
    " are substituted into the boolean expression in place of the search strings and\n" 
    " the proper Boolean expression is evaluated.                                   \n" 
    " The inclusion of operators such as 'xor' may seem odd at first, as most       \n" 
    " search evaluators (i.e. web search engines) do not support them. If you were  \n" 
    " to search for 'bob xor jane', " LISTTITLE " will find lines that contain 'bob' but   \n" 
    " don't contain 'jane', and lines that do not contain 'bob' but do contain      \n" 
    " 'jane'. 'eqv' and '==' work in the opposite way in that they will match lines \n" 
    " that contain either both or neither of the terms.                             \n" 
    " Strings may be atomized using double-quotations, and evaluation order may be  \n" 
    " forced using parenthesis. For example:                                        \n" 
    "     bob and \"jill doe\" or (taco and (grande or cheese)) or \"taco grande\"      \n";


const char *ListHelpPages[] = 
{
    ListEnglishHelpPage1,
    ListEnglishHelpPage2,
    ListEnglishHelpPage3
};


const int ListHelpPageCount = 3;


#endif // _HELPTEXT_H