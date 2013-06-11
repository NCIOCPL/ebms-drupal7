/**********************************************************************
 * $Id$
 *
 * Process an EBMS print job
 *
 * This program runs on a user's workstation.  It takes output created
 * by an EBMS print job on the EBMS server and downloaded to the workstation,
 * and directs that output to a printer, using software configured to
 * print the various types of document.
 *
 * Processing is controlled by a configuration file named RunEbmsPrintJob.cfg,
 * that must be located in the same directory as this program.
 *
 * A log file is also created in the same directory with a record of what
 * happened in each run.
 *
 *                                              Alan Meyer
 *                                              April, 2013
 *********************************************************************/

#include <windows.h>
#include <process.h>
#include <time.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>

using namespace std;

// Types: Configuration name key = config value
//        In memory version of data loaded from the config file
typedef map<const string, string> cfgMap;

// Types: Pairs of filename, description, in print order
//        Data originates on the EBMS server
typedef pair<string, string> printPair;
typedef vector<printPair> printVector;


// Forward declarations
class Config;
void   fatal(const string msg);
void   log(const string msg, bool timestamp=false);
void   log(const char * msg, bool timestamp=false);
bool   fileExists(const string& fileName);
bool   dirExists(const string& dirPath);
bool   getReportListItem(const char **positionp, size_t *itemLength);
void   untar(Config& cfg);
void   runCommand(const string& cmd, const string& msg="", const int timeout=0);
void   usage(Config cfg, string msg="");
string whereami();
string quoteIt(string inString);
string deQuoteIt(string inString);
string doubleSlash(string inString);
string lowerStr(string& mixed);
string findHighestTarFile(string dirPath, string& jobid);
string makePdfPrintCmd(Config& cfg, const string& fileName);
string makeDocPrintCmd(Config& cfg, const string& fileName);
string makeReportCmd  (Config& cfg, const string& fileName);
printVector getReportList(const string& rptFileName);
printVector getPrintList(const string& jobFilesName);

// We allocate this for every buffer used for a file name or component
// Should be safe since Windows MAX_PATH is only 256, but we check to be sure
const int bufsize = 1024;

// Names of default values stored in the config file
static const string s_defNames[] = {
    "def_folder",
    "def_printer",
    "def_pdfprinter",
    "def_pdfprintargs",
    "def_docprinter",
    "def_docprintargs",
    "def_untar",
    "def_untarargs",
    "def_browser"
};

// Name of configuration file
// Format is lines of:
//   name=value
// Do not put quotes around value, even if it is a file name with spaces
const string configFileName = "RunEbmsPrintJob.cfg";

// Name of backup created if a user stores changes
const string configFileBak = "RunEbmsPrintJob.cfg.bak";

// Log file, in the same directory as program and config
const string logfileName = "RunEbmsPrintJob.log";

// Fully qualified path to log file - initialize before using
static string s_logfile = "";

// Added this later, easiest to make it static
static bool s_debug = false;

/**
 * Fatal error handler.
 *
 * This logs the error, displays it to stderr, and exits.
 *
 *  @param msg      Error message.
 */
void fatal(const string msg) {
    log(msg + " - Exiting");
    cerr << msg << "\nExiting RunEbmsPrintJob" << endl;
    exit(1);
}

/**
 * Start logging.
 */
void startLog() {

    if (s_logfile == "")
        s_logfile = whereami() + logfileName;
    log("\n");
    log(" ======= Starting PrintJob =========", true);
    if (s_debug)
        log(" ========== Debug Mode =============\n", true);
    cout << "Logfile: " << s_logfile << endl;
}

/**
 * End logging.
 */
void endLog() {
    log("\n");
    if (s_debug)
        log(" =========== Debug Mode ============", true);
    log(" ======== Printing complete ========\n", true);
}

/**
 * Front end to string logging for C strings.
 */
void log(const char *msg, bool timestamp) {
    log(string(msg), timestamp);
}

/**
 * Log a message.  s_logfile must have been initialized first.
 * See class Config.setParms().
 *
 *  @param msg      What to log.
 */
void log(const string msg, bool timestamp) {

    // Can only log if we've gotten far enough to have a log filename
    if (s_logfile == "")
        return;

    // Timestamp
    char timeBuf[80];
    if (timestamp) {
        time_t ltime;
        time(&ltime);
        const char *ascTime = asctime(localtime(&ltime));

        // Chomp off the terminating newline created by asctime
        int  timeLen = strlen(ascTime) - 1;
        strncpy(timeBuf, ascTime, timeLen);
        timeBuf[timeLen] = '\0';
    }

    // Open file for append
    ofstream logf;
    logf.open(s_logfile.c_str(), ios::app);
    if (logf.is_open()) {
        if (timestamp)
            logf << timeBuf << ": ";
        logf << msg << endl;
        logf.close();
    }

    // And to the console
    cout << msg << "\n";
}

/**
 * Find the path to the currently running executable.
 * We're going to use it to locate the config file and the log file
 * in the same directory.
 *
 *  @return             Directory path including drive letter and trailing '\'
 *                      e.g., "C:\here\there\"
 *
 *  Errors are fatal.
 */
