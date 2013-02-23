#include "ListDefaults.h"
#include "Hive.h"


// Generate default Hive path at runtime :)
string DefaultHivePath;
bool GotDefaultHivePath = GetDefaultHivePath (DefaultHivePath);


// Macros for building the variable list
#define MINMAXGRAN(TYPE,MIN,MAX,GRAN) \
      VarNamed("min",TYPE,MIN)        \
   << VarNamed("max",TYPE,MAX)        \
   << VarNamed("gran",TYPE,GRAN)      \

#define MINMAXGRAN_BOOL MINMAXGRAN(VTUint32, "0", "1", "1")

#define MINMAXGRAN_COLOR MINMAXGRAN(VTUint32, "0", "16777215", "1")

#define SCW_USEDEFAULT tostring(CW_USEDEFAULT)

// Global defaults + attributes for ListXP
// Saved to the registry in order to persist settings. Attributes are NOT saved to the registry,
// and neither are variables that start with an underscore.
VarListExt GlobalDefaults (VarListExt ()
//            Name                         | Type    |  Default Value | Attributes List
//  ---------------------------------------+---------+----------------+-----------------
    << VarNamedExt ("AllowNonCached",       VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("AlphaLevel",           VTUint32,           "200", VarList() << MINMAXGRAN (VTUint32,    "0",        "255",    "1"))
    << VarNamedExt ("AlwaysMaximized",      VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("AnimatedSearch",       VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("BlockSizeLog2",        VTUint32,            "13", VarList() << MINMAXGRAN (VTUint32,   "12",         "20",    "1") << VarNamed ("dispvalorder", VTUint32, "1"))
    << VarNamedExt ("CacheChunks",          VTUint32,             "8", VarList() << MINMAXGRAN (VTUint32,    "2",        "128",    "1"))
    << VarNamedExt ("DetectParsers",        VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("DoneFirstTime",        VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("DisableTailWarning",   VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("DisableSIMD",          VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("EditProgram",          VTString,   "wordpad.exe", VarList())
    << VarNamedExt ("FileCacheSizeKB",      VTUint32,          "2048", VarList() << MINMAXGRAN (VTUint32,    "4",    "1048576",    "4"))
    << VarNamedExt ("FontName",             VTString,      "Terminal", VarList())
    << VarNamedExt ("FontSize",             VTUint32,            "14", VarList() << MINMAXGRAN (VTUint32,    "6",         "36",    "1"))
    << VarNamedExt ("Height",               VTUint32,            "25", VarList() << MINMAXGRAN (VTUint32,   "10",        "512",    "1"))
    << VarNamedExt ("HexUseConstantWidth",  VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("HexConstantWidth",     VTUint32,            "80", VarList() << MINMAXGRAN (VTUint32,   "80",        "512",    "1"))
    << VarNamedExt ("HexWordSizeLog2",      VTUint32,             "0", VarList() << MINMAXGRAN (VTUint32,    "0",          "3",    "0") << VarNamed ("dispvalorder", VTUint32, "1"))
    << VarNamedExt ("HexLittleEndian",      VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("Highlight.Foreground", VTUint32,      "16777215", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("Highlight.Background", VTUint32,           "255", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("Highlight.Line",       VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("Highlight.Terms",      VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("Highlight.Underline",  VTUint32,             "0", VarList() << MINMAXGRAN (VTUint32,    "0",          "1",    "1"))
    << VarNamedExt ("HiveCompression",      VTUint32,             "1", VarList() << MINMAXGRAN (VTUint32,    "0",          "1",    "1"))
    << VarNamedExt ("HiveEnabled",          VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("HiveMinSizeKB",        VTUint32,         "20480", VarList() << MINMAXGRAN (VTUint32,    "0", "2147483647",    "1"))
    << VarNamedExt ("HivePath",             VTString, DefaultHivePath, VarList())
    << VarNamedExt ("IgnoreParsedColors",   VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("InfoColor.Foreground", VTUint32,             "0", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("InfoColor.Background", VTUint32,      "16777215", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("IPaid",                VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("Language",             VTString,       "English", VarList() << VarNamed ("validlist", VTString, "English"))
    // LineCacheSize is now computed as CacheChunks * 2^SeekGranularityLog2
//  << VarNamedExt ("LineCacheSize",        VTUint32,          "4096", VarList() << MINMAXGRAN (VTUint32,    "1",    "1000000",    "1"))
    << VarNamedExt ("Maximized",            VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("NonCachedMinSizeKB",   VTUint64,         "20480", VarList() << MINMAXGRAN (VTUint32,    "4", "2147483647",    "4"))
    << VarNamedExt ("Prefetching",          VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("SearchLinesPerJob",    VTUint32,           "128", VarList() << MINMAXGRAN (VTUint32,    "1",    "1048576",    "1"))
    << VarNamedExt ("SeekGranularityLog2",  VTUint32,            "10", VarList() << MINMAXGRAN (VTUint32,    "2",         "24",    "1") << VarNamed ("dispvalorder", VTUint32, "1"))
    << VarNamedExt ("TabSize",              VTUint32,             "8", VarList() << MINMAXGRAN (VTUint32,    "1",         "64",    "1"))
    << VarNamedExt ("TailingInterval",      VTUint32,             "5", VarList() << MINMAXGRAN (VTUint32,    "1", "2147483647",    "1"))
    << VarNamedExt ("TailingIntervalUnit",  VTString,       "seconds", VarList() << VarNamed ("validlist", VTString, "milliseconds^seconds^minutes^hours^days"))
    << VarNamedExt ("TailingBacktrackKB",   VTUint32,            "64", VarList() << MINMAXGRAN (VTUint32,    "1", "2147483647",    "1"))
    << VarNamedExt ("TailingForceUpdate",   VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("TailingJumpToEnd",     VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("TextColor.Foreground", VTUint32,      "13158600", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("TextColor.Background", VTUint32,             "0", VarList() << MINMAXGRAN_COLOR)
    << VarNamedExt ("Transparent",          VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("UpdatesFirstTime",     VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("UpdatesEnabled",       VTUint32,             "1", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("UpdatesLastChecked",   VTUint64,             "0", VarList())
    << VarNamedExt ("UpdatesCheckInterval", VTUint32,             "7", VarList() << MINMAXGRAN (VTUint32,    "1",         "90",    "1"))
    << VarNamedExt ("Width",                VTUint32,            "80", VarList() << MINMAXGRAN (VTUint32,   "80",        "512",    "1"))
    << VarNamedExt ("WrapText",             VTUint32,             "0", VarList() << MINMAXGRAN_BOOL)
    << VarNamedExt ("X",                    VTString,  SCW_USEDEFAULT, VarList())
    << VarNamedExt ("Y",                    VTString,  SCW_USEDEFAULT, VarList())
    << VarNamedExt ("CustomColors",         VTBinary, "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 ", VarList())

    // These are used in the preferences dialog, the prefixed underscore indicates volatile variables that are not saved to the registry
    << VarNamedExt ("ColorList",            VTString, "Infobar Background", VarList() << VarNamed ("validlist", VTString, "Highlight Background^Highlight Foreground^Infobar Background^Infobar Foreground^Text Background^Text Foreground"))

    << VarNamedExt ("_IPaidExplain",        VTString, 
                    "Click on the 'I paid!' button if you've donated money already. This will hide the "
                    "'Donate via PayPal' nag-button that's in the lower-left of this Preferences "
                    "dialog. Click on the 'Just kidding' button if you have not actually donated money.", 
                    VarList())

    << VarNamedExt ("_DonateExplain",       VTString, 
                    "If you use " LISTTITLE " on a regular basis, please donate money via PayPal. This "
                    "will help ensure that development continues with new features, bug fixes, and "
                    "performance improvements. This is not required, however. You may continue to use "
                    LISTTITLE " without sending me any money.", 
                    VarList())

    << VarNamedExt ("_HiveExplain",         VTString, 
                    "The index hive is a folder where " LISTTITLE " can store structural information "
                    "about large files. With this data, scanning the file can be avoided in subsequent "
                    "sessions of " LISTTITLE ". If you work with large files that don't change often, "
                    "it is recommended that you enable this.\n"
                    "\n"
                    "Index data for a file will not be saved to the hive unless the file is larger "
                    "than the specified minimum size.", 
                    VarList())

    << VarNamedExt ("_RMBExplain",          VTString, 
                    "Enabling the shell integration will add a 'List' verb to the right-click menu for "
                    "all files. This is a convenient way to open any file using " LISTTITLE ".\nThis "
                    "setting affects all users.", VarList())

    << VarNamedExt ("_EditExplain",         VTString, 
                    "When you choose the Edit command from the File menu, the given external application "
                    "will be launched to edit the file that is currently open.", 
                    VarList())

    << VarNamedExt ("_DisplayExplain",      VTString, 
                    "The transparent window option is only available in Windows 2000, XP or later. It "
                    "is not recommended that you enable this option unless your video card has support "
                    "for hardware alpha blending, such as those based on the nVidia GeForce or ATI Radeon chips.", 
                    VarList())

    << VarNamedExt ("_CachingExplain",      VTString, 
                    "" LISTTITLE " can perform its own I/O caching for superior performance and a much "
                    "lower overall system performance impact. This is accomplished by avoiding \"pollution\" "
                    "of the global Windows file cache.\n"
                    "\n"
                    "Prefetching is a method of reading and caching data before it is used to avoid stalls "
                    "in the scanning and searching pipelines. It is highly recommended you leave this enabled.", 
                    VarList())

    << VarNamedExt ("_TailingExplain",      VTString, 
                    "Tailing is a method of monitoring a file's size and rescanning the new end of the "
                    "file if it has grown.\n"
                    "\n"
                    "However, enabling tailing is only recommended for files you know will only have data "
                    "appended to them. It is possible that a file could change somewhere else, in which case "
                    LISTTITLE " may not reflect the correct structure of the file's data.", 
                    VarList())

    << VarNamedExt ("_TailingNotice",       VTString, 
                    "Tailing is a very powerful feature; however, it is possible that enabling this will "
                    "cause " LISTTITLE " to show incorrect data if the file is changed anywhere besides "
                    "at its very end.\n"
                    "\n"
                    "It is recommended you use tailing only on files you know will only have data appended "
                    "to them.", 
                    VarList())

    << VarNamedExt ("_UpdatesExplain",      VTString, 
                    "" LISTTITLE " can check every so often to see if a new version is available. This "
                    "is done transparently in the background and sends no data over the Internet from "
                    "your computer. A text file is simply downloaded from the " LISTTITLE " website and "
                    "parsed for certain information.\n\nUpdates can only be checked for when " LISTTITLE 
                    " is open and only if you are connected to the Internet.", 
                    VarList())

    << VarNamedExt ("_SearchingExplain",    VTString, 
                    "Animated searches will graphically show the progress of a search operation at the "
                    "expense of performance.\n",
                    VarList())
);


// Other important information not related to the Preferences system, and which doesn't need attributes
const VarList GlobalInfo (VarList ()
    << VarNamed ("Website",          VTString, LISTHOMEPAGE)
    << VarNamed ("E-mail",           VTString, LISTEMAIL)
    << VarNamed ("Version",          VTString, LISTVERSION)
    << VarNamed ("ProgramName",      VTString, LISTTITLE)
    << VarNamed ("FullName",         VTString, LISTTITLE " " LISTVERSION)
    << VarNamed ("UpdatesFile",      VTString, "listxp_updateinfo.txt")
    << VarNamed ("Credits.PCREPage", VTString, "http://pcre.sourceforge.net/")
    << VarNamed ("Credits.AVLPage",  VTString, "http://purists.org/avltree/")
    << VarNamed ("Credits.IconPage", VTString, "http://www.tercerangel.com.ar/")
);