string whereami() {

    // Get full path to currently executing filename
    char namebuf[bufsize];
    GetModuleFileName(NULL, namebuf, bufsize);

    // Parse out the part before the filename
    char drivebuf[bufsize];
    char dirbuf  [bufsize];
    char fnamebuf[bufsize];
    char extbuf  [bufsize];
    _splitpath(namebuf, drivebuf, dirbuf, fnamebuf, extbuf);

    // Check for overruns
    if (strlen(drivebuf) + strlen(dirbuf) > bufsize)
        fatal("whereami: Pathname overrun, please tell programming staff");

    // Paste the drive and directory together
    string dirpath = (string) drivebuf + (string) dirbuf;

    return dirpath;
}

/**
 * Class holding all of the configuration information from the config file
 * and all parameters loaded from the command line or inferred by the
 * program logic.
 */
class Config {

    // Contents
    private:
        cfgMap configMap;       // Configuration parameter key = value
        bool   configChanged;   // True = need to save config changes
        cfgMap parmMap;         // Parameter key = value

    public:
        /**
         * Constructor.
         */
        Config::Config() {

            // Initialize map with empty values
            for (int i=0; i<(sizeof(s_defNames)/sizeof(string)); i++)
                configMap[s_defNames[i]] = "";

            // Load values from the config file
            load();
        }

        /**
         * Is there a key in the map of configuration entries?
         *
         *  @param key      Name of the key to check.
         *
         *  @return         True = yes.  Else no.
         */
        bool isConfigKey(string key) {
            if (configMap.find(key) == configMap.end())
                return false;
            return true;
        }

        /**
         * Is there a config value set for a given key?
         *
         *  @param key      Name of the item to check.
         *
         *  @return         True = yes there, false = no.
         */
        bool chkConfigKey(string key) {
            // Maps always have exactly 1 or 0 occurences of a key
            if (configMap.count(key) == 1)
                return true;
            return false;
        }

        /**
         * Get the value for a configuration item.
         * Abort on error.
         *
         *  @param key      Name of the item to retrieve.
         *  @param mkQuote  True = put quotes around it.
         *  @param required True = fatal error if not found
         *
         *  @return         Value.
         */
        string getConfigValue(const string key, bool mkQuote=false,
                              bool required=true) {

            if (!isConfigKey(key))
                fatal(
                  "Internal error!  Trying to get unknown configuration key: "
                  + key);

            // All legal keys were initialized by load(), so this can't
            //  fail to find something
            cfgMap::iterator it = configMap.find(key);
            if (it->second == "" && required)
                fatal ("No configuration set for " + key);

            if (mkQuote)
                return quoteIt(it->second);

            return it->second;
        }

        /**
         * Is there a parameter value set for a given key?
         *
         *  @param key      Name of the item to check.
         *
         *  @return         True = yes there, false = no.
         */
        bool chkParmKey(string key) {
            // Maps always have exactly 1 or 0 occurences of a key
            if (parmMap.count(key) == 1)
                return true;
            return false;
        }

        /**
         * Get the value of a runtime parameter.
         * Abort on error.
         *
         *  @param key      Name of the item to retrieve.
         *  @param mkQuote  True = put quotes around it.
         *
         *  @return         Value.
         */
        string getParmValue(string key, bool mkQuote=false) {

            cfgMap::iterator it = parmMap.find(key);
            if (it == parmMap.end())
                fatal ("No parameter set for " + key);

            if (mkQuote)
                return quoteIt(it->second);

            return it->second;
        }


        /**
         * Modify a value in the map.
         * Aborts if error.
         *
         *  @param key      Name of the key.
         *  @param value    New value.
         */
        void addItem(string key, string value) {

            // Don't add anything we don't expect
            if (!isConfigKey(key))
                fatal("Internal error!  Unknown configuration key: " + key);

            // Add it
            configMap[key] = value;

            // Config changed, may no longer match disk
            configChanged = true;
        }

        /**
         * Was the configuration changed (addItem was called)?
         *
         * @return          true = yes, changed.
         */
        bool isConfigChanged() {
            return configChanged;
        }

        /**
         * Load the Config object with the contents of a file.
         *
         * The file must be named by the conventional name and be in the
         * standard conventional format:
         *   def_printer=abcxyz
         *   def_folder=...
         *   ...
         *
         */
        void load() {

            // Get path to executable file
            string path = whereami();

            // Append filename
            path += configFileName;

            // Shouldn't be quotes here, but just in case
            path = deQuoteIt(path);

            // Open it
            const char *cpath = path.c_str();
            ifstream cfgFile(cpath);
            if (!cfgFile.is_open()) {
                perror(cpath);
                fatal("Unable to open configuration file for reading");
            }

            // For line and tokens from each line
            char lineBuf[bufsize];
            char *cfgName;
            char *cfgValue;
            char *seps = "=";

            while (true) {
                cfgFile.getline(lineBuf, bufsize - 1);
                if (cfgFile.gcount() == 0)
                    break;
                cfgName  = strtok(lineBuf, seps);
                cfgValue = strtok(NULL, seps);

                // Blank line?
                if (*cfgName == '\n')
                    continue;
                // Comment line?
                if (*cfgName == '#')
                    continue;

                if (cfgValue == NULL || *cfgValue == '\n')
                    fatal(string("Missing value for \"") + cfgName +
                          "\" in config file");

                // Chomp newline from value
                size_t len = strlen(cfgValue);
                if (*(cfgValue + len - 1) == '\n')
                    *(cfgValue + len - 1) = 0;

                // Add item to the configuration object
                addItem(cfgName, cfgValue);
            }
            cfgFile.close();

            // The in-memory object now exactly matches what's on disk
            configChanged = false;
        }

        /**
         * Store the current configuration, backing up the old config file.
         */
        void store() {

            // Absolute path version of the filename
            string filePath = whereami();
            string backPath = filePath;
            filePath += configFileName;
            backPath += configFileBak;
            filePath = deQuoteIt(filePath);
            backPath = deQuoteIt(backPath);

            // Save the old configuration, ignore failure
            rename(filePath.c_str(), backPath.c_str());

            // Open a new output file
            ofstream cfgFile(filePath.c_str());

            // Get and save all of the key=value pairs
            // This method always writes them in the same order
            string name;
            string value;
            for (int i=0; i<(sizeof(s_defNames)/sizeof(string)); i++) {
                name  = s_defNames[i];
                value = getConfigValue(name);
                cfgFile << name << "=" << value << endl;
            }

            cfgFile.close();

            // Don't need to save again
            configChanged = false;
        }

        /**
         * Initialize the parameters that control the operation of the
         * program.
         *
         *  @param argc     Count of command line parameters.
         *  @param argv     Array of pointers to command line arguments.
         *
         * All errors are fatal.
         */
        void setParms(int argc, char *argv[]) {

            // Initial values taken from the config file
            parmMap["folder"]  = getConfigValue("def_folder");
            parmMap["printer"] = getConfigValue("def_printer");

            // Don't have these parms yet
            parmMap["start"]  = "1";
            parmMap["end"]    = "999999";
            parmMap["report"] = "";

            // Get required job ID from command line
            if (argc < 2)
                usage("Missing required PrintJob ID parameter");
            parmMap["jobid"] = argv[1];

            // Get any optional args from the command line
            string opt;
            string val;
            while (optParse(argc, argv, opt, val)) {
                // Check for required values
                if (opt != "report" && opt != "help" && val == "" &&
                    opt != "debug")
                    fatal("Option \"" + opt + "\" requires a value");

                // Changes to defaults
                if (opt == "def_folder") {
                    if (!dirExists(val))
                        fatal("Folder \"" + val + "\" not found");
                    addItem("def_folder", val);
                }
                else if (opt == "def_printer") {
                    // Don't know how to validate this one
                    addItem("def_printer", val);
                }
                else if (opt == "def_pdfprinter") {
                    if (!fileExists(val))
                        fatal("pdfprint software at \"" + val + "\" not found");
                    addItem("def_pdfprinter", val);
                }
                else if (opt == "def_pdfprintargs") {
                    addItem("def_pdfprintargs", val);
                }
                else if (opt == "def_docprinter") {
                    if (!fileExists(val))
                        fatal("Word processing software at \"" + val
                               + "\" not found");
                    addItem("def_docprinter", val);
                }
                else if (opt == "def_docprintargs") {
                    addItem("def_docprintargs", val);
                }
                else if (opt == "def_untar") {
                    if (!fileExists(val))
                        fatal("Tar extraction software at \"" + val
                               + "\" not found");
                    addItem("def_untar", val);
                }
                else if (opt == "def_untarargs") {
                    addItem("def_untarargs", val);
                }
                else if (opt == "def_browser") {
                    if (!fileExists(val))
                        fatal("Web browser software at \"" + val
                               + "\" not found");
                    addItem("def_browser", val);
                }

                // Run-time options
                else if (opt == "folder")
                    parmMap["folder"] = val;
                else if (opt == "printer")
                    parmMap["printer"] = val;
                else if (opt == "start")
                    parmMap["start"] = val;
                else if (opt == "end")
                    parmMap["end"] = val;
                else if (opt == "report")
                    parmMap["report"] = "report";
                else if (opt == "debug")
                    s_debug = true;
                else if (opt == "help")
                    usage();
                else
                    usage(string("Unrecognized command line option \"--")
                                  + opt + "\"");
            }

            // Check folder
            string folder = parmMap["folder"];
            if (!dirExists(folder))
                fatal("Cannot find document folder \"" + folder + "\"");

            // Normalize trailing slash
            if (folder[folder.length() - 1] != '\\') {
                folder += "\\";
                parmMap["folder"] = folder;
            }

            // For numeric form of jobid
            char   namebuf[bufsize];
            int    jobIdNum;
            string jobid = parmMap["jobid"];

            // Check that file exists for the job
            jobIdNum = atoi(jobid.c_str());
            if (jobIdNum < 1 || jobIdNum > 99999)
                usage("jobid \"" + jobid + "\" should be a number 1..99999");
            sprintf(namebuf, "PrintJob%05d.tar", jobIdNum);
            parmMap["tarFile"] = string(namebuf);
            if (!fileExists(folder + parmMap["tarFile"]))
                fatal("No file in folder with required name \""
                       + string(namebuf) + "\"");

            // We'll use this string again, keep it around
            char numbuf[20];
            sprintf(numbuf, "%05d", jobIdNum);
            parmMap["zeroIdNum"] = string(numbuf);

            // Knowing the job ID we can identify where the files go
            string dataDir = folder + "PrintJobs\\" + "PrintJob" +
                             parmMap["zeroIdNum"] + "\\";
            parmMap["dataDir"] = dataDir;

            // This file will exist for every print job
            parmMap["reportFile"] = parmMap["dataDir"] + "PrintJobReport" +
                                    parmMap["zeroIdNum"] + ".html";

            // Double any backslashes to handle C/Windows backslash
            //  ambiguities
            /*
            parmMap["folder"]       = doubleSlash(parmMap["folder"]);
            parmMap["printer"]      = doubleSlash(parmMap["printer"]);
            parmMap["pdfprinter"]   = doubleSlash(parmMap["pdfprinter"]);
            parmMap["pdfprintargs"] = doubleSlash(parmMap["pdfprintargs"]);
            parmMap["docprinter"]   = doubleSlash(parmMap["docprinter"]);
            parmMap["docprintargs"] = doubleSlash(parmMap["docprintargs"]);
            parmMap["browser"]      = doubleSlash(parmMap["browser"]);
            */

            // Initialize logging
            startLog();
        }

        /**
         * Simple command line option parser.  Only handles longopt style.
         *
         *  @param argc         Count of cmd line args.
         *  @param argv         Array of pointers to them.
         *  @param opt          Ref to option string, without leading "--"
         *  @param val          Ref to option value
         *
         *  @return             True = have another options, else we're done.
         *
         * Upon true return the option name is copied to caller's opt
         * string and similar for value.
         *
         * All errors fatal.
         */
        bool optParse(int argc, char *argv[], string& opt, string& val) {

            // Track where we are in passed argc/argv
            //  arg[0] = program name
            //  arg[1] = Job ID
            //  arg[2] = first optional command line arg
            static int argnum = 2;

            if (argnum >= argc)
                // We've reached the end
                return false;

            if (strncmp(argv[argnum], "--", 2) == 0) {
                opt = argv[argnum]+2;
                opt = lowerStr(opt);
                ++argnum;
            }
            else
                // opt is mandatory, val is optional
                usage(string("Unexpected command line argument \"")
                              + argv[argnum] + "\"\n");

            if (argnum == argc || strncmp(argv[argnum], "--", 2) == 0) {
                // val is optional
                val = "";
            }
            else {
                val = argv[argnum];
                ++argnum;
            }

            return true;
        }

        /**
         * Display error message, usage message, and exit.
         *
         *  @param cfg      Config object with all default values initialized.
         *  @param msg      Optional error message.
         */
        void usage(string msg="") {

            if (msg != "")
                cerr << "\nError: " << msg << endl;

            cerr
 << "\nDirect the contents of an EBMS print job to a printer\n\n"
 << "usage:  RunEbmsPrintJob JobID {--options}\n\n"
 << "  Job ID is the job number reported by the EBMS server, also found as\n"
 << "  final digits of the downloaded tarfile name,"
 <<    " e.g. 29 in PrintJob00029.tar\n\n"
 << "Options:\n"
 << "  --folder  {string}  Find jobs in this folder, default="
 <<    getConfigValue("def_folder") << "\n"
 << "  --printer {string}  Use this printer, default="
 <<    getConfigValue("def_printer") << "\n"
 << "  --start   {number}  Start printing with this doc, default=1\n"
 << "  --end     {number}  Last doc to print, default=last one\n"
 << "  --report            Don't print anything, just show the report\n"
 << "  --debug             Don't print anything, just show commands\n"
 /* The following may be confusing to users.
  * Keep the logic but kill the messenger
 << "  --help              Display this usage help and exit\n\n"
 << "Changing defaults.\n"
 << "To change a default value, use these parameters.  No printing is done:\n"
 << "  --def_folder       {string} Change default folder to {string}\n"
 << "  --def_printer      {string} Change default printer to {string}\n"
 << "  --def_pdfprinter   {string} Change path to pdfprint software\n"
 << "  --def_pdfprintargs {string} Change arguments passed to pdfprinter\n"
 << "  --def_docprinter   {string} Change path to Word doc print software\n"
 << "  --def_docprintargs {string} Change arguments passed to docprinter\n"
 << "  --def_browser      {string} Change path to web browser for reports"
 */
 << endl;

            exit(1);
        }
};

/**
 * Does a file exist?
 *
 *  @param fileName     Check for this file, including optional path.
 *
 *  @return             True = file exists.
 */
bool fileExists(const string& fileName) {

    // Quotes are only wanted when passing filenames to cmd.exe
    string fName = deQuoteIt(fileName);

    ifstream fstrm(fName.c_str());
    if (fstrm.is_open()) {
        fstrm.close();
        return true;
    }
    return false;
}

/**
 * Does a directory exist?
 * Uses WinAPI.
 *
 *  @param dirPath      Directory name with optional path.
 *
 *  @return             True = exists and is a directory.
 */
bool dirExists(const string& dirPath) {

    // Quotes are only wanted when passing path to cmd.exe
    string dPath = deQuoteIt(dirPath);

    DWORD fType = GetFileAttributesA(dPath.c_str());
    if (fType & FILE_ATTRIBUTE_DIRECTORY)
        return true;
    return false;
}

/**
 * Identify the highest numbered PrintJobNNNNN.tar file in the directory
 * of print job tar files.
 *
 *  @param dirPath      Search here.  String includes trailing slash.
 *  @param jobid        Reference to place to put job ID as a string.
 *
 *  @return             Fully qualified path to the file.
 *
 *  NOTE: It was decided not to use this function but to require the user
 *        to specify a job ID on the command line.  That's safer.
 *        We keep it here in case it is ever needed again.
 */
string findHighestTarFile(string dirPath, string& jobid) {

    int             tarNum;     // Number of the PrintJob tar file found
    int             maxTarNum;  // Highest number in tar file names found
    int             rc;         //
    string          foundName;  // Filename to return
    WIN32_FIND_DATA wfd;        // Struct filled in by WinAPI
    HANDLE          hFind;      // Windows object handle

    // Search for files matching this pattern
    string pathPat = dirPath + "PrintJob*.tar";

    // Search
    hFind = FindFirstFile(pathPat.c_str(), &wfd);
    if (hFind == INVALID_HANDLE_VALUE)
        // Nothing found
        fatal("No PrintJob*.tar files in folder \"" + dirPath + "\"");

    // Loop through results
    maxTarNum = -1;
    while (true) {

        // Get the number
        sscanf(wfd.cFileName, "PrintJob%d.tar", &tarNum);
        if (tarNum > maxTarNum) {
            maxTarNum = tarNum;
            foundName = wfd.cFileName;
        }

        // Next
        rc = FindNextFile(hFind, &wfd);
        if (!rc)
            break;
    }
    FindClose(hFind);

    // Job ID as a string
    stringstream jobstr;
    jobstr << tarNum;
    jobid = jobstr.str();

    return foundName;
}


/**
 * Untar a tar file.
 *
 *  @param cfg          Reference to Config file object.  All parms from there
 *
 * All errors are fatal.
 */
void untar(Config& cfg) {

    // Identifiers from the job or configuration
    string tarFile = cfg.getParmValue("tarFile");
    string folder  = cfg.getParmValue("folder");
    string tarProg = cfg.getConfigValue("def_untar");

    // The fully qualified pathname of the tar file from which to extract
    string tarPath = folder + tarFile;

    /* KLUDGE alert
     *   I could not get GNU tar to work with drive letters.
     *   It wants the cygwindrive, not c:, L:, etc. though it will accept
     *    a UNC name for a network drive.
     *   So, for tar only, I will do some hatchet work on drive letters
     */
    string tarDriveId = "";
    string tarPathName = tarPath;
    if (tarPathName.substr(1, 1) == ":") {
        tarDriveId  = tarPathName.substr(0,2);
        tarPathName = tarPathName.substr(2, 9999);
    }

    string qTarPath = quoteIt(tarPathName);

    // Does the tar file exist?
    if (!fileExists(tarPath)) {
        string msg = "Could not find tar file: " + qTarPath;
        if (tarDriveId.length() > 0) {
            msg += " - Drive letter in def_folder may be the problem, "
                   " Please inform the programmer.";
            log(msg);
        }
    }

    // Find optional tar parameters, gets "" if value never set
    string tarArgs = cfg.getConfigValue("def_untarargs", false, false);

    // Does the untar program exist
    if (!fileExists(tarProg))
        fatal("Can't find tar extract program \"" + tarProg + "\"");

    // Set environment variable for gnu tar to accept drive: and backslash
    // But it doesn't actually work in my tests
    SetEnvironmentVariable("CYGWIN", "nodosfilewarning");

    // Create the command
    // Assumes:
    //    tar program filename is fully qualified or in the path
    //    It is a UNIX style tar program with args in the right places
    string cmd = tarProg + " xf " + qTarPath + " " + tarArgs;

    // Run it with appropriate timeout
    runCommand(cmd, "Extracting files", 45);
}

/**
 * Put quote marks around a string.  Used for handling file names that
 * include spaces.
 *
 * Safe to call even if the string is already quoted.  Will not quote twice.
 *
 *  @param inString     Quote this string.
 *
 *  @return             "inString".
 */
string quoteIt(string inString) {
    size_t len = inString.size();
    if (inString[0] == '\"' && inString[len - 1] == '\"')
        return inString;
    return "\"" + inString + "\"";
}

/**
 * Command line programs need quotes to handle files with spaces.
 * WinAPI functions don't need or like the quotes.
 *
 * Safe to call even if the string has no quotes.
 *
 *  @param inString     Remove quotes from a string like "inString"
 *
 *  @return             inString without quotes.
 */
string deQuoteIt(string inString) {
    size_t len = inString.size();
    if (inString[0] == '\"' && inString[len - 1] == '\"')
        return inString.substr(1, len-2);
    return inString;
}

/**
 * Convert "\" to "\\" in order to avoid C style backslash escaping.
 *
 * Examples: "c:\abc\\xyz\" => "c:\\abc\\\\xyz\\"
 *
 *  @param inString     Quote this string.
 *
 *  @return             inString with slashes doubled.
 */
string doubleSlash(string inString) {

    // Build the new string here
    char *strBuf     = new char[inString.length() * 2];
    const char *pSrc = inString.c_str();
    char *pDest      = strBuf;

    // Copy with slash doubling
    while (*pSrc) {
        *pDest++ = *pSrc;
        if (*pSrc == '\\')
            *pDest++ = '\\';
        pSrc++;
    }
    *pDest = '\0';

    string outStr = string(strBuf);
    delete[] strBuf;

    return outStr;
}

/**
 * Return a lower cased version of a string.
 *
 *  @param mixed        String in whatever case.
 *
 *  @return             Lowercased version.
 */
string lowerStr(string& mixed) {

    size_t len = mixed.length();
    char  *buf = new char[len+1];

    const char  *src;
    char        *dest;

    // Copy and convert
    src  = mixed.c_str();
    dest = buf;
    for (int i=0; i<len; i++)
        *dest++ = tolower(*src++);
    *dest = '\0';

    string retStr(buf);
    delete[] buf;

    return retStr;
}
/**
 * Parse out the file names and descriptions from the PrintJob.
 *
 * This version assumes a file named "PrintJobFiles.txt" exists and has the
 * following format, one line per file
 *    filename|Optional description
 * Example:
 *    "267885.pdf|The role of androgen receptors in prostate cancer"
 *
 * The lines must be in print order.  A single line may appear multiple times
 * in the file listing if it is printed multiple times, e.g. for different
 * board memebers.
 *
 *  @param jobFilesName     Name of the file containing the file names, path
 *                           optional but probably always needed.
 *
 *  @return                 Vector of std::pair containing: name, description.
 *                           If no description, then "".  Pairs appear in the
 *                           same order in the vector as in the file.
 *
 * All errors are fatal.
 *
 *  !!! UNTESTED - CAN'T TEST UNTIL WE CAN UPDATE SERVER SOFTWARE !!!
 */
printVector getPrintList(const string& jobFilesName) {

    if (!fileExists(jobFilesName))
        fatal("Couldn't find required file: " + jobFilesName);

    printVector printList;

    const char *cpath = jobFilesName.c_str();
    ifstream prtFile(cpath);
    if (!prtFile.is_open()) {
        perror(cpath);
        fatal("Unable to open print list file for reading");
    }

    // For line and tokens from each line
    char lineBuf[bufsize];
    char *fileName;
    char *fileDesc;
    char *seps = "|";

    while (true) {
        prtFile.getline(lineBuf, bufsize - 1);
        if (prtFile.gcount() == 0)
            break;
        fileName = strtok(lineBuf, seps);
        fileDesc = strtok(NULL, seps);

        if (*fileName == '\n')
            // Blank line, shouldn't be there but, whatever
            continue;
        if (fileDesc == NULL || *fileDesc == '\n')
            // Description is optional
            fileDesc = "";

        // Chomp newline from value
        size_t len = strlen(fileDesc);
        if (len && *(fileDesc + len - 1) == '\n')
            *(fileDesc + len - 1) = 0;

        // Add them to the vector
        printList.push_back(printPair(fileName, fileDesc));
    }
    prtFile.close();

    return printList;
}

/**
 * Parse out the file names and descriptions from the PrintJobReport*.html.
 *
 * This is a kludge, written because of bureaucratic difficulties
 * implementing changes on the EBMS servers.
 *
 * Assumes a very particular report format that only uses html list items
 * for file names and descriptions.  Anything else will break it.
 *
 * The list items must be in print order.  A single file may appear
 * multiple times in the file listing if it is printed multiple times,
 * e.g. for different board memebers.
 *
 *  @param rptFileName  Fully qualified name of report file.
 *
 *  @return             Vector of std::pair containing: name, description.
 *                       If no description, then "".  Pairs appear in the
 *                       same order in the vector as in the file.
 *
 * All errors are fatal.
 */
printVector getReportList(const string& rptFileName) {

    // Check
    if (!fileExists(rptFileName))
        fatal("Report HTML file not found");

    // Open the file
    ifstream fin(deQuoteIt(rptFileName).c_str());
    if (!fin.is_open()) {
        perror(rptFileName.c_str());
        fatal("Could not open report file");
    }

    // And suck in all of the bytes
    stringstream filebuf;
    filebuf << fin.rdbuf();
    string      fileStr   = filebuf.str();
    const char *fileChars = fileStr.c_str();

    // Return data here
    printVector printList;

    // Variables to track all the parts of the list items
    const char *itemp = fileChars;
    size_t      itemLen;
    size_t      nameLen;
    const char *descp;
    size_t      descLen;
    const char *p;
    const char *endp;

    // Final destination of parts
    string fileName;
    string fileDesc;

    // Find all of the list items in the file
    while (getReportListItem(&itemp, &itemLen)) {

        // Split it into filename + description
        p    = itemp;
        endp = p + itemLen - 4;

        // There will always be an end tag.  getReportList() checked for it
        while (p < endp && *p != '<')
            ++p;

        // It's easier to write code to work around a bug than to
        // write a justification to CBIIT for fixing it.
        if (strncmp(p, "<br", 4) == 0  || strncmp(p, "<BR", 4) == 0 ||
            strncmp(p, "</br", 4) == 0 || strncmp(p, "</BR", 4) == 0) {

            // There is a filename and a description
            nameLen = p - itemp;

            // Find the description
            while (*p != '>')
                ++p;
            descp = p +1;
            while (*p != '<')
                p++;
            descLen = p - descp;
        }
        else {
            nameLen = p - itemp;
            descp   = "";
            descLen = 0;
        }

        // Copy the data into a string, dropping final punctuation
        if (itemp[nameLen-1] == ':')
            nameLen--;
        fileName = string(itemp, nameLen);
        fileDesc = string(descp, descLen);

        // Append them to the vector
        printList.push_back(printPair(fileName, fileDesc));
    }

    return printList;
}


/**
 * Find the next list item in the report HTML.  Subroutine of getReportList().
 *
 * This is a specialized dirty function that might easily fail in any
 * application beyond the specific intent of parsing the HTML report
 * sent down by EBMS.
 *
 *  @param positionp        (In/Out) Pointer to character position in a
 *                           C string of the entire report.
 *                           Upon return, it has been updated to point to
 *                           the next value of a list item.
 *
 *  @param itemLength       Pointer to place to put length of the next item.
 *                           May be zero.
 *                          After each call, the caller should add itemLength
 *                           + 1 to positionp before the next call.
 *
 *  @return                 True = There is another list item.
 *                          False = No more list items in report.
 */
bool getReportListItem(const char **positionp, size_t *itemLength) {

    const char *p         = *positionp;
    const char *endp      = p + strlen(p) - 3;
    const char *startItem = NULL;
    bool inListTag;

    // Find the first list item
    // Assumes "<li" or "<LI" is the start of a list item
    if ((p = strstr(*positionp, "<li")) == NULL)
        if ((p = strstr(*positionp, "<LI")) == NULL)
            return false;

    // We're pointing to a list start tag.
    // Find the end
    while (p < endp && *p != '>')
        ++p;

    if (*p != '>')
        return false;

    // We've got the start
    startItem = ++p;

    // Find the end tag
    if ((p = strstr(startItem, "</li>")) == NULL)
        if ((p = strstr(startItem, "</LI>")) == NULL)
            return false;

    // Found end, update caller's pointer and length
    *positionp  = startItem;
    *itemLength = p - startItem;

    return true;
}

/**
 * Execute (spawn) a command in a separate process.
 * Uses Windows API, code is based on a Microsoft example.
 *
 * Escape quote marks as needed to pass long strings to cmd.exe.
 * Examples:
 *   "pdfprint -license_key whatever -printer foo \"This document.pdf\""
 *
 *  @param cmd      Complete command line, including any args.
 *  @param msg      Tells user what we're doing.
 *  @param timeout  Give up if it takes more than this many seconds,
 *                   0=infinite.
 *
 * If the process fails, treats it as a fatal error.
 */
void runCommand(
    const string& cmd,
    const string& msg,
    const int     timeout
) {

    char buf[bufsize];   // For when c_str required

    /* Tell the user what we're doing
    cout << "Executing command: " << cmd << endl;
    if (msg.size() > 0)
        cout << msg << endl << endl;
    cout << endl;
    */

    // Log it
    if (msg.size() > 0)
        log("\n" + msg + ":\n    " + cmd);
    else
        log(cmd);

    // If in debug mode, that's all we do
    if (s_debug)
        return;

    // Not using these for now.  Change later if required.
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Need a C string for command
    if (cmd.size() > bufsize-1)
        fatal("Command exceeds maximum size");
    strcpy(buf, cmd.c_str());

    // Start the child process.
    if( !CreateProcess(
        NULL,           // No module name (use command line)
        buf,            // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        0,              // No creation flags
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    ) {
        sprintf(buf, "runCommand failed, error number=%d", GetLastError());
        fatal(buf);
    }

    // Wait until child process exits or timeout expired
    DWORD millisecs;
    if (timeout == 0)
        millisecs = INFINITE;
    else
        millisecs = timeout * 1000;

    DWORD rc = WaitForSingleObject(pi.hProcess, millisecs);
    if (rc == WAIT_TIMEOUT) {
        sprintf(buf, "Timeout after %d seconds.  Command=\n%s",
                timeout, cmd);
        fatal(buf);
    }

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
}

/**
 * Create a command line suitable for printing a PDF file.  Uses the
 * configured software for printing PDFs.
 *
 *  @param cfg          Reference to configuration values.
 *  @param filename     Fully qualified path to file to print.
 *
 *  @return             The command, suitable for runCommand().
 */
string makePdfPrintCmd(Config& cfg, const string& fileName) {

    // Name the file in the data directory
    string fullPath = cfg.getParmValue("dataDir") + fileName;

    // XXX CHECK pdfprint license
    string cmd = cfg.getConfigValue("def_pdfprinter", true)
               + " " + cfg.getConfigValue("def_pdfprintargs", false)
               + " " + cfg.getParmValue("printer", true)
               + " " + quoteIt(fullPath);
    return cmd;
}

/**
 * Create a command line suitable for printing a word processing file.
 * Handles .doc, .docx, .rtf, .odt, using the configured software for
 * printing such documents.
 *
 *  @param cfg          Reference to configuration values.
 *  @param filename     Fully qualified path to file to print.
 *
 *  @return             The command, suitable for runCommand().
 */
string makeDocPrintCmd(Config& cfg, const string& fileName) {

    // Name the file in the data directory
    string fullPath = cfg.getParmValue("dataDir") + fileName;

    // Command line
    string cmd = cfg.getConfigValue("def_docprinter", true)
               + " " + cfg.getConfigValue("def_docprintargs", false)
               + " " + cfg.getParmValue("printer", true)
               + " " + quoteIt(fullPath);
    return cmd;
}

/**
 * Create a command line for displaying the print job report in a browser
 * window.
 *  @param cfg          Reference to configuration values.
 *  @param filename     Fully qualified path to HTML file to display.
 *
 *  @return             The command, suitable for runCommand().
 */
string makeReportCmd(Config& cfg, const string& fileName) {

    string cmd = cfg.getConfigValue("def_browser", true) + " "
                 + quoteIt(fileName);

    return cmd;
}

int main(int argc, char *argv[]) {

    // Holds a command string to be spawned with runCommand()
    string cmd;

    // Initialize defaults
    Config cfg = Config();

    // Add in command line
    cfg.setParms(argc, argv);

    // If any defaults were changed, set them and exit
    if (cfg.isConfigChanged()) {
        cfg.store();
        cout << "Config file modified.  Old file is now \"" << configFileBak
             << "\"" << endl;
        cout << "No documents printed" << endl;
        exit(0);
    }

    // Untar the tarfile in place
    untar(cfg);

    // Test that required file is where we expect it
    string reportFile = cfg.getParmValue("reportFile");
    if (!fileExists(reportFile))
        fatal("Can't find print job report in expected place \""
              + reportFile + "\"");

    // If the report is the only thing requested
    if (cfg.getParmValue("report") == "report") {
        cmd = makeReportCmd(cfg, reportFile);
        runCommand(cmd, "", 0);
        cout << "Exiting program" << endl;
        exit(1);
    }

    // Get the list of documents to print
    // printVector getPrintList(...)
    printVector printVec = getReportList(cfg.getParmValue("reportFile", true));
    if (printVec.size() < 1)
        fatal("Nothing to print in this print job");

    // What docs do we print?
    int firstDoc = atoi(cfg.getParmValue("start").c_str());
    int lastDoc  = atoi(cfg.getParmValue("end").c_str());
    if (firstDoc < 1)
        firstDoc = 1;
    if (lastDoc > printVec.size())
        lastDoc = printVec.size();

    // Report
    stringstream info;
    info << "Got " << printVec.size() << " documents"
         << ": Printing docs " << firstDoc << " - " << lastDoc;
    log(info.str());

    // If we got here, we're ready to print
    char bitbucket[bufsize];
    char extbuf  [bufsize];
    for (int i=firstDoc; i<=lastDoc; i++) {

        // Adjust origin 1 for humans to origin 0 for C++ vector
        printPair fPair    = printVec[i-1];
        string fileName    = fPair.first;
        string description = fPair.second;

        // Get the file extent of the filename we're going to print
        _splitpath(fileName.c_str(), bitbucket, bitbucket, bitbucket, extbuf);

        // Construct a print command suitable for that extension
        if (strcmp(extbuf, ".pdf") == 0)
            cmd = makePdfPrintCmd(cfg, fileName);
        else if ((strcmp(extbuf, ".doc") == 0) ||
                 (strcmp(extbuf, ".docx") == 0) ||
                 (strcmp(extbuf, ".rtf") == 0) ||
                 (strcmp(extbuf, ".odt") == 0))
            cmd = makeDocPrintCmd(cfg, fileName);

        else {
            char numbuf[20];
            sprintf(numbuf, "%d", i);
            fatal("File \"" + fileName
                  + "\" has unknown extension.\n"
                  + "Please tell programming staff.\n"
                  + "Printing stopped on document number " + numbuf);
        }

        // Execute the command.  Not sure what the timeout should be
        char numbuf[20];
        sprintf(numbuf, "%3d: ", i);
        runCommand(cmd, numbuf + description, 180);
    }

    endLog();

    return 0;
}
