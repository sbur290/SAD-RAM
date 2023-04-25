/* =============================================================================
     File: C3_errors.cpp. Version 7.0.0.1. Nov 08, 2012.

     This module has a comprehensive list of all error codes.
     The program BuildErrors will generate the C3_Errors.h from this file.

     Copyright Sadram, Inc., 2022. All rights reserved.

============================================================================= */

#ifndef _C3_ERRORS_CPP_INCLUDED_
#define _C3_ERRORS_CPP_INCLUDED_
#pragma warning(disable:4706) //assignment within an if ()
#include <stdlib.h>
#include <stdio.h>
#include <C3_always.h>
#include <C3_string.h>
#include <C3_errors.h>

#ifndef WIN32 //ie Linux
  #include <sys/param.h>
  #undef TCHAR
  #define TCHAR char
  #include <errno.h>
  #include <sam_linux_shim.h>
#endif

typedef struct {int erC, severity; const char *errTextP, *parameterP, *fullErrorP;} C3_ERROR_TABLE;
static C3_ERROR_TABLE ErrorTable[] = 
{
{ERR_0001, XSEVERITY_ERROR, "Unknown error", "",
  "This is a catchall error that can result from a variety of conditions."
},

{ERR_0003, XSEVERITY_ERROR, "File or directory not found (%s)", "file name",
  "The file specified could not be found. Refer to the context for additional information such "
  "as which component is trying to access the file. Verify that the file exists and is not marked "
  "read only in a context that would expect read-write."
},

//Following error must be in table location[2] !)
{ERR_0002, XSEVERITY_ERROR, "Unknown error code", "",
  "This error will occur if information is requested on an error that is not in the error table."
},

{ERR_0005, XSEVERITY_ERROR, "Not enough memory (%s)", "requestedAmount",
  "An attempt to allocate memory has failed. This could be caused by an attempt to allocate memory "
  "in which a negative size parameter is treated as a very large number. The error may also occur "
  "if an attempt has been made to store more information in a fixed length internal structure "
  "than was declared."
},

{ERR_0006, XSEVERITY_CATASTROPHIC, "Memory allocation error", "",
  "An attempt to allocate memory has failed. This could be caused by an attempt to allocate memory "
  "in which a negative size parameter is treated as a very large number. The error may also occur "
  "if an attempt has been made to store more information in a fixed length internal structure "
  "than was declared."
},

{ERR_0997, XSEVERITY_ERROR, "Feature not supported in this release %s.", "name of feature",
  "The feature you have requested is not supported in this release of the code."
},

{ERR_0998, XSEVERITY_ERROR, "Error in external module (%s).", "module name",
  "An error has occurred accessing the specified external binary module. This is a software malfunction "
  "which you should report to Pico Computing."
},

{ERR_1000, XSEVERITY_ERROR, "Error Number %s specified twice.", "error number",
  ""
},

{ERR_1001, XSEVERITY_ERROR, "Missing error 'number=' or 'summary=' field.", "",
  "The program build_pico_errors_h encountered an error processing the file pico_errors.cpp."
},

{ERR_1002, XSEVERITY_ERROR, "Invalid number (%s).", "number",
  "Multiple decimal points or other syntax errors in a number"
},

{ERR_1003, XSEVERITY_ERROR, "Unbalanced quotes in a string: %s", "string",
  "A string is delimitted by an opening and closing quote. This error occurs if the closing quote was not found.\n"
  "To include a quote character in the string prefix it with \\. For example \"abc=\\\"xyz\\\" perhaps\""
},

{ERR_1004, XSEVERITY_ERROR, "Unable to find --> or < or > found in comment.", "",
  ""
},

{ERR_1005, XSEVERITY_ERROR, "Invalid .bit file format. Missing sync flag.", "",
  "The .bit file is missing the necessary sync flag."
},

{ERR_1006, XSEVERITY_ERROR, "Flash data verification failed.", "",
  "Verification indicated that the data on the flash was incorrect."
},

{ERR_1010, XSEVERITY_ERROR, "Can't detect flash chip for loading the FPGA.", "",
  "The CFI info read from the flash chip was invalid, so we can't verify that we're talking to the chip. This error can occur if the PCIe"
  " was left in a bad state by a previous operation."
},

{ERR_1020, XSEVERITY_ERROR, "Can't set FPGA load address.", "",
  "Failed to set the address for the FPGA to load the new .bit file from. This is often the result of PCIe errors."
},

{ERR_1021, XSEVERITY_ERROR, "Card hotswapping failed.", "",
  "When the FPGA is reloaded, it goes down and then comes back up with the new .bit file."
  " The OS needs to detect this and perform a hotswap cycle in order to connect to the new .bit file."
  " This usually has one of two causes: 1) The OS is not configured for hotswapping. Please see the hotswap section of the 'Getting Started Guide.'"
  " 2) The new .bit file is bad, and the FPGA didn't come back up and connect to PCIe properly."
},

{ERR_1029, XSEVERITY_ERROR, "Fan out exceeded.", "",
 "The number of variables controlled by the specified element cannot be larger than 16"
},

{ERR_1030, XSEVERITY_ERROR, "Unbalanced /* versus */ in a comment, begging with: %s", "startOfText",
  "The closing pair */ was not found in a comment beginning with a /* in the spcified line."
},

{ERR_1031, XSEVERITY_WARNING, "Unknown #pragma directive: '%s'", "",
  "The commands found in the #pragma are unknown. This warning may be because the software "
  "you are using is from another implementation of this compiler."
},

{ERR_1032, XSEVERITY_ERROR, "Duplicate parameter name '%s'", "parameter name",
  "A parameter in a wizard command file has duplicate parameters specified. This error will also "
  "occur if the variables conflict with the preset directives (see error 1039)."
},

{ERR_1033, XSEVERITY_ERROR, "Syntax error in #directive .... ", "",
  "This error can occur for a number of reasons. For example:\n\n"
  "  - The items in the #if(expression) do not form a valid expression."
},

{ERR_1034, XSEVERITY_ERROR, "Unknown #directive '%s'", "directiveName",
  "The preprocessor did not understand the specified directive. Expected directived are:\n"
  "    #set(...), #reset(...), #progma(...),\n"
  "    #if(...), #ifdef(...), #ifndef(...), #else,\n"
  "    #endif, #error(...) or #define(...) or #undef"
},

{ERR_1035, XSEVERITY_ERROR, "Unable to find #endif corresponding to this #if %s.", "#directive",
  ""
},

{ERR_1036, XSEVERITY_ERROR, "#if ... , #set ... , #reset ...  directive inside #if  ....#endif (%s).", "#directive",
  ""
},

{ERR_1037, XSEVERITY_ERROR, "#endif, #else, #elif, #endfor, or #endef found without matching opening directive ...  (%s).", "#directive",
  "#endif, #else, #elif, should be matched by an earlier #if directive\n"
  "#endfor               should be matched by an earlier #for directive\n"
  "#endef                should be matched by an earlier #define macName(param1, param2,...)= directive. The )= is the signal for beginning of a block #define."
},

{ERR_1038, XSEVERITY_ERROR, "Variable in #if(%s) is not found in a #define directive or formal parameter list.", "#directive",
  ""
},

{ERR_1039, XSEVERITY_ERROR, "Unknown #directive name (%s).", "#directive",
  "The Wizard encountered an unacceptable name in a $directive in the specified source file. $directives "
  "are of the form $(variable) or $variable. For example, $(projectname). projectname is a preset "
  "variable (see below). You can also create your own variables using $set variable=expression. "
  "One possible cause of this error is the underlying text file might require a $(variable) directive; "
  "msdev uses this methodology. In this case use $$ to replace, eg, $(CFG) becomes $$(CFG). The "
  "preset $directives are:\n\n"
  "    $$                       Replaced with $.\n"
  "    $(tab)                   Replaced with \\t\n"
  "    $(projectname)           Replaced with name of the project from\n"
  "                                    \'Name of New Project\' box.\n"
  "    $(projectnameUC)         Replaced with name of the project in upper case.\n"
  "    $(picoUtil_exe)          Replaced with directory from which PicoUtil.exe was executed.\n"
  "    $(ide)                   Replaced with name of IDE (currently == \'msdev6\'.\n"
  "    $(displaying_readme)     Replaced with true when processing this file for display, false otherwise.\n"
  "    $(currentVersionDotted)  Replaced with version number in ascii #.#.#.#.\n"
  "    $(currentVersionComma)   Replaced with version number in ascii #, #, #, #.\n"
  "    $(currentVersionHex)     Replaced with version number in hex 0x########.\n"
  "    $(currentDate)           Replaced with current date of PicoUtil.exe in the format MMM DD YYYY.\n"
  "    $(linux)                 Replaced with 1 if running under linux, 0 otherwise.\n"
  "    $(fileseparator)         Replaced with \\\\ when running under windows,\n"
  "                                      or   // when running under Linux.\n"
},

{ERR_1040, XSEVERITY_ERROR, "File has already been included (%s)", "file name",
  "A file should not be #included twice. If you absolutely must do this, pervert the file name "
  "to defeat the duplicate file name check. (The verification of the file name is deliberately "
  "sloppy to allow this error to be sidestepped). For example,\n"
  "    \'#include fileName.hlx\' and \'#include .\\filename.hlx\' will defeat this test. You may "
  "also copy the file and #include the copied file."
},

{ERR_1045, XSEVERITY_ERROR, "Invalid attribute in %s tag", "XML tag",
  "An invalid attribute was found in an XML tag. Valid attributes are:\n\n"
  "   number\n\n"
  "   summary\n\n"
  "   severity\n\n"
  "   parameter\n"
},

{ERR_1046, XSEVERITY_ERROR, "< found without a corresponding > in an XML node", "",
  "A < was found but there was no corresponding > to close the XML node."
},

{ERR_1047, XSEVERITY_ERROR, "> found without a corresponding <", "",
  "A > was found but there was no corresponding > to open the XML node."
},

{ERR_1048, XSEVERITY_ERROR, "<!-- found without --> in an XML comment", "",
  "The text <!-- was found without the corresponding --> in an XML comment."
},

{ERR_1049, XSEVERITY_ERROR, "Incomplete parameter list for a printf type statement", "",
  "The incorrect number of parameters to a printf line statement was found."
},

{ERR_1095, XSEVERITY_ERROR, "Invalid characters in define macro name: '%s'", "actual name",
  "Acceptable characters are any alphanumeric character plus underscore."
},

{ERR_1096, XSEVERITY_ERROR, "Incomplete parameter list for define ... %s", "",
  "Incomplete parameter list for define."
},

{ERR_1097, XSEVERITY_ERROR, "Invalid option in #pragma directive '%s'", "directive Name",
"The #pragma directive was invalid:\n"
"Proper syntax is #pragma followed by one or more of the following settings:\n"
"   printProgram     immediately display all lines in program at the current state of macro expansion.\n"
"   debug            debug           decorative only.\n"
"   printDefines                     immediate display of all defines.\n"
"   printMacros                      (spelling variant of printDefines).\n"
"   macDepth         =number         set acceptable macro recursion. Typical value would be 5 thru 10.\n"
"   nowarning        =warningNum     disable specified warning.\n"
"   pop              pop  variable\n"
"   push             push variable\n"
"                         variable = macDepth or allOptions (all the following switches):\n"
"   warningsAreErrors[=on|off]       treat warnings as errors.\n"
"   macro            [=on|off]       set macro debugging.\n"
"   compiler         [=on|off]       set high level compile debugging.\n"
"   compile                          (spelling variant of compiler)\n"
"   opcodes          [=on|off]       display each opcode as generated.\n"
"   optimizer        [=on|off]       display optimizer machinations\n"
"   optimize                         (spelling variant of optimizer)\n"
"   tokens           [=on|off]       display each token as parser proceeds.\n"
"   constantEval     [=on|off]       display detail steps in constant evaluation.\n"
"   asmContext       [=on|off]       macros defined herafter are only applicable inside asm{...}\n"
 },

{ERR_1100, XSEVERITY_ERROR, "Unable to find card with access to EX500 board.", "",
  "When writing to the EEprom on an EX500 board the program SetupM500 was unable to "
  "locate an M50x board. An M50x board is required to gain access to the EX500 board.\n"
  "You might try changing the position of the M50x board on the EX500 backplane."
},

{ERR_1101, XSEVERITY_ERROR, "Unacceptable input parameters:%s", "",
 ""
},

{ERR_1102, XSEVERITY_ERROR, "EE did not stabilize", "",
 "Data from the EE rom did not stabilize in the routine Read1EEadr().\n"
 "This is a software error. Please report this to Pico Computing."
},

{ERR_1103, XSEVERITY_ERROR, "Signature on EE rom is invalid", "",
 "EE rom attached to PLX switch does not have the proper signature (0x5A) or "
 "the number of words specified in location zero of the EE rom is too large.\n"
 "The first 16bit word of the EE rom should have the length in 16bit words) and "
 "the signature (0x5A). This error will normally be ignored when you are writing "
 "information to the EE rom since it will be re-generated in this case."
},

{ERR_1104, XSEVERITY_ERROR, "EE rom did not return proper status", "",
 "The EE rom did not return a status indicating that the last operation completed successfully. "
 "This probably signifies that the software did not wait long enough for the EE rom to finish.\n"
 "Please report this error to Pico Computing."
},

{ERR_1106, XSEVERITY_WARNING, "EE rom is unitialized", "",
 "EE rom attached to PLX switch does not have the proper signature (0x5A) or "
 "the number of words specified in location zero of the EE rom is too large.\n"
 "The first 16bit word of the EE rom should have the length in 16bit words) and "
 "the signature (0x5A). This error will normally be ignored when you are writing "
 "information to the EE rom since it will be re-generated in this case."
},

{ERR_1159, XSEVERITY_ERROR, "Syntax error processing %s.", "file name",
  ""
},

{ERR_1379, XSEVERITY_ERROR, "Invalid '&' directive in XML.", "",
  "The & character must be followed by up one of the following reserved words or up to three digits. "
  "In both cases it should be terminated with a semicolon. The following are examples of valid "
  "&directives:\n"
  "            &amp;   representing & \n\n"
  "            &lt;    representing <  \n\n"
  "            &gt;    representing >  \n\n"
  "            &apos;  representing \'\n\n"
  "            &quot;  representing \"\n\n"
  "            &#92;   representing \\ ascii character 92"
},

{ERR_1380, XSEVERITY_ERROR, "Unacceptable child element (%s) in XML group.", "XML tag",
  ""
},

{ERR_1404, XSEVERITY_ERROR, "single quoted string must be one character long.", "",
  "For example: 'ABC' will generate this error."
},

{ERR_1405, XSEVERITY_ERROR, "Missing or improper label.", "name",
  "The specified label could not be found."
},

{ERR_1406, XSEVERITY_ERROR, "Missing close quote in string constant.", "",
  "Missing close quote in a string. Strings are delimited with double quote or single quote and "
  "may be broken into substrings for convenience. For example:\n\n"
  "    \"abc\"\n\n"
  "    \'abc\'\n\n"
  "    \"a\" \"b\" \"c\"\n\n"
  "    \"a\" \'b\' \"c\""
},

{ERR_1407, XSEVERITY_ERROR, "Missing semicolon in command '%s'.", "string",
  "A command in a file WizardCommands.txt or <filename>.tests should be terminated with a semicolon."
},

{ERR_1408, XSEVERITY_ERROR, "Invalid option to Copy command in WizardCommands.txt '%s'.", "string",
  "A COPY or XCOPY command in a WizardCommands.txt file may have the following options:\n\n"
  "   noExpand - meaning the file should be copied verbatim and not expanded."
},

{ERR_1498, XSEVERITY_ERROR, "Error in call to Pico Computing Server %s.", "serverName",
  "A COM error was returned by a call to Pico Computing Pico Computing. This error occurs when "
  "a client to Pico Computing cannot access the specified remote machine. The format of the text "
  "will usually be:\n\n"
  "    \\\\machineName\\Pico ComputingName\n\n"
  " - The machineName is the name of a computer on your network, and should be visible in Network\n"
  "   Neighborhood or any of the standard network navigation tools available to Windows\' users.\n "
  " - The Pico ComputingName is an arbitrary name such as JMMTIDS or MONITOR which is the default name\n"
  "   for the program you are running or the name chosen when Pico Computing was installed.\n\n"
  "The most likely cause of this problem is that \\\\machineName is invalid or cannot be accessed "
  "on your network because of network errors."
},

{ERR_1500, XSEVERITY_ERROR, "Error in hostname or portname to Pico_ipXface.", "",
  ""
},

{ERR_1501, XSEVERITY_ERROR, "Incorrect package sequence in TFTP message.", "",
  "The sequence number of the TFTP packet is incorrect. The TFTP transfer will attempt to correct the error."
},

{ERR_1502, XSEVERITY_ERROR, "TFTP server has returned an error packet.", "",
  "During a TFTP download of a file, the server (MRC-200) returned an error. The "
  "download was aborted."
},

{ERR_1503, XSEVERITY_ERROR, "TFTP Error unknown", "",
  "An error of unknown origin has occurred in a TFTP transaction. The file download is aborted."
},

{ERR_1504, XSEVERITY_ERROR, "TFTP Error file not found", "",
  "The requested file was not found in a TFTP transaction. The file download is aborted."
},

{ERR_1505, XSEVERITY_ERROR, "TFTP Error access", "",
  "Access was denied to the specified file in a TFTP transaction. The file download is aborted."
},

{ERR_1506, XSEVERITY_ERROR, "TFTP Error target device full", "",
  "Target storage device is full in a TFTP transaction. The file download is aborted."
},

{ERR_1507, XSEVERITY_ERROR, "TFTP Error illegal op", "",
  "An illegal opcode was found in a TFTP packet. The file download is aborted."
},

{ERR_1508, XSEVERITY_ERROR, "TFTP Error illegal port number", "",
  "The port number of the TFTP packet is incorrect. The file download is aborted."
},

{ERR_1509, XSEVERITY_ERROR, "File '%s' already exists.", "",  //NOTE: used in non-MRC
  "The target file already exists."
},

{ERR_1510, XSEVERITY_ERROR, "Unable to obtain buffer to send Ethernet message.", "",
  "The lowest level Ethernet driver was unable to obtain a buffer required to transmit "
  "a packet. This error probably signals a hardware error in the Ethernet subsystem. "
  "For further diagnosis of this problem:\n"
  "1. Verify that the Ethernet cable is in place and is a cross over cable.\n"
  "2. Use an Ethernet sniffer to determine whether the MRC-200 is transmitting packets "
  " onto the network. (This error indicates that it is not.)\n"
  "3. Replace the MRC-200 charger board."
},

{ERR_1526, XSEVERITY_ERROR, "Incorrect syntax of field name '%s'", "offending string", //NOTE: used in non-MRC
  "The field names for a flash ROM file must be in the format variableName=value. For example:\n\n"
  "   sourceFile=c:\\dir\\file\n"
  "No spaces are allowed between the variable name and the = sign, "
  "and the variableName must comprise letters, digits and the underscore."
},

{ERR_1530, XSEVERITY_ERROR, "No Response from MRC-200.", "",
  "No message has been received from the MRC-200 within the last 30 seconds.\n"
  "Possible causes of this problem include:\n"
  "  1. The UDP connection between the MRC-200 and the PC host has failed.\n"
  "  2. The MRC-200 program has 'lost' the UDP connection.\n"
  "  3. The MRC-200 hardware has failed.\n"
  "  4. The cable between the MRC-200 and the host PC is disconnected.\n"
  "Diagnostic techniques you might consider include:\n"
  " 1. Exit MRC-200.exe and restart\n"
  " 2. Power off / on the MRC-200\n"
  " 3. Ping the MRC-200: from the command prompt (cmd.exe) enter 'ping 10.0.0.69'\n"
  "     'ping' sends a message to the MRC-200 board and receive an acknowledging reply.\n"
  "   NOTE: The IP address of the MRC-200 can be changed in the IPsetup menu.\n"
  "       The default IP address of the MRC-200 is 10.0.0.69;\n"
  "       the default address for the PC host is 10.0.0.100;\n"
  "       the default gateway address is 10.0.0.1\n"
  "       the default IP mask is 255.255.255.0.\n"
  " 4. Check that the MRC-200 is configured for IP. When you restart the MRC-200 the LCD should display\n"
  "      'Ethernet startup / Arp 10.0.0.100'\n"
  "       this indicates that the MRC-200 is trying to establish the (Ethernet) address of the PC host.\n"
  "       When this is successful the LCD screen will be full of IP & Ethernet addresses.\n"
  "   The MRC-200 can be configured for IP operation in the Setup IP menu.\n"
},

{ERR_1531, XSEVERITY_ERROR, "No Response to ARP generated by MRC-200 charger board", "",
  "An attempt by the MRC-200 to ARP (Address Resolution Protocol) a PC-host at address 10.0.0.100 has failed.\n"
  "ARP is an IP technique used to establish the Ethernet address from the IP address.\n"
  "The MRC-200 charger board will operate in 'standalone mode'.\n"
  "Possible causes of this problem include:\n"
  "  1. The cable between the MRC-200 and the host PC is disconnected.\n"
  "  2. There is no PC-host with an IP address of 10.0.0.100 on the network to which the MRC-200 charger "
  "is attached\n"
  "Other diagnostic techniques you might consider:\n"
  " IpConfig (on the PC-host). This will report the IP address and other parameters of interest.\n"
  "NOTE: The IP address of the PC-host can be changed in the IPsetup menu from the default value of 10.0.0.100.\n"
},

{ERR_1544, XSEVERITY_ERROR, "Estimated file size exceeded.", "",
  "During the transfer of a file using the Xmodem protocol, the allocated space for a file is "
  "not sufficient for the file being written. The Xmodem protocol will run to completion, "
  "however, the file will not be properly written to the flashROM. \n"
  "Verify that the file you are writing is a reasonable size. If necessary add "
  "the actual file size following the file name, but before other fields of the file, eg.\n"
  "    get filename,size=132000,Environment=Linux.cfg\n"
  "or  get filename,size=132000"
},

{ERR_1545, XSEVERITY_ERROR, "Data not properly written to SecSci.", "",
  "Data being written to SecSci (the permanent part of the flash ROM) did not readback correctly."
},

{ERR_1546, XSEVERITY_ERROR, "Data for SecSci is not properly formatted.", "",
  "Data being written to SecSci (the permanent part of the flash ROM) is not properly formatted."
},

{ERR_1547, XSEVERITY_ERROR, "%s Ip addresses are inconsistent.", "",
  "The specified IP addresses are inconsistent.\n"
  "The MRC IP address, Host IP address, and gateway IP address must all reside on the same 'network'.\n"
  "This means that all IP address masked with the IP address mask must be the same.\n"
},

{ERR_1548, XSEVERITY_ERROR, "Error creating MRC-200.cfg file.", "",
  "An error has occurred creating a new configuration file MRC-200.cfg.\n"
  "The system will erase MRC-200.cfg and re-create it with the default parameter set.\n"
  "As a consequence, voltage thresholds, IP addresses must be re-entered."
},

{ERR_1549, XSEVERITY_ERROR, "Error Processing MRC-200.cfg file.", "",
  "An error has occurred reading or processing the configuration file MRC-200.cfg.\n"
  "The system will erase MRC-200.cfg and re-create in with the default parameter set.\n"
  "As a consequence, voltage thresholds, IP addresses must be re-entered."
},

{ERR_1550, XSEVERITY_ERROR, "Error accessing flash ROM.", "",
  "An error has occurred reading from or writing to the flash ROM on the target board."
},

{ERR_1551, XSEVERITY_ERROR, "Error interfacing with flash ROM on MRC-200.", "",
  "A basic hardware error has occurred accessing the flash ROM on the MRC-200.\n"
  "The flash ROM interface did not return a proper status for a read/write 16bit word."
},

{ERR_1552, XSEVERITY_ERROR, "Protocol error accessing MRC-200.", "",
  "Protocol error accessing MRC-200."
},

{ERR_1553, XSEVERITY_ERROR, "Timeout transferring data to remote card.", "",
  "The upload / download of a file failed because the device did not respond in a timely fashion."
},

{ERR_1554, XSEVERITY_ERROR, "Module not found.", "",
  "The required overlay module was not found on the flash ROM."
},

{ERR_1555, XSEVERITY_ERROR, "Unable to obtain allocation information for flash ROM.", "",
  "Unable to obtain allocation information for flash ROM."
},

{ERR_1556, XSEVERITY_ERROR, "Sector was not erased.", "",
  "The designated sector was not erased correctly."
},

{ERR_1557, XSEVERITY_ERROR, "Error capturing data from MRC-200.", "",
  "No reply form MRC-200 after Error capturing data from MRC-200."
},

{ERR_1558, XSEVERITY_ERROR, "Error writing permanent information '%s'.", "serial number found",
  "An error occurred writing the serial number or other permanent information to the BCCBS board\n"
  "This may be because:\n"
  "1. there is already information in the permanent sector which cannot be "
  "overwritten (eg the board already has a serial number).\n"
  "2. The amount of information exceeds the capacity of the permanent sector."
},

{ERR_1559, XSEVERITY_ERROR, "Multiple occurrences of file '%s'.", "file name",
  "The specified file occurs multiple times in the flash ROM."
},

{ERR_1560, XSEVERITY_ERROR, "Incorrect password for flash access.", "",
  "The password entered for flash access is not correct. Passwords are not case sensitive.\n"
  "If you know what you are doing and absolutely must write a file to the flash ROM, delete\n"
  "the MRC-200 history file and restart MRC-200.exe.\n"
  "Be warned - here there are dragons."
},

{ERR_1561, XSEVERITY_WARNING, "Remember to change Maintenance Mode switch to '%s' before rebooting.", "inside or outside position",
  "The switch on the MRC-200 charger board controls how the system boots up:\n"
  " Switch to the outside position (towards outer edge of board):\n"
  "   System will boot from MaintenanceBoot.bit\n"
  " Switch to the inside position (towards middle of board):\n"
  "   System will boot from PrimaryBoot.bit\n"
},

{ERR_1562, XSEVERITY_ERROR, "File download module not present in bitfile.", "",
  "The bit file you are currently using does not have the capability to download a new file.\n"
  "Change the maintenance mode switch to the outside position and reboot the system"
},

{ERR_1563, XSEVERITY_ERROR, "Transfer incomplete.", "",
"Not all the records were delivered during Xmodem update of flash ROM"
},

{ERR_1564, XSEVERITY_ERROR, "File Checksum error.", "",
"File checksum does not match value provided by source"
},

{ERR_1565, XSEVERITY_ERROR, "Header field %s has invalid format.", "<field name>",
"The header field of a file specified in the get command of monitor must have the format:\n"
"keyword=value,keyword=value,...\n"
"for example:\n"
"  get PrimaryBoot.bit,LinkedFile=zimage.elf\n"
"  get zimage.elf,Environment=lltemac.cfg\n"
"The header field should not contain spurious spaces."
},

{ERR_1566, XSEVERITY_ERROR, "Buffer is too small to perform requested operation.", "",
"A buffer in one software component is too small for the amount of data requested by another component.\n"
},

{ERR_1567, XSEVERITY_ERROR, "Error updating variable %s.", "",
"MRC-200 encountered an error when attempting to update the specified variable\n"
"or the value returned from the read operation was not the value written."
},

{ERR_1568, XSEVERITY_ERROR, "Timeout waiting for sent data on TCP/IP socket", "",
"In the TEMAC on the Pico Card, a request to send data was not completed in a timely fashion."
},

{ERR_1569, XSEVERITY_ERROR, "AwaitSendPacket cannot be called from inside an interrupt.", "",
"AwaitSendPacket relies upon the interrupts to complete a transmit packet. It cannot therefore"
"be called from within an interrupt."
},

//FAULT_FLASH=0,
{ERR_1570, XSEVERITY_ERROR, "Error accessing flash", "",
 "The initial self-test found an error access the flashROM on the MRC-200 charger board."
 "This error causes the MRC-=200 to halt with all the LED's flashing.\n"
 "NOTE: You can suppress the red flashing light for any error. Proceed as follows:\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
 },

//FAULT_PA_OVERTEMP,
{ERR_1571, XSEVERITY_ERROR, "PA signal temperature greater than PaOverTemp", "",
 "The Power amplifier has reported an internal temperature than is greater than acceptable."
 "This limitation is designed to prevent the power amplifier from burning up."
 "However, this value can be changed as follows:\n"
 "1. Press the menu button until the LCD displays Power Amplifier.\n"
 "2. Press select.\n"
 "3. Choose the field you wish to modify.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
 },

//FAULT_VSWR,
{ERR_1572, XSEVERITY_ERROR, "VSWR is too large", "",
 "The ratio between the reverse power and forward power is too high. This indicates that the R/F energy "
 "you are generating is not propagating out into space but is reflected back. The usual reason for this "
 "is the antenna is the wrong type for the frequency you are operating at, or the antenna is "
 "not connected properly.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
 },
//FAULT_NO_PWR,
{ERR_1573, XSEVERITY_ERROR, "Radio's drawing current, P/A is not", "",
"This error occurs when the radios are drawing eno9g power to indicate they are transmitting "
"but the power amplifier is not drawing a commensurate amount of power.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},
//FAULT_I2C,
{ERR_1574, XSEVERITY_ERROR, "An I2C device did not respond properly", "",
"This error occurs if a device on the MRC-200 charger board, or in the smart battery is not "
"responding as expected. In the case of a smart battery this is usually interpreted to mean that "
"the battery is a 'dumb battery', ie a 2590 battery.\n"
"You must power down the MRC-200 and startup again to clear this error.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},
//FAULT_NO_PA_COMS,
{ERR_1575, XSEVERITY_ERROR, "no information returned from PA", "",
"No information was returned from the power amplified when an 'STS' was sent. This may be because "
"the power amplified is turned off, or because an internal cable to the P/A has become disconnected.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_LOW_INTERNAL_BAT,
{ERR_1576, XSEVERITY_ERROR, "internal batteries are too low", "",
"This error condition indicates that all the internal batteries are operating at less than 5% of capacity."
"In a very short time the internal batteries will become exhausted and the MRC-200 will shut down."
"To correct this problem:"
"1. Plug the MRC-200 into an A/C source, \n"
"2. Start the engine on the vehicle to recharge the internal batteries.\n"
"3. Replace the batteries with fresh batteries.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_MISSING_BAT,
{ERR_1577, XSEVERITY_ERROR, "missing internal battery", "",
"One or more internal batteries are missing. To correct this situation:\n"
"1. Plug in a battery in the missing location.\n"
"2. If the battery is inserted, wiggle the battery to ensure that it is making good contact "
"with the charging circuitry.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_BIT,
{ERR_1578, XSEVERITY_ERROR, "BIT failure (multiple sources)", "",
"This error occurs if the BIT (Built in Test) fails. There may be many reasons for BIT to fail:\n"
"1. The Chargers failed to respond to smBus commands.\n"
"2. There is an error in the MRC-200.cfg file.\n"
"3. One or more files on the flash have a checksum error.\n\n"
"In any case you may wish to run the bit test again to discriminate the sources of errors.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_CONFIG_FILE,
{ERR_1579, XSEVERITY_ERROR, "MRC-200.cfg has errors.", "",
"There are one or more errors in the MRC-200.cfg file. This file contains operational parameters such as:\n"
"1. the vehicle voltage thresholds at which the system shutdown operation form the vehicle battery.\n"
"2. the IP address of the MRC-200 and other IP control information.\n\n"
"Normally the MRC-200.cfg file will be recreated automatically using the default values. "
"You may wish to review these settings:\n"
"1. Press the menu key until you get to Setup.\n"
"2. Press the select key and choose the submenu that you wish to change.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_PA_STUCK,
{ERR_1580, XSEVERITY_ERROR, "P/A is drawing large current (stuck in transmit)", "",
"This error occurs if the power amplifier is on for an unusually long time (nominally 5 minutes). "
"Such a condition may indicate that the power amplifier is stuck on. "
"Reset the power amplifier, or the MRC-200 system to correct this fault.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

//FAULT_USING_BAT,
{ERR_1581, XSEVERITY_WARNING, "Running on internal batteries", "",
"This warning condition occurs if the MRC-200 is running from internal batteries. This may indicate that "
"the A/C or vehicle source has failed for some reason.\n"
 "1. Press the menu key until the Self-Test item appears.\n"
 "2. Press the select key to choose self-tests.\n"
 "3. Press the menu key until 'Fault Detail' appears.\n"
 "4. Press the select key to choose fault detail.\n"
 "5. Use the menu key to navigate through the fault sources. Press select to suppress the error."
},

{ERR_1582, XSEVERITY_ERROR, "Timeout waiting for data from Power Amplifier.", "",
  "A message from the Power Amplifier was not received in a timely fashion.\n"
  "This may indicate that the P/A is turned off, or not connected to the system."
},

{ERR_1583, XSEVERITY_ERROR, "Flash Parameters are unknown.", "",
  "The PC program has not received an the FlashROM parameters from the charger board.\n"
  "The flash ROM file system operation cannot be initiated."
},

{ERR_1590, XSEVERITY_ERROR, "I2C device %s is babbling", "",
"The specified I2C bus never became quiet when the system was waiting for a transmission."
},

{ERR_1591, XSEVERITY_ERROR, "Not data returned on I2C device %s", "",
"The specified I2C device did not return data when the system read the I2C bus."
},

{ERR_1592, XSEVERITY_ERROR, "Invalid script command:%s", "command",
  "The script command specified in a custom tool is invalid. Valid commands are:\n\n"
  "variable=value\n"
  "?variable\n"
  "wait number\n"
  "msg <text>\n"
  "The variables may be chosen from the list in generated_debug.txt"
},

{ERR_1593, XSEVERITY_ERROR, "Unknown variable:%s", "variable name",
  "The script command contains an unknown variable. Refer to generated_debug.txt for a list of valid names."
},

{ERR_1594, XSEVERITY_ERROR, "Error reading/writing variable to charger board:%s", "variable name",
  "An error occurred reading/writing a variable to the charger board.\n"
  "Verify that the version of software running on the charger board is the same as the version "
  "running on the PC."
},

{ERR_1601, XSEVERITY_ERROR, "Unable to find UDP channel to communicate with MRC-200.", "",
  "MRC-200.exe was unable to find an RS-232 channel with which to communicate with the MRC-200."
},

{ERR_1602, XSEVERITY_ERROR, "RS232 test has failed.", "",
  "This internal software test for the RS-232 channel has failed. This means that data sent out "
  "from the host computer did not match data returned after being echoed back through the Pico "
  "Card. Verify:\n\n"
  "   1. the RS-232 cable is plugged into the Pico Card.\n"
  "   2. the 9-pin connector is plugged into a host rs-232 port.\n"
  "   3. the com port matches the one specified on the self-test screen."
},

{ERR_1603, XSEVERITY_ERROR, "Unable to create process thread.", "",
  "This internal software error occurs because Rosetta is unable to create a process thread. This "
  "can arise when running under WIN-95 or WIN-98 instead of Windows NT. PicoUtil is not designed "
  "to operate on WIN-95, WIN-98, or Win-2000. Report to Pico Computing."
},

{ERR_1604, XSEVERITY_ERROR, "Unable to open %s port.", "com port name",
  "The com port cannot be opened because it does not exist or is already in use by another program."
},

{ERR_1605, XSEVERITY_ERROR, "Invalid com port settings.", "",
  "Valid examples are \"115200,n,8,1\" and \"baud=9600 parity=N data=8 stop=1\"."
},

{ERR_1606, XSEVERITY_ERROR, "Error accessing specified com port.", "",
  ""
},

{ERR_1607, XSEVERITY_ERROR, "Error Reading %s file.", ".picoutil\\picoutil.conf",
  "WxWidgets reported an error processing the file picoutl.conf. The context returns "
  "the information provided by Widgets.\n"
  "The exact name of the file in question can be obtained from the about message. Open "
  "this file with a text editor (such as notepad) and look for the syntax errors - they "
  "should be obvious."
},

{ERR_1792, XSEVERITY_ERROR, "User defined error in preprocessor '%s'", "user error text",
  "This error is induced by the statement #error() in an input file. For example:\n\n"
  "   #error(Cannot set both wide and narrow)"
},

{ERR_1793, XSEVERITY_WARNING, "User defined warning in preprocessor '%s'", "user warning text",
  "This warning is induced by the statement #warning .... in a source file. For example:\n\n"
  "   #warning Cannot set both wide and narrow"
},

{ERR_1800, XSEVERITY_ERROR, "Error starting Sockets", "",
  "An call to WSAStartup has failed. This is a fundamental error in the operating system and will "
  "probably oblige you to reboot your machine."
},

{ERR_1801, XSEVERITY_ERROR, "Timeout waiting for data on TCP/IP socket", "",
  "An attempt to connect to an IP socket has failed. IP is a general purpose communication protocol "
  "so the error may arise from a number of situations.\n\n"
  " If the failure occurs when trying to establish a session with a remote host:\n"
  "  - The remote machine is not accessible. Verify the IP address, and the port address.\n"
  "    Try PINGing the remote host to verify the IP address.\n"
  "    When the IP protocol is TCP (ie. TCP/IP) and the remote host is not ready for the session.\n"
  "  - Verify that the remote host is acting as a server and the command to RQL specifies \",client\" \n"
  "  - the remote host is acting as a client and the command to RQL specifies \",server\"."
},

{ERR_1803, XSEVERITY_ERROR, "TCP/IP connection gracefully closed", "",
  ""
},

{ERR_1804, XSEVERITY_ERROR, "Syntax error in command string to TCP / UDP driver", "",
  ""
},

{ERR_1805, XSEVERITY_ERROR, "Terminating sockets interface", "",
  ""
},

{ERR_1806, XSEVERITY_ERROR, "Unable to find unicast address to support multicast group", "",
  "The multicast address will use the immediately preceding unicast address to establish a multicast "
  "group. No unicast address was found."
},

{ERR_1807, XSEVERITY_ERROR, "Unable to start sockets.", "",
  "Error connecting a socket. This is probably because the version of Winsock (sockets implemented "
  "in Windows) such as WSOCK32.DLL is not the right version."
},

{ERR_1808, XSEVERITY_ERROR, "Error connecting to a socket %s", "",
  "THe TCP/IP function connect() failed to find the specified network or station on that network"
},

{ERR_1809, XSEVERITY_ERROR, "Device type parameter is illegal.", "",
  "Acceptable devices are udp or tcp."
},

{ERR_1810, XSEVERITY_ERROR, "Unable to create a socket.", "",
  "The system was unable to create a socket used to communicate with another host using UDP. This "
  "may be because the TCP drivers are not loaded in NT."
},

{ERR_1811, XSEVERITY_ERROR, "Missing delimiter in hostName/serviceName field.", "",
  "The host name and service name are specified in a UDP port specification such as:\n"
  "    \"udp,123.45.67.89\\VTT\" The host name \"123.45.67.89\" should be separated from the service "
  "name \"VTT\" by one of the following delimiters:     / \\ , :"
},

{ERR_1812, XSEVERITY_ERROR, "Unable to resolve host name (%s) to an IP address.", "",
  "The host name specified in a UDP port by a clause such as:\n\n"
  "    \"udp,Myhost\\vtt\"\n"
  "could not be resolved to an IP address using the standard sockets "
  "routines. Host names are resolved using a Domain Name Server (DNS) or by reference to the file\n\n"
  "    %systemroot%\\SYSTEM32\\DRIVERS\\ETC\\HOSTS.\n"
  "In the latter case a line such as\n"
  "    123.45.67.89    Myhost\n"
  "is expected."
},

{ERR_1813, XSEVERITY_ERROR, "Unable to resolve service name to an IP port address.", "",
  "The service name specified in a UDP port by a clause such as:\n"
  "    from \"tadilj,udp(Myhost\\myservice)\"\n"
  "could not be resolved to an IP port address using "
  "the standard sockets routines. Service names are resolved by reference to the file\n\n"
  "    %systemroot%\\SYSTEM32\\DRIVERS\\ETC\\SERVICES\n"
  "which expects a line something like:\n"
  "    \"vtt 8510/udp\""
},

{ERR_1814, XSEVERITY_ERROR, "Unable to create a process to handle UDP access.", "",
  "NT declined to create the thread used to handle UDP input. This is an NT error."
},

{ERR_1815, XSEVERITY_ERROR, "Error encountered reading a socket.", "",
  "The sockets interface returned a SOCKET_ERROR when requested to read a packet from the specified "
  "UDP port. This is an NT error."
},

{ERR_1816, XSEVERITY_ERROR, "Error encountered writing a socket.", "",
  "The sockets interface returned a SOCKET_ERROR when requested to write a packet to the specified "
  "UDP port. This is an NT error."
},

{ERR_1817, XSEVERITY_ERROR, "Error binding socket to local address.", "",
  "Error binding socket to local address."
},

{ERR_1818, XSEVERITY_ERROR, "Unable to join multicast group.", "",
  "An error was reported from the router when an attempt was made to join a multicast group. This "
  "will occur if the underlying unicast connection was not established - for example if there "
  "is no physical connection, or the router does not support multicast clients."
},

{ERR_1819, XSEVERITY_ERROR, "Error setting a socket to broadcast mode.", "",
  "Error setting a socket to broadcast mode."
},

{ERR_1820, XSEVERITY_ERROR, "Missing or invalid destination address for Route Add '%s'", "address",
  "The set ip.route=ip-address ... did not contain a valid ip address"
},

{ERR_1821, XSEVERITY_WARNING, "Inter Record Gap is significantly changed %s", "new inter record gap",
  "Pico.dll adjusted the inter record gap to the specified value in order to transfer data from the Pico Card. "
  "The Pico Card can transfer data over UDP at a rate that will exceed the capacity of any processor based "
  "system to receive. The inter record gap is an artificial delay injected into the data stream which "
  "gives the receiving system time to respond to the onslaught of data generated by the Pico Card.\n\n"
  "The inter record gap is nominally set to 100 microseconds (.1 milliseconds), however, when data was "
  "transferred from the Pico Card at this setting the error rate was quite high and Pico.dll increased "
  "the delay on retries.\n\n"
  "You may wish to use an override this computed value for no other reason than it takes time to "
  "probe for an appropriate setting when the first call to transfer a file is made.\n"
  "This can be done with PicoUtil from \"PicoUtil / menu / options / Switches and Debug settings /\"."
  "PicoUtil will remember the setting from run to run."
},
#define ERR_1822 1822
{ERR_1822, XSEVERITY_ERROR, "Timeout waiting for reply from server", "",
"A message from a server to a client was not received in a timely fashion.\n"
"This may signal that the server is down or has failed in some way."
},

{ERR_1830, XSEVERITY_ERROR, "Unable to ping target device", "",
  "Basic communication test (ping) between the host and target device has failed. "
  "This signals a baisc failure in the communication infracstructure. Verify that "
  "all cables are installed correctly. If the communication is over public infrastructure "
  "it could be caused by incorrect IP addresses, missing DNS, failed routers or "
  "many other causes."
},

{ERR_1851, XSEVERITY_ERROR, "Packet Buffer too small for received packet", "",
  "Packet Buffer too small for received packet"
},

{ERR_1852, XSEVERITY_ERROR, "Packet directed at Pico board lacks 'PICO' signature", "",
  "Packet directed at Pico board lacks 'PICO' signature. This is a fundamental sanity check on the "
  "incoming data and its absence suggests either:\n"
  "1. Some other program is using the Pico transfer port.\n"
  "2. There is a fundamental networking problem."
},

{ERR_1853, XSEVERITY_ERROR, "Monitor-reserved command code is undefined '%s'", "",
  "Monitor-reserved command code is undefined",
},

//Errors generated by cPico_flash
{ERR_2000, XSEVERITY_ERROR, "Unable to build synopsis of file data on flash", "",
 "The system was unable to build a synopsis of file headers on the flash ROM.\n"
 "Without this synopsis the flash ROM is unusable."
},

{ERR_2001, XSEVERITY_ERROR, "Invalid call sequence to ProgramAbuffer", "",
 "Invalid call sequence to ProgramAbuffer.\n"
 "Error - should never occur - always called with 16 or 1."
},

{ERR_2002, XSEVERITY_ERROR, "Physical parameters of flash ROM are not valid", "",
 "The flash parameters returned by GetPicConfig are not valid.\n"
 "This could be because the pico card does not have a flash ROM.\n"
 "In any case, without this information the flash ROM is unusable."
},

{ERR_2003, XSEVERITY_WARNING, "Last File on flash ROM", "",
 "A call to cPicoFlash::GetNext could find no more files after the specified position.\n"
},

{ERR_2004, XSEVERITY_ERROR, "File not found on Flash ROM '%s'", "flashFileName",
 "The file specified in a cPicoStream::LoadFPGA or cPicoFlash::Open was not "
 "found on the flash. Use PicoUtil or PicoCommand to write the file to flash.\n"
},

{ERR_2005, XSEVERITY_ERROR, "Missing or unknown command line argument'%s'", "command",
 "The command line does not contain valid commands.\n"
 "Use <pgm>.exe /h<commandLetter> for assistance with expected command line format."
},

//-------------------- Errors 25xx for MAD -------------------------------------------------------------------
{ERR_2500, XSEVERITY_ERROR, "Invalid attribute syntax in A/P declaration attributes: '%s'", "attributeFound",
 "The Automata Processor (A/P) attributes are specified in a string between < > brackets."
 "For example:\n"
 "   int12 <\"target=10, roll\"> ii;\n"
 "would declare a twelve bit integer value that rolled over when it reached its target value of 10.\n\n"
 "Acceptable keywords in the string are:\n"
 "all-input         STE is active for all inputs; applies to STEs\n"
 "start_of_data     STE is active at start of data; applies to STEs\n"
 "roll              Counter will roll over when it reaches its target value; applies to Counters\n"
 "pulse             Counter will roll over when it reaches its target value; applies to Counters\n"
 "hold              Counter will roll over when it reaches its target value; applies to Counters\n"
 "latch             Counter will roll over when it reaches its target value; applies to Counters\n"
 "report-on-target  Element will report when it files (STEs) or reaches its target(counters)\n"
 "at-target-latch   Counter will latch its value\n"
 "at-target-pulse   Counter will assert for one cycls\n"
 "target=           value at which the counter stops; applies to Counters\n"
 "reportcode=       applies to all Elements\n"
 "high-only-on-eod  applies to Logical elements only\n"
 "initial=          initial value loaded into programmable logic for each LUT; applies to LUTs\n"
 },

{ERR_2501, XSEVERITY_ERROR, "Missing close > in declaration attribute.", "",
 "The Automata Processor (A/P) attributes are specified in a string between < > brackets."
 "For example:\n"
 "   int12 <\"target=10, roll\"> ii;\n"
},

{ERR_2502, XSEVERITY_WARNING, "Attribute not implemented on specified platform: %s", "",
 "The specifed attribute is not appropriate to the current platform. For example:\n\n"
 "   int12 <\"report-on-target\"> ii;\n\n"
 "is not appropriate when the platform is S7.\n\n"
 "The platform can be specified in two ways:\n"
 " - in the environment by setting the variable madPlatform; "
#ifdef _WIN32
"set madPlatform=S7\n"
 " - on the mad commmand line using commands like /p S7.\n"
#else
"madPlatform=S7\n"
 " - on the mad commmand line using commands like -p S7.\n"
#endif
 "The command line will override the environment setting."
},

{ERR_2503, XSEVERITY_ERROR, "Invalid Initialization of variable:%s", "right hand side",
  "A declared variable followed by = expression, or = {list of expressions} is invalid.\n"
  "This could occur because:\n"
  "1. The type of variable does not allow initialization (eg ste foo=123 is invalid.\n"
  "2. The variable defines an scalar, but the right hand side of the = is a vector.\n"
  "   For example int12 foo={1,2,3} will provoke this error.\n"
  "3. The number of values on the right side exceeds the number between [].\n"
  "   For example, int12 foo[3] = {1,2,3,4,5,6};"
},

{ERR_2504, XSEVERITY_WARNING, "No bot containing an input parameter was encountered", "",
  "This is surprising. Did you perhaps forget the word input in the parameter list to a bot. For example:\n"
  "bot foo(input ch::string){...}"
},

{ERR_2505, XSEVERITY_WARNING, "Unexpected qualifier to a MAD statement", "",
  "The qualifiers const, or volatile, can only be applied to a declaration."
},

{ERR_2506, XSEVERITY_WARNING, "Feature %s is not available on current platform", "",
 "The platform can be specified in two ways:\n"
 " - in the environment by setting the variable madPlatform; "
#ifdef _WIN32
"set madPlatform=S7\n"
 " - on the mad commmand line using commands like /p S7.\n"
#else
"madPlatform=S7\n"
 " - on the mad commmand line using commands like -p S7.\n"
#endif
 "The command line will override the environment setting."
},

{ERR_2507, XSEVERITY_WARNING, "Invalid <attribute>: %s", "tag",
  "An invalid attribute was found between the angle brackets of a declaration\n"
  "The proper syntax i:\n"
  "  type <\"string\"> name(parameters,...\n"
  "The only valid attribute is \"initial=hex string\" appllied to a lut declaration.\n"
},

{ERR_2508, XSEVERITY_ERROR, "Incorrect signature in BOT file: %s", "signatureFound",
  "A .bot file comprises records; each record in the file contains a 16 bye header with a four byte signature. "
  "The signature encountered in the specified bot file is not one of the expected, namely:\n"
  "#define BOT_SIGNATURE      \"bot\x7F\"\n"
  "#define LUT_SIGNATURE      \"lut\x7E\"\n"
  "#define MON_SIGNATURE      \"mon\x7D\"\n"
  "#define FYL_SIGNATURE      \"fyl\x7C\"\n"
  "#define MEM_SIGNATURE      \"mem\x7B\"\n"
  "#define LYN_SIGNATURE      \"lyn\x7A\"\n"
},

{ERR_2509, XSEVERITY_ERROR, "Unable to find a record containing BOT code for specified botnumber: %s", "<source file info>",
  "A record was encountered in a .bot file which refers to a BOT number for which the corresponding code could not be found. "
  "This is either a file integrity problem or the MAD compiler has generated an incorrect file."
},

{ERR_2510, XSEVERITY_ERROR, "/* found without */ in a comment: %s", "startOfText",
  "The text /* was found without the corresponding */ in a comment."
},

{ERR_2511, XSEVERITY_CATASTROPHIC, "End of file: %s", "",
},

{ERR_2512, XSEVERITY_ERROR, "Local variable name may not be the same as the BOT name: %s", "elaborated botname",
 "A local variable inside a bot declaration may npot have the same name as the bot. For example:\n"
 "bot foo(ch:target)\n"
 "   {int foo;\n"
 "will provoke this error.\n"
 "The variable foo inside the declaration may be used to refer to properties of the bot itself."
},

{ERR_2513, XSEVERITY_ERROR, "Global variable '%s' must be declared as const", "variable name",
 "Global variables are not accessible to a bot. Access to information outside the bot requires special access routines "
 "(such as signalling events, RAM access, etc).\n"
 "A const global variable is used to control compilation, and may not be changed at program execution."
},

{ERR_2514, XSEVERITY_ERROR, "Cannot assign value to const variable:%s", "variable name",
 "A variable declared as constant may not be assigned a value."
},

{ERR_2515, XSEVERITY_ERROR, "Missing equal sign in #directive: %s", "variable name",
 "#directive variables which have a value other than true/false, must have an = to assign a new value:\n"
 "for example: '#nowarning=1234' is the proper syntax, '#nowarning 1234' will generate this error."
},

{ERR_2516, XSEVERITY_CATASTROPHIC, "Opcode not available on this platform: %s", "opcodeName",
 "The opcode generated by the MAD parser was not found in the opcode mapping file.\n"
 "The opcode mapping file is a set of #defines corresponding to the individual operators "
 "in the MAD language. For example, the macro \"operator==\" corresponds to the == operator.\n"
 "The macro definition will generally reside in the platform library file such as S7lib.mad for the S7 processor.\n"
 "The syntax of the appropriate define is:\n"
 "     #define operator\\x3D\\x3D(a,b) .....\n"
 "Ascii \\x3D is the equivalent to '=' so the above define represents \"operator==\"."
},

{ERR_2520, XSEVERITY_ERROR, "Missing '{', '}', or ';' in Sam statement: '%s...'", "",
"'{' following the keyword 'asm' was not found, or the closing '}' was not found. The expected syntax for asm statement is:\n"
  "   asm{opcode param1, param2,...;\n"
  "    label: opcode param1, param2,...;\n"
  "   ...\n"
  "      }\n"
  "Refer to the file <platform>Lib.mad for the exact opcodes implemented on the current platform.\n"
},

{ERR_2521, XSEVERITY_ERROR, "Invalid platform: %s in /p command or environment variable madPlatform", "<platform>",
  "The platform specified with the /p command or found in the environment variable madPlatform "
  "is not acceptable. One possible reason is that the file <platform>Lib.mad was not found in the "
  "include directory.\n"
  "\nNominally:\n"
  "<platform> = S7      Version 6 of stack bot\n"
  "The setting /p~ or madPlatform=~ will set the platform to unknown"
},

{ERR_2522, XSEVERITY_ERROR, "'%s' is not a valid opcode.", "",
},

{ERR_2523, XSEVERITY_ERROR, "Missing semicolon at end of asm statement: %s", "",
 "A semicolon is missing at the end of a line of assembler text within an asm{...} "
 "statement. This error can also occur if spurious text is found in the assember statement."
},

{ERR_2524, XSEVERITY_ERROR, "Stack overflow", "",
"The stack pointer in the BOT has exceed its boundaries."
},

{ERR_2525, XSEVERITY_ERROR, "Constant exceed register size of target", "",
"The value calculated by the MAD compiler is too large for the specified platform."
},

{ERR_2526, XSEVERITY_ERROR, "No bot file specified", "",
"The operation cannot be performed because a .bot file was not found.\n"
"The .bot file is the output from the compiler and contains the object code "
"for the BOTs, along with all the other debug support required for this design. "
"This file can be specified in a number of ways:\n"
"1. When mad is invoked (eg. mad test /r)\n"
"2. Using the f command from the monitor\n"
"3. Selecting or opening a project under the MAD IDE, and compiling the aforesaid project."
},

{ERR_2527, XSEVERITY_ERROR, "Illegal opcode", "",
},

{ERR_2528, XSEVERITY_ERROR, "asm{... may not have two parameters to $printf()", "",
"The $printf statement inside an asm{...} has some restrictions that do not apply to $printf(...) outside "
"the asm statement. These restrictions are:\n\n"
"1. That only the format string is passsed to the $printf().\n"
"   For example:    asm{$printf(\"start of major cycle\\n\");..... } is acceptable.\n\n"
"2. That the parameter must be pushed onto the stack before it can be used in the format phrase.\n"
"   For example:    asm{push 0; $printf(\"zero=%d\\n\");} is correct.\n\n"
"3. That at most one parameter may be specified in the format string.\n"
"   For example:    asm{push 1; push 0; $printf(\"zero=%d\\n, one=%d\\n\");} is erroneous.\n\n"
"   NOTE: The $printf is responsible for popping the parameter from the stack."
},

{ERR_2531, XSEVERITY_ERROR, "Requested bot number is larger than the available bots in the hardware", "",
""
},

{ERR_2532, XSEVERITY_ERROR, "Requested bits exceed capacity of underlying hardware", "",
"A declaration such as 'int12 xyz;' specifies a bit width (12 bits) which is larger than the "
"number of bits available onthe platform specified."
},

{ERR_2533, XSEVERITY_ERROR, "include directory not specified (as required for the /p command).", "",
"An include directory must be specified before the /p command can be used. "
"The /p command checks for the existence of the file <includeDir>\\<platform>.lib. "
"If this is not present, then the platform request (/p option) will fail."
},

{ERR_2534, XSEVERITY_ERROR, "Requested data not found for specified BOT %s in file %s", "botNumber",
"The requested data was not found in the file. This means that the object file does not have any "
"reference to the specified bot number. THe followuibg program could generate this error:\n"
"   int target[] = {0};\n"
"   bot foo(ii::target) P{....}\n"
},

{ERR_2590, XSEVERITY_ERROR, "Duplicate filename on command line: %s", "",
 "Two filenames have been found on the command line to the MAD compiler.\n"
#ifdef _WIN32
 "The most likely cause of this problem is a missing / in front of a command."
#else
 "The most likely cause of this problem is a missing - in front of a command."
#endif
},

{ERR_2600, XSEVERITY_ERROR, "Software error generating botcode from Madcode: %s", "",
 ""
},

{ERR_2601, XSEVERITY_ERROR, "Software error generating line number table from Madcode:", "",
 "The line number table is not properly ordered."
},

{ERR_2602, XSEVERITY_ERROR, "Unresolved label: '%s'", "",
 "A label could not be found when compiling a SAM program."
},
//------------------------------------------------------------------------------------------

//----Errors from sim.exe, sam.exe, or emu.exe ---------------------------------------------
{ERR_2700, XSEVERITY_INFORMATION, "pause for output", ""
 "The simulation has paused to allow the GUI to update the output messages.\n"
 "This condition occurs when stepping into a subroutine (F-11 button)."
},

{ERR_2701, XSEVERITY_INFORMATION, "Simulation has stopped at the requested line or PC", ""
},

{ERR_2702, XSEVERITY_INFORMATION, "pause for output", ""
 "The simulation has paused to allow the GUI to update the output messages.\n"
 "This condition occurs when stepping over a subroutine (F-10 button)."
},

{ERR_2703, XSEVERITY_MESSAGEBOX, "Sequence error in BRAM dump used", "",
 "The lines generated by the firmware simulation beginning with #bram[adr]\n"
 "were not in sequence. This is a failure in the dumpIndexes.sv."
},

{ERR_2704, XSEVERITY_MESSAGEBOX, "stop encountered during simulatioin/emulation", ""
 "This is a normal shutdown of a SAM program ocassioned by the stop statement."
},

{ERR_2705, XSEVERITY_MESSAGEBOX, "target of opcode is invalid", "" //not used
 "The target of an opcode should be a quoted string or a hex number.\n"
 "In either case the size should be exactly the keysize for the sort operation."
},

{ERR_2706, XSEVERITY_MESSAGEBOX, "the number commands(=%s) in user.cmds > space available", "command count"
 "The number of commands specified in the file user.cmds exceeds the capacity of\n"
 "the array cmds[] declared in simulationDriver.sv. You may increase the size (see \n"
 "define TEST_OPS in samDefines.sv) or remove some of the commands in user.cmds.\n"
 "Each opcode specified in user.cmds will occupy one word in cmds[], and some\n"
 "(such as read[m:n]) occupy multiple slots."
},

{ERR_2707, XSEVERITY_MESSAGEBOX, "Unable to open 'simulate.log'", ""
},

{ERR_2708, XSEVERITY_MESSAGEBOX, "#check message has invalid format: %s"
},

{ERR_2709, XSEVERITY_MESSAGEBOX, "Row[%sd] out of sequence"
 "During a scan or scin operation the row being scanned is found to be out of sequence."
},

{ERR_2710, XSEVERITY_MESSAGEBOX, "Unknown #check number: %s"
},

{ERR_2711, XSEVERITY_MESSAGEBOX, "target is invalid: %s\n"
"The target of an opcode should be a quoted string or a hex number.\n"
 "In either case the size should be exactly the keysize for the sort operation."
},
{ERR_2712, XSEVERITY_MESSAGEBOX, "%s inconsistent: FPGA versus software", "parameter"
 "The specified parameter is not equal to the current value as defined in the software."
},

{ERR_2713, XSEVERITY_MESSAGEBOX, "Wrong duck: %s", "",
 "The comparison of target against the sorted row does conform to the expected pattern.\n"
 "If the row is in sorted order, the ducks should be: <<<==>>>>>>, ie., some\n"
 "number of < comparison, possible zero == comparisons, and > thereafter."
},

{ERR_2714, XSEVERITY_MESSAGEBOX, "Invalid command on command: %2", "offending command",
 "The command passed to this progam on the command line is not recognized.\n"
 "Try <programName> /h for more information."
},

{ERR_2715, XSEVERITY_MESSAGEBOX, "$expect or $actual used recursively", "",
 "An $expect or $actual command may not be used inside another %expect or $actual command."
 "For example, the following code will provoke this error:\n"
 "$expect{\"X=15\"; $actual{\"X=$0\";} will provoke this error."
},

{ERR_2716, XSEVERITY_MESSAGEBOX, "commands(=%s) in userCmds.data has invalid length", "line length",
 "The length of the entry in userCmds.data should be 2*keySize + 4 (opcode)\n"
},

{ERR_2717, XSEVERITY_MESSAGEBOX, "commands(=%s) in userCmds.data contains invalid data", "data",
 "The data of the entry in userCmds.data should be hex\n"
},

{ERR_2718, XSEVERITY_MESSAGEBOX, "commands(=%s) in userCmds.data has invalid opcode", "opcode",
 "The opcode in an entry found in userCmds.data is invalid. Valid opcodes are: OP_BUG(=0x01),\n"
 "or OP_CFG(=0x02), OP_READ(=0x04), OP_WRYT(=0x05), OP_SCAN(=0x06), OP_SCIN(=0x07) + item bits(=0x40 or 0x80)."
},

{ERR_2719, XSEVERITY_MESSAGEBOX, "commands(=%s) in userCmds.data has invalid item field", "item",
 "The item field of an opcode in userCmds.data is invalid. Valid opcodes are:\n"
 "=0x40 (next scan is an INDX row), or =0x80 (next scan is PAGE/BOOK row)."
},

{ERR_2720, XSEVERITY_MESSAGEBOX, "Unable to find %s", "text",
 "The specified text was not found in the file simulate.log (output from FPGA simulation).\n"
},

{ERR_2721, XSEVERITY_MESSAGEBOX, "%s is not consistent with value from previous simulation", "paramName",
 "The specified parameter to the FPGA simulation cannot be changed when sim.exe is reprocessing the\n"
 "output file. The command 'sim /skipXsim' reprocesses the file simulate.log (output from Xilinx FPGA simulation)\n"
 "without actually re-invoking xsim.exe. Parameters such as bugLevel cannot be changed during this process\n"
 "because the necessary data was not generated in the original simulate.log file."
},

{ERR_2722, XSEVERITY_MESSAGEBOX, "Ducks do not correspond to grpMask: %s", "grpMask",
 "The ducks in the debug message [>>>==<<<<<] do not correspond to the generated group mask.\n"
 "Every group at or beyond the insertion point should be marked with a 1 signally that\n"
 "the group will be moved to the right on OP_SCIN. The group mask will be displayed in\n"
 "sets corresponding to the number of groups required to represent the hITEM."
},

{ERR_2723, XSEVERITY_MESSAGEBOX, "Insert point is incorrect: %s%s", "grpMask",
 "The insertion point for the specified target (as calculated by the FPGA/hardware)\n"
 "is not equal to the value computed by the software. The correct insertion point\n"
 "is will have the target greater or equal to kreg for every positrion to the left of\n"
 "the insertpoint, and for every comparision to the right less than kreg.\n"
 "The value returned by the hardware does not conform to this lofty standard :)"
},

{ERR_2724, XSEVERITY_MESSAGEBOX, "Error reported by firmware: %s", "error message", ""
},

{ERR_2725, XSEVERITY_MESSAGEBOX, "Data read from memory is incorrect, %s", "row.word address", ""
 "The data read from memory is not correct."
},

{ERR_2726, XSEVERITY_MESSAGEBOX, "Else is not followed by {", "", ""
 ""
},

{ERR_2727, XSEVERITY_ERROR, "Missing comma (,) in $bug statement.", "",
 "In the $bug macro '$bug(level, rowDump, rowType, sstep)' a comma was missing.\n"
 "The $bug macro generates a 16-bit opcode with the following C structure:\n"
 "   struct {uint16_t act    : 8, //0x00FF bits  = 0xD0 (OP_BUG)\n"
 "                    level  : 5, //0x1F00 bits  = bugLevel\n"
 "                    f0     : 1, //0x2000 bit type for dump (1 = INDX, 0=PAGE)\n"
 "                    f1     : 1, //0x4000 bit 1 = dump current row\n"
 "                    f2     : 1; //0x8000 bit 1 = single step mode\n"
 "          }\n"
 "The level parameter/field specifies the debugging level:\n"
 " == 0 - normal operation, supressing OP_PRINT output\n"
 " >= 1 - normal operation, allowing   OP_PRINT output\n"
 " >= 2 - normal operation, allowing   OP_PRINT output, and \n"
 "                          verifying '#expect:... ' versus '#actual...' assertions.\n"
 " >= 3 - $display all opcodes, but consolidate long literal (op_rimm + LDI, LDI,...)\n"
 " >= 4 - $display all opcodes except inside a subroutine (call .... ret)\n"
 " >= 5 - $display all opcodes\n"
 " >= 6 - $display results of comparison from each group \n"
 " >= 7 - $display action of sequencer cell (requires seqCell-withDebug.sv source)\n"
 " 8-31 - not used\n"
 "Note that OP_PRINT is a complicit opcode - host & SamPU are complicit in its execution.\n"
 "OP_PRINT is ignored in the hardware implementation (probably).\n"
 "If the .dump == 1, the FPGA/Xilinx simulation will $display the current DRAM row;\n"
 "in this context rowtype specifies the type of row,\n"
 "    ie, 1 = type INDX, 0 = type PAGE/BOOK\n"
 "This gimmick so that sim.exe can properly interpret the DRAM row received."
 "sstep == 1 signals single-step mode. This causes the Xilinx simulation \n"
 "or software emulation to pause awaiting user response. The simulation/\n"
 "emulation is under the control of the host thereafter."
},

{ERR_2728, XSEVERITY_MESSAGEBOX, "Shift Amount was not found or is invalid", "", ""
 "An opcode SHL or SHR should be followed by a comma and a number, for example SHL $4,3\n"
 "This error will also arise if the shift amount is larger than the size of the target bus."
},

{ERR_2729, XSEVERITY_MESSAGEBOX, "Missing equal ('=') in assignment statement", "", ""
 ""
},

{ERR_2730, XSEVERITY_MESSAGEBOX, "Register overflow", "", ""
 "The register for a multi word read/write has exceeded the available registers.\n"
 "For example: $6 = [15:17] reads address [15] to register $6,\n"
 "address[16] to register $7, and address [17] to register $8.\n"
 "The snag is that there is no reg$8."
},

{ERR_2731, XSEVERITY_MESSAGEBOX, "Improper qualifer", "", ""
 "The opcode CFG must be followed by (cell) or (group), eg., CFG(cell) or CFG(group)\n"
 "The opcode scan or scin must be followed by (indx), (page), or (book);\n"
 "eg., scan(page), scin(book)"
},

{ERR_2732, XSEVERITY_MESSAGEBOX, "Unknown or improper target specified", "", ""
 ""
},

{ERR_2733, XSEVERITY_ERROR, "Invalid element in an expression (%s).", "atom",
 ""
},

{ERR_2734, XSEVERITY_ERROR, "Repeat field is too large.", "atom",
 "The value specified for the repeat fields in the opcode OP_REPEAT must not be\n"
 "larger than 32."
},

{ERR_2735, XSEVERITY_ERROR, "Invalid row override.", "",
 "The override specified for an address is invalid. The syntax of an address expression\n"
 "is [wordAdr]                  single word access to current row.\n"
 "or [loAdr:hiAdr]              multiword access to current row.\n"
 "or [rowOverride:wordAdr].     single word access to row[rowOverride]\n"
 "or [rowOverride:loAdr:hiAdr]. multiword access to row[rowOverride]\n"
 "The row override may also be specified on the hiAdr, however, it must be identical to the\n"
 "row override on loAdr.\n"
 "loAdr and hiAdr may be interchanged in order to access the memory in reverse order.\n"
 "For example:\n   $5 = [0];  [0] = %5; $5 = [4:7]; $5 = [13@4:7];"
},

{ERR_2736, XSEVERITY_ERROR, "Invalid register.", "",
 "A register ($0 thru $7) is expected in the opcode specified.\n"
 "This error can also occur if the register range (eg, $5:$7) is incorrect."
},

{ERR_2737, XSEVERITY_ERROR, "Illegal placement of break statement.", "",
 "A break command must be in the context of a for-loop, while-loop or do-loop."
},

{ERR_2738, XSEVERITY_MESSAGEBOX, "Register not found following arithmetic opcode", "", ""
 "A register should follow an arithmetic opcode (ADD, ADC, SUB, SBB, CMP, XOR, OR, AND,\n"
 "INC, DEC, SHL, or SHR. For example ADD $4; ADC $5; INC $7; etc"
},

{ERR_2739, XSEVERITY_ERROR, "#expect: <value> not equal to #actual: <value> in simulation.", "",
 "In a SAM test programs the facility to specify the expected value of a given operation\n"
 "can be specifified using the #expect: / #actual: mechanism.\n"
 "The statement print \"#expect: Davy Jones $1  \\n\"; will set the expectation.\n"
 "A subsequent  print \"#actual: Davy Jones $5\\n\";   will satisfy this expectation\n" 
 "(registers $reg0 and $reg5 have the same values when the print statement is executed).\n"
 "However, if the messages differ (white spaces ignored) this error (2739) will be\n"
 "displayed and the simulation will be terminated."
},

{ERR_2740, XSEVERITY_MESSAGEBOX, "Address of call is beyond range of call OPcode.", "",
 "OP_CALL has an address range of 0-2047. The address requested is beyond that range.\n"
 "A workaround is to place a long jump goto(long) in the early part of the code -\n"
 "to the subroutine in question, and call this low table address. This code is nominally\n"
 "in the jmp table at the begining of the microcode."
},

{ERR_2741, XSEVERITY_MESSAGEBOX, "Sam source file not specified on command line.", "",
 "sim.exe expects the name of an ascii source file on the command line.\n"
 "Traditionally these have the .sam suffix.\n\n"
 "Proper invocation syntax is sim <fileName> </options>,\nfor example:\n"
    "\tsim myfile.sam\n"
 " or\tsim yourfile.sam /buglevel=3\n\n"
 "For comprehensive option information refer to the Sam manual or\n"
 "enter 'sim.exe -h' or 'sim /h'."
},

{ERR_2742, XSEVERITY_MESSAGEBOX, "/run command must be invoked from cmd.exe.", "",
 "The /run option must be invoked from cmd.exe; it cannot be invoked from a secondary\n"
 "program (such as Visual Studio). If you must invoke from Visual Studio use the command:\n"
 "    cmd /c samPgm /run samProgramName\n"
 "The run option will execute the Sam Program and generate a log file of the activity.\n"
 "The command $bug_indx, $bug_page, and $bug_book within the Sam Program will dump out the\n"
 "current row with appropriate formatting.\n"
 "The bugLevel should be set from within the Sam Program using $bug = value to specify\n"
 "opcode-by-opcode, line-by-line, or print only tracing (see ERR_2727).\n"
},
{ERR_2743, XSEVERITY_WARNING, "Target Register not selected", "",
 "When running a Sam program and an address is present in the 'Stop When' box, \n"
 "(on the right side of the form), the ListBox to the immediate right of the 'Stop When'\n"
 "label should specify a register to compare with the value in the box.\n"
 "Possible choices are:\n"
 "   $pc   - stop when the program counter equals the stop value.\n"
 "   $reg  - stop when the specified register equals the stop value,\n"
 "or $line - stop when the program starts executing the specified line." 
},

{ERR_3000, XSEVERITY_ERROR, "Missing name", "",
 "The compiler expected a variable name. This is typically in the context of a declaration;\n"
 " int <name>, ste <name>, bot <name>, etc).\n"
},

{ERR_3001, XSEVERITY_ERROR, "Duplicate name in declaration", "variable name",
 "The specified variable has already been declared in this context."
},

{ERR_3002, XSEVERITY_ERROR, "Variable '%s' has not been declared", "variable name",
 "The specified variable has not been declared in this context."
},

{ERR_3003, XSEVERITY_ERROR, "Missing semicolon in statement", "variable name",
 ""
},

{ERR_3004, XSEVERITY_ERROR, "Missing opening square bracket '[' or brace '{'", "",
 ""
},

{ERR_3005, XSEVERITY_ERROR, "Missing closing square bracket ']' or brace '}'", "",
 "The error message will display the beginning of the text that was not properly closed."
},

{ERR_3006, XSEVERITY_ERROR, "Invalid $variable '%s'", "variable encountered",
 "Valid simulate statements are:\n"
 "#input <string Expression>     defines input stream for simulation\n"
 "$stop  <expression>            defines expression signally simulation is complete\n"
 "$break(<expression>)           break to debugger in simulation\n"
 "$display(<format>, <expression1>, <expression2>,...)   display in simulation.\n"
 "For example:\n"
 "$input \"ABABABCD...HXYZABCDEF.HXYZ........\";\n"
 "$stop Found >= 7;"
},

{ERR_3007, XSEVERITY_ERROR, "Missing open parenthesis in if, while, for, or function call", "",
 "The open parenthesis is missing in an if, while, for, bot or ste declaration:\n"
 "Proper syntax is:\n"
 "if (expression) statementOnTrue [;else statementOnFalse;]\n"
 "while (expression) statementOnTrue \n"
 "for (variable=expression; expression; statement) forStatementBody\n"
 "set steName(inputChars)\n"
 "bot botName(input localChinx::stringVbl, event::EventVariable"
},

{ERR_3008, XSEVERITY_ERROR, "Invalid indexing expression for %s", "",
 "This error will occur for the following reasons:\n"
 " 1. The expression between the [] does not resolve to an integer.\n"
 " 2. The expression between the [] is outside the declared size of the variable.\n"
 " 3. The variable is not a vector but [expression] followed the variable name.\n"
 " 4. The variable is a vector but [expression] was not found.\n"
 "For example:\n"
 "    int12 Match[3];  Match[\"abc\"] = 0;       ""//\"abc\" is not of type integer\n"
 "    int12 Match;     Match[1]       = 0;     "  "//Match does not expect an index\n"
 "    int12 Match[3];  Match          = 0;     "  "//Match expects an index\n"
 "In the case of a bot having n parameters, the number of indexes must be equal to n:\n"
 "    bot foo(input ch::target, event::events)\n"
 " then foo[1,3]; is acceptable,\n"
 " but  foo[0];   is not acceptable."
},

{ERR_3009, XSEVERITY_ERROR, "Only one instance of $input or $stop allowed", "",
 "Only one instance of $input, or one instance of $stop statement is allowed.\n"
 "Proper syntax is:\n"
 "$input string;\n"
 "$stop  expression;\n"
 "For example:\n"
 "   $input  \"ABCDEFGHIJKLMNOPQRSTUVWXYZ\";\n"
 "   $stop Found != 0;"
},

{ERR_3010, XSEVERITY_ERROR, "Pre/post increment instruction must be followed by a variable name", "",
 "Proper syntax is: ++vbl, --vbl[1], +vbl[0].field, for example"
},

{ERR_3011, XSEVERITY_ERROR, "Cannot Pre and post increment a variable", "",
 "The statement 'x = ++y--' will generate this error"
},

{ERR_3012, XSEVERITY_ERROR, "Invalid opcode in p-code", "",
 ""
},

{ERR_3013, XSEVERITY_ERROR, "First parameter to $display function should be a string", "",
 "The proper format for the $display function is $display(format, expression1, expression2,...)"
},

{ERR_3014, XSEVERITY_ERROR, "Missing closing }, ) or ]", "",
 ""
},

{ERR_3015, XSEVERITY_ERROR, "Pre increment is not supported", "",
 "The increment instruction will take effect at the end of the major cycle (7nsec nominally)\n"
 "For this reason the pre-increment instruction will not take effect at the time you anticipate."
 "The pre-increment operation has been disabled.\n"
 "Use the post increment instruction and modify the test condition, eg,\n"
 "    if (++X >= 5) .....\n"
 "becomes:\n"
 "    if (X++ >= 4)...."
},

{ERR_3016, XSEVERITY_ERROR, "Unknown #directive or syntax error in #directive", "",
  "The preprocessor did not understand the #directive, or the #directive has a syntax error:\n"
  "Valid directives are:\n"
 "#ifdef <variable>             include following text up to #endif, #elif, or #else if variable is defined.\n"
 "#ifndef <variable>            include following text up to #endif, #elif, or #else if variable is NOT defined.\n"
 "#if <expression>              include following text up to #endif, #elif, or #else if expressions evaluates to non-zero.\n"
 "#error <string expression>    outputs expression to user\n"
 "#elif                         terminates #if and starts a new #if\n"
 "#else                         used in the context #if/#ifdef/#ifndef... #else ... #endif\n"
 "#endif                        terminates #if/#ifdef/#ifndef\n"
 "#define                       introduces a new #define variable\n"
 "                              #define variable text up to end of line      simple define.\n"
 "                              #define variable(p1, p2, p3) text up to end of line      define with parameters.\n"
 "#undef                        closes the scope of a #defined variable\n"
 "#include                      includes a source file\n"
 "#pragma                       defines compile and runs time options.\n"
 "#warning <string expression>  output warning message\n"
 "#for(variable=value; variable < value2; variable++)   beginning of a code generator\n"
 "                              the variable name is local to the for statement\n"
 "#endfor                       terminates #for"
},

{ERR_3017, XSEVERITY_ERROR, "Missing comma", "",
""
},

{ERR_3018, XSEVERITY_ERROR, "Invalid option or duplicate command on command line '%s'", "",
"The options specified on the command line are not valid. All Pico programs will print a "
 "synopsis of command line syntax in response to the /h command."
},

{ERR_3019, XSEVERITY_ERROR, "Parameters to a BOT must be type input or type event", "",
 "The proper declaration of a bot is:\n"
 " bot botName(input localVbl::intputSet, event localVbl2::eventSet) {....}\n"
},

{ERR_3020, XSEVERITY_ERROR, "Missing set membership operator (::)", "",
 "The proper declaration of a bot is:\n"
 " bot botName(input localVbl::intputSet, event localVbl2::eventSet) {....}\n"
 "This error indicates that ::inputSet or ::eventSet is missing"
},

{ERR_3021, XSEVERITY_ERROR, "Syntax requires a constant or constant expression", "",
"Prefix any declaration with 'const' to alert the MAP compiler that the value is constant.\n"
"This is particularly important in the context of a bot declaration, because the MAP compiler "
"will use the const information to generate optimized code. In fact the compiler may not be able "
"to generate a program for the target platform without this prefix."
},

{ERR_3022, XSEVERITY_ERROR, "Invalid declaration or statement:%s", "",
"A statement or declarations was not recognized.\n"
"Declarations have the general format: 'keyWord name[arraySpecs]...;'\n"\
"A statement must begin with one of the keywords if, while, for, or {"
// "The proper declaration of a bot is:\n"
// " bot botName(input localVbl1::inputSet, localVbl2::eventSet) {....}\n"
// "This error indicates that either localVbl1 or localVbl2 are missing"
},

{ERR_3023, XSEVERITY_ERROR, "Parameter in formatting phrase is inconsistent with the actual parameter", "",
 "The following code will generate this error:\n"
 "   display(\"text=%s\n\", 3);\n"
 "because the formatting phrase specifies a string parameter (%s) but an integer parameter (3) is provided."
},

{ERR_3024, XSEVERITY_ERROR, "Named parameter '%s' may not be mixed with positional parameters in a #define", "parameter",
 "A #define may use positional or named parameters, as follows:\n"
 "#define mac(name, value, scope)             //definition of macro\n"
 "mac(blotz,15,17)                            //invocation using positional parameters\n"
 "mac(.scope=15, .name=blotz, value=17)       //invocation using named parameters\n"
 "The combination:\n"
 "mac(.scope=15, blotz, 17)                   //mixed mode invocation\n"
 "is not supported.\n"
 "Named parameters for #defines with a variable number of parameters, such as:\n"
 "#define mac(name,...)\n"
 "mac(.name=blotz, 1 2 3)\n"
 "is also not supported."
},

{ERR_3025, XSEVERITY_ERROR, "Named parameter to a #define must be followed by '='", ""
},

{ERR_3026, XSEVERITY_ERROR, "Number of parameters to a #define does not match the declaration", ""
},

{ERR_3027, XSEVERITY_ERROR, "Named parameters to a #define used twice", "parameterName",
 "The proper syntax for named parameters to a define is:\n"
 "#define defineName(parameter1, parameter2)  ....\n"
 "defineName(.parameter1=5, .parameter2=6)\n"
 "A call to the define was attempted in which the same parameter was used twice, eg:\n"
 "defineName(.parameter1=1, .parameter1=y)"
},

{ERR_3028, XSEVERITY_ERROR, "Constant declaration must have an initial value", "variable name"
 "The keyword const preceding the declaration of an integer or boolean value implies that "
 "the variable in the body of the declaration has a constant value. This value must be assigned "
 "in the declaration, for example:\n"
 "const int size=15;"
},

{ERR_3029, XSEVERITY_ERROR, "Block define %s may not contain #define, #undef, or #include macros", "define name"
 "A block define, delimited by #define macroName(param1, param2,...)= .... #endef may include macros such "
 "as #for, #if, etc, but not #define, #undef, or #include."
},

{ERR_3030, XSEVERITY_ERROR, "The #define %s already exist with a different define body", "defineName",
"A #define defineName may be specified twice provided the second definition is identical to the first.\n"
"The program:\n"
"#define defineName 1234\n"
"#define defineName 2468\n"
"will cause this error.\n"
"However, if you need to change the meaning of a #define use #undef to cancel the previous definition\n"
"before creating the new #define."
},

{ERR_3031, XSEVERITY_ERROR, "Impermissible string operation in #directive", "defineName",
},

{ERR_3032, XSEVERITY_ERROR, "Invalid integer declaration '%s'", "variable type",
"The permissible fields in a declaration at int1, int2, int3,... int 48"
},

{ERR_3033, XSEVERITY_ERROR, "Define expansion level is too deep in '%s'", "define name",
"The specified define has been call recursively or in some other fashion that exceeds the acceptable depth."
"You may increase the acceptable depth of #define expansion using #pragma depth = <value>"
},

{ERR_3034, XSEVERITY_ERROR, "The expression cannot be realized on the underlying BOT '%s'", "",
"The instruction generated by the MAD compiler cannot be realized on the underlying BOT. "
"The context of the error message will identify the offending opcode, for example:\n"
"     foo.botsify[000]: 'binop|'\n"
"-    foo.botsify signals that the error occured in function 'foo' during translation from\n"
"     MAD code to BOT code.\n"
"-    [000]       is the bot code address within the function foo.\n"
"-    'binop|'    signals that the offending operation was a word OR.\n\n"
"You may find it useful to enable opcode debugging (#pragma opcodes=on) in order to locate this error.\n\n"
//"You may rescue this situation by creating your own custom opcode for word OR. To do this add\n"
//"    '#define operator| and the botcodes you wish to generate.\n"
//"Be sure to include a logic statement if your expect to invoke special purpose logic to evaluate this operation."
},

{ERR_3035, XSEVERITY_ERROR, "Too many  initializers for array: %s", "variableName",
"The hardware platform that you have targeted for this MAD program does not have facilities to support."
"the operation your MAD program has requested."
},

{ERR_3036, XSEVERITY_ERROR, "Illegal or out of sequence iterator in declaration: %s", "variableName",
"An expression such as {1,2,...,9} must specifiy the initial two and final values of the set.\n"
"Furthermore the direction extablished by the first two elements (1 and 2 in this case) must be "
"maintained by the last value (9 in this case).\n"
"Valid initilizers are:\n"
"   int xxx[] = {1,2,...9};\n"
"   int xxx[] = {1,2,10,20,...,90,91,92,... 99};\n"
"Invalid initializers are:\n"
"   int xxx[] = {1,1,...9};            1,1 does not establish a direction\n"
"   int xxx[] = {1,2,,... -99};        1,2  implies upward iteration, which can never reach -99"
},

{ERR_3037, XSEVERITY_WARNING, "Too few initializers: %s", "variableName",
"The number of values specified for an array is less than the declared size. "
"The remainder of the array will be filled with zeros, which may not be what you intend.\n"
"The following declaration will cause this error:\n"
"    int foo[8] = {1,2,3,4};"
},

//Errors from STM
{ERR_3500, XSEVERITY_ERROR, "STM error: %s", "",
 "General purpose error from STM scope. The scope will be disabled."
},

{ERR_3501, XSEVERITY_ERROR, "Field '%s' not found in Verilog file", "fieldName",
 "The field specified in the m3.cfg file was not found in the SCOPE_RECORD of scopeREcorde.v. The scope will be disabled."
},

{ERR_3502, XSEVERITY_ERROR, "Data read back from RAM does not compare with data written to RAM", "",
},
{ERR_3503, XSEVERITY_ERROR, "Requested block RAMs Exceeds total block RAMS on Zynq", "",
 "The context of this message will contain the size requested."
},

{ERR_3504, XSEVERITY_ERROR, "kcount (=%s) should be a 8, 16, 32, or 64.", "value of kcount",
"kcount is the number of BOTs built into the FPGA emulation. The only values supported for kcount are 8, 16, 32, or 64."
},

{ERR_3505, XSEVERITY_ERROR, "Record size (=%s) should be a power of two.", "value of record size"
},

{ERR_3506, XSEVERITY_ERROR, "kwidth should be numeric and non zero", "",
"kwidth is the number of bits in the sort field. It must be non-zero and would likely be more than 8 in any meaningfull FPGA emulation."
},

{ERR_3507, XSEVERITY_ERROR, "Unable to locate text '%s'", "text",
},

{ERR_3508, XSEVERITY_ERROR, "End of line 'set_property BISTREAM.CONFIG...' not found", "",
},

{ERR_3509, XSEVERITY_ERROR, "Unacceptable keyword: %s in regen command", "offending keyword"
},

{ERR_3510, XSEVERITY_WARNING, "Value(s) adjusted: %s", "values",
 "The number of bits calculated for LWIDTH is less than the number of bits required to address the interior "
 "part of the record (rcd_Size_Bits). This may occur when qcount=4, kcount=4, kwidth=4 and choke is specified.\n"
 "Solution: Use fullwrite (ie ~choke).\n"
},

{ERR_3511, XSEVERITY_ERROR, "Length requested for scope record (%s bytes) is too large", "sizeRequested",
"The length of scope record required to store the fields defined in SCOPE_RECORD (in the file callScope.v) "
"is too large for the scope to handle (specified in scope_payload).\n"
"You will have to remove some of the fields specified in SCOPE_RECORD"
},
{ERR_3512, XSEVERITY_WARNING, "Filter conflict: %s", "",
"Multiple filters have been specified. Scope can only support a single filter.\n"
"stm will ask which filter you wish to use. A more permanent correction can be affected by "
"changing the file m3.cfg to spcify only a single scopeFilter:"
},
{ERR_3513, XSEVERITY_ERROR, "Compare error in loopback test.", "",
  "The data returned from a loopback test to the E012 has failed.\n"
  "This probably indicates that the E102 firmware has not been build correctly "
  "or that stmServer.elf has not been built correctly.\n"
  "Naturally, these two subsystems and the program stm.exe must be consistent."
},

{ERR_3514, XSEVERITY_WARNING, "SCOPE_RECORD as defined in Verilog file does not comport with record returned from firmware", "",
"SCOPE_RECORD defined in callScope.v contains a SCOPE_FILLER field which is automaticly adjusted to the payload size of the internal scope.\n"
"This field (SCOPE_FILLER) will always be in the most significant bits positions, and have a value equal to its bitlength. "
"This error signals that these two values are not the same.\n"
"This error can occur if a field is entered with the incorrect size in the SCOPE_RECORD.\n"
"You may be able to spot the offending field using the output from the 'on command."
},
{ERR_3515, XSEVERITY_ERROR, "Requested variable exceed absolute upper limit of FPGA implementation", "",
"See context of message for more information"
},
{ERR_3516, XSEVERITY_WARNING, "field SCOPE_FILLER is incorrectly formed.", "variableName",
"The field SCOPE_FILLER must have the syntax 'localparam [size-1:0] = size;', for example 'localparam SCOPE_FILLER[18:0] = 19;'\n"
"This cranky rule is designed so that the software can verify that the record retrieved from the hardware is consistent "
"with the record defined in SCOPE_RECORD.v."
},
{ERR_3517, XSEVERITY_ERROR, "Invalid field name in scope record: %s", "fieldName",
"STM analyzes SCOPE_REORD in the file callScope.v.\n"
"Fields within COPE_RECORD may have bit designations provided the expressions defining "
"the bits use numeric constants or the following variables:\n"
"   ADDR_WIDTH,       DATA_WIDTH,       QCOUNT,           KCOUNT\n"
"   KWIDTH,           RCD_SIZE_WORDS,   DMEM_SIZE_1024,   SCOPE_DEPTH,\n"
"   SCOPE_PAYLOAD,    FIRMWARE_VERSION, SMEM_ROWS,        SMEM_COUNT,\n"
"   SMEM_ADDR_BITS,   K_ADDR_BITS,      LWIDTH,           MAX_RCD_COUNT,\n"
"   DMEM_ADDR_BITS,   KSIZE,            KLWIDTH,\n"
"   STA_SIZE,         RCD_SIZE_BITS,    SMEM_ROWS_BITS,   LWIDTH_MIN"
},
{ERR_3518, XSEVERITY_WARNING, "SCOPE_FILLER size was adjusted", "",
 "The size of the SCOPE_FILLER field in scopeRecord.v was adjusted to make the size of SCOPE_RECORD "
 "sequal to the payload size of scope.v"
},
{ERR_3519, XSEVERITY_ERROR, "Number of records reported does not match valuesin keyTable[]", "",
},
{ERR_3520, XSEVERITY_ERROR, "Patch not found: %s", "patchCode",
"The code marked //WARNING: generate code%s  or th ecorresponding //END WARNING was not found"
},
{ERR_3521, XSEVERITY_ERROR, "Incompatible parameters to generate function: %s", "reason",
},

{ERR_3522, XSEVERITY_ERROR, "Error in sort table (%s)", "type of error",
"Data found in the sort table is incorrect. Either it is out of sequence, the wrong "
"number of entries were found, or the keys do not correspond with the data written."
},

{ERR_3523, XSEVERITY_WARNING, "Sort RAM overflow", "",
 "The nuber of records written to RAM exceeds the capacity of the sort tables.\n"
 "Nominally the number of records that can be reliably sorted is kcount * (kcount/2 + 1). "
},

{ERR_3524, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3525, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3526, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3527, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3528, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3529, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3530, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3531, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3532, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3533, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3534, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3535, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3536, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3537, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3538, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3539, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3540, XSEVERITY_WARNING, "Cannot have competing states in state transition: %s", "state",
 "The declaration of a state variable specifies competing states for in the action field. "
 "For example:\n"
 "    state xxx {STATEA, STATEB, STATEC}: {STATEA : STATEB, STATEC;};\n"
 "will provoke this error because the state variable cannot transition to both STATEB and STATEC simultaneously."
},
{ERR_3541, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3542, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3543, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3544, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3545, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3546, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3547, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3548, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
{ERR_3549, XSEVERITY_WARNING, "Error not documented: %s", "variableName",
},
//end of STM specifi errors

{ERR_3038, XSEVERITY_WARNING, "Function, bot or ste declaration must be at the global level", "",
"A function, bot or ste must be declared at the global level. The may not be inside {} "
"and may not be inside another ste, bot of function declaration."
},

{ERR_6901, XSEVERITY_ERROR, "Platform flash failed to change ISC mode", "",
},

{ERR_6902, XSEVERITY_ERROR, "Block number(s) specified for Platform Flash is out of range", "",
},

{ERR_6903, XSEVERITY_ERROR, "Invalid parameter(s) to /fw command", "",
},

{ERR_6904, XSEVERITY_ERROR, "Invalid sub-command to /f<letter> command", "",
},

{ERR_6905, XSEVERITY_ERROR, "Jtag interface is busy", "",
},

{ERR_6906, XSEVERITY_ERROR, "Jtag interface was not opened", "",
},

{ERR_6907, XSEVERITY_ERROR, "Slot number proposed for JTAG interface is not valid", "",
},

{ERR_6908, XSEVERITY_ERROR, "LX45(T) is not loaded or failed to load when requested", "",
},

{ERR_6909, XSEVERITY_ERROR, "Device Position proposed for JTAG interface is invalid", "",
},

{ERR_6910, XSEVERITY_ERROR, "Error in TDO data returned from JTAG device or insufficient data returned", "",
},

{ERR_6911, XSEVERITY_ERROR, "GPIO interface failed", "",
},

{ERR_6912, XSEVERITY_ERROR, "GPIO Data strobe is not reflected properly by data strobe acknowledge", "",
},

{ERR_6913, XSEVERITY_ERROR, "Parameters to WriteLed are invalid", "",
"The whichLed parameter must be in the range 0-6 (ex500 backplane) or 0-7 (Ex700 backplane)\n"
"The rate parameter must be in the range 0-7\n"
"The color.0x0F bits must be in the range 0-7, an color.0xF0 bits must be in the range 0x00-0x70\n"
},

{ERR_6914, XSEVERITY_ERROR, "Specified Pico card does not have platform flash", "",
"The flash ROM supporting the Pico cards are as follows\n"
"   M501 - Spansion flash\n"
"   M503 - Platform flash\n"
},

{ERR_6915, XSEVERITY_ERROR, "Field specified is invalid", "",
 "Syntax for field access opcode is [$reg].<fieldName>. Valid <fieldName>s are:\n"
 "indxData, indxStop, P1, P2, count, total, pageStop, and\n"
 "key0, key1, key2, key3, key4, key5, key6, key7."  
},

{ERR_6916, XSEVERITY_ERROR, "Attempt to read platform flash failed after 10 attempts", "",
},

{ERR_6917, XSEVERITY_ERROR, "Command to platform flash requires the P/F be in ISC mode", "",
},

{ERR_6918, XSEVERITY_ERROR, "Invalid command to platform flash", "",
},

{ERR_6919, XSEVERITY_ERROR, "Unknown zw command ", "",
 "The zw commands to picocommand are ad-hoc commands for software debugging.\n"
 "They are not generally available in the released version of the product.\n"
 "Try zw0, zw40, zw50, or zw50 (ie values which are divisible by 10) for possible help information."
},

{ERR_6920, XSEVERITY_WARNING, "JTAG  interface is not available", "",
 "This error occurs if a Pico M5xx board is installed on a Tomahawk backplane board. The latter has not JTAG support."
},

{ERR_6921, XSEVERITY_ERROR, "JTAG lines are being driven by an external source.", "",
 "An external device (such as a JTAG programmer) is plugged into the JTAG device being accessed. "
 "This is driving the JTAG lines so that the internal connection cannot control them.\n"
 "Solution: Remove the external JTAG controller."
},

{ERR_6930, XSEVERITY_ERROR, "Unknown FPGA manufacturer", "testName",
},

{ERR_6931, XSEVERITY_ERROR, "FPGA not found at specified slot", "",
},


{ERR_6940, XSEVERITY_ERROR, "Unable to open bit file required for %s", "testName",
 "A bit file required to perform a specific test has not been provided was not found\n"
 "Refer to the options/user preferences menu item to specify the proper file."
},

{ERR_6941, XSEVERITY_ERROR, "Internal Software Error", "",
 "The code from MajorTests[] is invalid. "
},
{ERR_6944, XSEVERITY_ERROR, "Unable to locate Xilinx Impact program in picomfr", "",
 "Picomfr was unable to locate the Xilinx Impact tool (under Windows this is Impact.exe).\n"
 "Verify that the Xilinx tools are installed."
},

{ERR_6946, XSEVERITY_ERROR, "Unknown product", "",
 "The first two digits of the serial number should be the product number:\n"
 "11 - EX500,  12 - M501,      14 - M503,   15 - M504,  16 - BFBSX,\n"
 "17 - M505,  24 - M505K160T,  22 - M506,   26 - M507,  23 - EX700\n"
},

{ERR_6947, XSEVERITY_WARNING, "Conflicting FPGA/model number", "",
 "Each FPGA has a unique 16 digit number referred to as the DNA number. It is assigned by Xilinx.\n"
 "Each board has a unique number in the format MM.SSSS (MM=model, SSSS=serial number).\n"
 "MM is a constant for a particular board, and the SSSS numbers are assigned by the manufacturer.\n"
 "The DNA and product numbers should be in one-to-one correspondence.\n"
 "This error may arise from a number of conditions:\n"
 "  - You are re-testing a board that has already been tested.\n"
 "  - There is an error in the assignment of product numbers. This is serious "
 "and you need to get to the bottom of this problem before you proceed.\n"
 "If you are satisfied that this error is innocuous, picomfr will allow you to proceed "
 "even in the face of this error."
},

{ERR_6950, XSEVERITY_ERROR, "Missing delimiter in log file %s", "fieldName",
 "The test log file has become corrupted and may contain incorrect data.\n"
 "This is probably a software error. Please send the log file to Pico for further analysis."
},

{ERR_6951, XSEVERITY_ERROR, "Card%s is over temperature", "number",
 "The specified card is over temperature.\n"
 "The card in question should be marked by a flashing red light on the mother board.\n"
 "You should power down the system and remove the offending board for further inspection."
},

{ERR_7000, XSEVERITY_ERROR, "Flash ROM has no primary image", "",
  "Flash ROM has no primary image. The primaryBoot.bit file on the flashROM is required to "
  "start up the target Pico device. Without this file you will have to load through the JTAG "
  "connector."
},

{ERR_7001, XSEVERITY_ERROR, "Files are missing in Flash ROM directory", "",
  "On a write-all command from MRC-flashUpdate.exe, some files were not found in the flash ROM directory. "
  "at the end of the write-all operation.\n"
  "You may write the missing files 'by hand', ie. using write \"menu / File / Write Flash File\"."
},

{ERR_7002, XSEVERITY_ERROR, "Notice: Do you wish to write a backup of the primary image", "",
  "The bit image file on the Flash ROM is used to load the FPGA at startup. In the event there "
  "is an error in this process, the backup image will be loaded. This message is generated when "
  "a primary image is being written to the Flash ROM to give you the option of writing an identical "
  "backup image."
},

{ERR_7003, XSEVERITY_ERROR, "E-12 Pico card is operating in degraded mode.", "",
  "On the E-12 Pico Card the driver Pico64.sys has found the Address Extension Register (AER) is "
  "not working correctly and has switched to a mode in which the attribute memory is used for "
  "certain register operations. The card should work correctly but will be noticeably slower."
},

{ERR_7004, XSEVERITY_ERROR, "Error reading remote file system.", "",
  "An error has occurred reading from a remote file system over TCP/IP."
},

{ERR_7005, XSEVERITY_ERROR, "Uncorrectable error reading remote file:", "fileName",
  "An uncorrectable error has occurred reading from the specified remote file.\n"
  "This error on the NAND flash itself and has already been retried several times. "
  "Further retries are not likely to correct this situation.\n"
  "A list of the errors in this file was stored in the corresponding .txt file."
},

{ERR_7006, XSEVERITY_ERROR, "Uncorrectable error reading NAND flash:", "",
  "An uncorrectable ECC error has occurred reading from the NAND flash after several retries."
},

{ERR_7007, XSEVERITY_ERROR, "Initialization incomplete", "",
  "The driver (pico.sys under Windows or pico.ko under Linux) has not yet completed initialization.\n"
  "Delay at least 100 milliseconds and call the operation again\n"
},

{ERR_7008, XSEVERITY_SYSTEM, "Unacceptable load of pico_drv.ko", "",
  "There is an unknown problem in which Linux refuses to power on the card after pico_drv.ko "
  "has been removed (rmmod) and reinserted (insmod) four times !\n"
  "Pico_drv.ko declined to load because it would crash Linux so do to."
},

{ERR_7009, XSEVERITY_SYSTEM, "Pico Card is not ready:", "",
  "The Pico card was removed or otherwise rendered inoperable. WARNING: Be alert to the possibility "
  "that the computer will \'blue screen\' attempting to fix this problem. Make sure you have terminated "
  "any program that has volatile information."
},

{ERR_7010, XSEVERITY_DANGEROUS, "Flash ROM not ready when expected.", "",
  "The Flash ROM should not report a \'not ready\' status when no operation is being performed. "
  "This test is performed during the \'SelfTest / FlashROM status register\' operation."
},

{ERR_7011, XSEVERITY_DANGEROUS, "Error reading Pico Card Configuration information.", "",
  "The configuration information read from the Pico Card did not have either the proper signature "
  "or did not have acceptable version information."
},

{ERR_7012, XSEVERITY_DANGEROUS, "Incorrect signature in %s registers.", "register name",
  "The signature in the designated register did not have the expected value."
},

{ERR_7013, XSEVERITY_ERROR, "I/O access to AER register failed self test.", "",
  "At startup of PicoUtil, and attempt to verify the operation of the Address Extension Register "
  "accessed through an I/O port failed the write/read self test. The error returns three values:\n\n"
  "       writeValue != readValue1 or readValue2\n\n"
  "The writeValue is the numbers written to the AER. readValue1 is the value returned.\n"
  "If writeValue != readValue1 it suggests that the register did not write correctly.\n"
  "readValue2 is the result of a second read. If readValue1 != readValue2 it suggests that the register "
  "is not read correctly.\n\n"
  "This error could occur if you have loaded your own FPGA image which does not have an address "
  "extension register.\n\n"
  "Reset the Pico Card (ie eject the card and reinsert). This will reload the PrimaryBoot.bit file "
  "which should correct the problem."
},

{ERR_7014, XSEVERITY_ERROR, "Attrib memory access to AER register failed self test.", "",
  "At startup of PicoUtil, and attempt to verify the operation of the Address Extension Register "
  "accessed through attribute memory failed the write/read self-test.  The error returns three "
  "values\n\n"
  "       writeValue != readValue1 or readValue2\n The writeValue is the numbers written to the "
  "AER. If writeValue != readValue1 it suggests that the register did not write correctly. If "
  "readValue1 != readValue2 is suggests that the register did not read correctly.\n\n"
  "This error could occur if you have loaded your own FPGA image which does not have an address "
  "extension register.\n\n"
  "Reset the Pico Card (ie eject the card using the eject button on the Windows desktop, and reinsert). "
  "If the Pico Card is running a normal PrimaryBoot.bit file the error should be corrected."
},

{ERR_7015, XSEVERITY_ERROR, "The text 'Pico Computing' was not found in the manufacturer section of attrib memory.", "",
  "If this is a standard image from Pico Computing this text will always be found. If it is not "
  "it suggests that there is something fundamentally wrong with the access to the flash ROM. Check "
  "the usual suspects:\n\n"
  "  1. That the card is properly inserted.\n"
  "  2. That the image properly implements the read of flash ROM from PCMCIA.\n"
  "If this error occurred after you rebooted a new image, reboot with the primary image. If this fails to correct "
  "the problem, eject the card and reinsert it."
},

{ERR_7016, XSEVERITY_ERROR, "Data delivered from JTAG spy FIFO is not in the expected sequence.", "",
  "In this test the JTAG port is alternately written with data and the status is read back. This "
  "error occurs when the JTAG spy port does not faithfully record these transitions.\n\n"
  "NOTE: Normal system operation accesses the parallel port occasionally. You will not get consistent "
  "results with this test unless you disable the parallel JTAG port (use Control panel / System "
  "/ Device manager). Don't forget to re-enable the parallel port or the internal jtag will "
  "not be available!"
},

{ERR_7017, XSEVERITY_WARNING, "BAR registers have incorrect values.", "",
#ifdef WIN32
  "The BAR (Base Address Registers) for the RAM address or Port address are incorrectly set. Use "
  "the device manager applet in Windows to find the correct values."
#else
  "This error indicates that the BAR registers assigned by the OS when the Pico Card "
  "was inserted are invalid. The most likely cause of this is that the OS has not "
  "powered the card on correctly.\n"
  "The only know solution to this problem is to power down the system and restart."
#endif
},

{ERR_7018, XSEVERITY_WARNING, "File %s also linked to BackupBoot.bit", "file name",
  "File linked to PrimaryBoot.bit file has also been linked to BackupBoot.BIT.\n\n"
  "NOTE: ELF files linked to PrimaryBoot.bit will not be invoked when the card is inserted into a PC."
  "However, they will be invoked when a BIT file is re-booted or when the card is powered on in standalone mode.\n\n"
  "NOTE: ELF file may be linked to BIT files and BIT files to ELF files, however,"
  "BIT files can only be linked to other BITs files when the originating file is PrimaryBoot.bit.\n\n"
  "For additional information about standalone mode refer to the manual Standalone.chm or Standalone.pdf."
},

{ERR_7019, XSEVERITY_ERROR, "File %s cannot be linked to this file.", "file name",
  "An attempt has been made to link a file with another file that has unacceptable type. The context "
  "will contain the name of the source fileName.\n"
  "Files other than bit files or ELF files can never be "
  "linked to another file.\n"
  "The target (ie linked) file does not have to exist when the linkage "
  "is established.\n"
  "A bit file can be linked to an ELF file - which is taken to be mean that the "
  "ELF file will be executed after the FPGA file has been loaded.\n"
  "or\n"
  "An ELF file can be linked to a bit file - which is taken to mean that the bit file must be loaded into the FPGA "
  "before the ELF file is executed,\n"
  "or\n"
  "A bit file may be linked to PrimaryBoot.bit - which is taken to mean that the second bit file will be loaded when PrimaryBoot "
  "file is loaded when the Pico Card is operating in standalone mode."
},

{ERR_7020, XSEVERITY_ERROR, "Serial number cannot be written without a valid primary image .", "",
  "The FPGA model number is written when the serial number is applied to the board. This information "
  "is obtained from the PrimaryBoot.Bit file. The PrimaryBoot.bit file could not be found or the "
  "model information was not found within this file."
},

{ERR_7021, XSEVERITY_WARNING, "Cannot find linked file %s", "file name",
  "The file specified as a linked file was not found. The reference to the linked file will be "
  "written to the current file header, however, when the current file is executed or loaded the "
  "linked file may not be found. You can write the linked file at a later time."
},

{ERR_7022, XSEVERITY_WARNING, "File %s Loaded because of AlwaysLoad property of PrimaryBoot.bit.", "alwaysLoad file",
  "The alwaysLoad property of PrimaryBoot.bit has caused the specified file to be loaded when the Pico Card was "
  "inserted into the slot.\n\n"
  "NOTE: This warning is generated when the specified AlwaysLoad program is detected. It does not verify "
  "that the program was actually loaded.\n\n"
  "To disable this functionality remove the property AlwaysLoad from PrimaryBoot.bit."
},

{ERR_7023, XSEVERITY_WARNING, "RAM test will appear to stall.", "",
  "Under Linux self-tests will not reflect their progress on the Picoutil dashboard, or on the lower panel of the screen. "
  "This is because the background mode of operation has not been enabled in this release of Picoutil"
  "for Linux\n"
  "Be patient, the test will run to completion."
},

{ERR_7024, XSEVERITY_ERROR, "Invalid bits in self test mask (%s).", "offending bits",
  "The bits specified by the command 'picocommand -t<bits> has values that are not "
  "recognized by selfTest. The message will contain the bit(s) that give offence.\n"
  "Delete the bits from the bit mask and try again."
},

{ERR_7025, XSEVERITY_ERROR, "Unknown Pico model number '%s'", "",
  "Model number of pico card could not be found in internal table. Verify that you have the latest version of the software.",
},

{ERR_7026, XSEVERITY_ERROR, "Firmware is out of sync with driver: Reload FPGA", "",
  "This error will occur if the driver (pico64.sys or pico.ko) is re-loaded on a "
  "pico M50x card, and some I/O activity has been performed on the previous firmware.\n\n"
  "The error only occurs under version 5+ of the firmware.\n\n"
  "Reload the FPGA to correct this condition."
},

{ERR_7036, XSEVERITY_ERROR, "File does not checksum %s.", "file name",
  "The checksum of the specified file is not equal to the computed checksum for the file. This indicates that "
  "the file has been corrupted somehow. Replace the file on the flash with an up-to-date copy."
},

{ERR_7037, XSEVERITY_ERROR, "Error deleting file %s.", "file name",
  ""
},

{ERR_7038, XSEVERITY_ERROR, "Error deleting directory %s.", "directory name",
  ""
},

{ERR_7049, XSEVERITY_INFORMATION, "Invalid setting for debug flags '%s'.", "offending letter code",
  "The letter code specifying the debug flag is not valid:\n"
  "Acceptable letters can be obtained by entering picocommand -h bug"
},

{ERR_7050, XSEVERITY_INFORMATION, "Pico64.sys or pico.ko is running in debug mode.", "",
  "The timing test may be somewhat compromised since the debugging version of pico64.sys is "
  "slower than the release version. Check that no debugging switches are set. This too which will materially "
  "slow the operation of Pico64.sys or pico.ko."
},

{ERR_7051, XSEVERITY_WARNING, "No Tests specified.", "",
  "No tests were specified to the self-test command."
},

{ERR_7052, XSEVERITY_WARNING, "Debug setting will have minimal effect when driver is built in release mode.", "",
  "The version of pico64.sys or pico.ko was compiled in release mode. This has minimal debug support."
},

{ERR_7090, XSEVERITY_ERROR, "Error reading/writing to device.", "wrong number of bytes",
  "The number of bytes transferred in a stream read or write operation does not match the number "
  "requested. This is usually caused by the underlying firmware device timing out.\n"
  "This may not always be an error, but may signal that the amount of data requested "
  "is not available."
},

{ERR_7091, XSEVERITY_ERROR, "Compare error in picoStream loopback test.", "data is wrong",
  "The data returned form the stream loopback test is incorrect."
},

{ERR_7100, XSEVERITY_SYSTEM, "Unable to find systemRoot in environment.", "",
  "The environment variable \'systemRoot\' was not found in the environment. From cmd.exe the "
  "command \'set\' will display all environment variables. Why systemroot is not found in the system "
  "environment is a bit of a stumper. It probably means your windows installation is corrupted. "
  "Try closing the copy of cmd.exe and starting again."
},

{ERR_7101, XSEVERITY_SYSTEM, "Error reported by operating system:", "",
  "This error occurs if the specified driver did not load correctly."
},

{ERR_7102, XSEVERITY_SYSTEM, "Error in compilation (%s).", "structure name",
  "This error occurs during execution of a program. The program verifies that the size of various "
  "structures is consistent with the sizes of called DLL's. Please report this failure to Pico Computing."
},

{ERR_7103, XSEVERITY_INFORMATION, "FPGA has already been booted.", "",
  "This is an internal information message used on the Pico E17 cards."
},

{ERR_7104, XSEVERITY_SYSTEM, "Attempt to load .chm file failed '%s'", "chm file name",
  "An attempt load the designated help file has failed. Two possible reasons for this are:\n"
  "  1. The .chm file is not present in the c:\\pico\\doc directory. \n"
  "  2. The Microsoft HTML help executable (hh.exe) is not in the current path."
},

{ERR_7105, XSEVERITY_SYSTEM, "Error deleting registry key '%s'", "registry key name",
  "An error occurred deleting the specified registry key. See the context of this message for "
  "further information from the operation system.\n"
  "One possible reason for this error is the specified key is protected."
},

{ERR_7106, XSEVERITY_SYSTEM, "Error accessing Pico Card:", "",
  "An error has occurred using DeviceIoControl(...) to access Pico64.sys. Additional information "
  "will be provided from the operating system. Possible reasons for this error are:\n\n"
  "  1. The Command code (second parameter to DeviceIoControl) is not valid.\n"
  "  2. The size of the return buffer is too small for the information requested.\n"
  "  3. The operation is not valid on the specific card (eg writing to flash on an E-16 card).\n"
  "  4. Pico64.sys encountered an error when it started.\n"
  "The supporting information in the error message should indicate the system error and its meaning.\n"
  "You may be able to enable debugging in the driver and locate the offending error. To do this:\n"
  "1. Copy pico\\bin\\picoD.sys to Windows\\System32\\drivers (overwriting pico64.sys).\n"
  "2. Eject and reinsert the Pico Card.\n"
  "3. Set the debugging flags either from PicoUtil (menu / Options / Pico64.sys debug settings /...), "
  "or with PicoBug.exe.\n"
  "4. Capture the output using DbgView.exe (from sysInternals.com)."
},

{ERR_7107, XSEVERITY_SYSTEM, "Error reported by operating system:", "",
  "This error occurs if the driver Pico64.sys did not load correctly. Possible reasons for this "
  "error are:\n"
  "  1. The  Pico64.sys is not present in Windows\\system32\\drivers.\n"
  "  2. The driver was not installed correctly.\n"
  "  3. Another program has opened Pico64.sys in exclusive mode (the normal mode for PicoUtil is "
  "exclusive).\n"
  "  4. The Pico Card was ejected from the slot while another program was attached to the card. "
  "You may be able to terminate the program, eject card and re-insert the card.\n"
  "  5. As a last resort you will have to restart your computer."
},

{ERR_7108, XSEVERITY_SYSTEM, "Error reported by operating system:", "",
	"The operating system reported an error to the driver or the channel class. Call GetLastError() for description."
},

{ERR_7109, XSEVERITY_SYSTEM, "Unable to open driver %s", "driver file name",
  "This error occurs if the driver Pico64.sys did not load correctly. Possible reasons for this "
  "error are:\n\n"
  "  1. The Pico Card was not plugged into the card slot.\n"
  "  2. The driver Pico64.sys is not present in Windows\\system32\\drivers.\n"
  "  3. The driver was not installed correctly. This is sometimes due to registry corruptions. "
  "This might be\n"
  "     corrected by deleting the registry keys associated with the driver (UnInstallTool.exe) "
  "and reinserting the Pico Card.\n"
  "     This will allow the hardware wizard to rescan the device and recreate the registry settings.\n"
  "  4. Another program has opened Pico64.sys in exclusive mode (the normal mode for PicoUtil is "
  "exclusive).\n"
  "  5. The Pico Card was ejected from the PCMCIA slot without using the eject card icon on the "
  "desktop. You may be able to press the eject card and correct the situation. However, it is more "
  "likely that you will have to restart your computer.\n WARNING: Be alert to the possibility "
  "that the computer may \'blue screen\' attempting to fix this problem. Make sure you have terminated "
  "any program that has volatile information."
},

{ERR_7110, XSEVERITY_ERROR, "Unable to reboot.", "",
  "The system cannot reboot the Pico Card. This could be caused by a number of things:\n"
  "  1. The bit image currently running does not support rebooting. See capabilities under tools/capabilities.\n"
  "  2. Major failures such as Pico Card not inserted or driver (Pico64.sys) not loaded.\n To correct "
  "this situation, either:\n\n"
  "  1. Use the Hard reboot option (menu / startup / Hard Reboot Primary FPGA file). This will "
  "power off the Pico Card and then power it back on again.\n"
  "  2. Remove the card using the remove card icon (under windows), or the proper removal procedure "
  "under Linux. \n"
  "Then reinsert the card. Either of these procedures will restart the card with PrimaryBoot.bit."
},

{ERR_7111, XSEVERITY_ERROR, "Reboot port did not complete successfully.", "",
  "The request to reboot the FPGA did not complete properly.\n"
  "On the E17, M501, or M503 this error signals that the write operation intended to reload "
  "the FPGA did not return the proper number of bytes.\n\n"
  #ifndef EXCLUDE_PICO_STREAM_EXX
  "On the E14 This error indicates that the port PICOPORT_REBOOT did not signal completion when the FPGA was rebooted.\n"
  "Normally the reboot circuit will signal that rebooting has commenced. This signal was not received within "
  "one second of initiating a reboot."
  #endif
},

{ERR_7112, XSEVERITY_ERROR, "Unknown or wrong image loaded.", "",
  "The system was unable to determine which image is loaded or the image loaded was not the one "
  "requested. This error is caused by the following failures:\n\n"
  "  1. The FPGA image loaded did not report the address of the last image loaded through the "
  "\'peekaboo\' port. This may be because the image does support this functionality or because "
  "of some software failure attempting to read this port. Verify that the FPGA file supports peekaboo "
  "(menu / tools / current FPGA capabilities).\n\n"
  "  2. The system was unable to read the directory on the flash ROM. If you are running PicoUtil "
  "this error will be very obvious because no directory will be visible in the upper panel.\n\n"
  "  3. The image requested is invalid and did not cause the done bit to be asserted on the FPGA. "
  "The CPLD which loads the image will continue reading the flash ROM until a valid image is loaded."
},

{ERR_7113, XSEVERITY_ERROR, "Tuple in '%s' not present.", "tuple name",
  "The CIS being processed refers to a linked tuple that was not found in the CIS. The target "
  "of the linked tuple will be marked by a type 0x13 tuple. A list of the valid target tuples "
  "is displayed in the context of this message."
},

{ERR_7114, XSEVERITY_ERROR, "CIS in '%s' has errors.", "FPGA image or file",
  ""
},

{ERR_7115, XSEVERITY_ERROR, "Unable to read CIS.", "",
  ""
},

{ERR_7116, XSEVERITY_ERROR, "Unable to write CIS to file.", "",
  ""
},

{ERR_7117, XSEVERITY_ERROR, "Flash Status bit is misbehaving.", "",
  "The Flash Status bit should signal that the flash is busy immediately after initiating an erase "
  "operating and should then signal not busy after a reasonable time. The status signal did not "
  "behave as expected."
},

{ERR_7118, XSEVERITY_ERROR, "External program already active.", "",
  "Another program has been launched from PicoUtil. Terminate this program before starting another one."
},

{ERR_7119, XSEVERITY_ERROR, "Flash ROM did not enter CFI mode correctly '%s'.", "type of error",
  "CFI (Common Flash Interface) is a standard supported by the flash ROM by which information "
  "about the flash ROM can be obtained. This error indicates that the attempt to enter CFI mode "
  "failed, or that the data returned in CFI mode is incorrect (missing signature, improper value, "
  "etc).\n"
  "Possible causes for this error are:\n"
  "  1. Flash ROM hardware failure.\n"
  "  2. FPGA firmware which does not access the flash ROM correctly.\n"
  "Possible solutions:\n\n"
  "  1. Hard reboot of the card (menu / startup / hard reboot).\n"
  "  2. If you have changed the firmware (FPGA image) try using a known good image.\n"
  "  3. Reprogram the card using the JTAG cable with a known good image. Do not overlook reprogramming "
  "the CPLD also.\n"
},

{ERR_7120, XSEVERITY_ERROR, "Unable to read file '%s'.", "file name",
  ""
},

{ERR_7121, XSEVERITY_ERROR, "Unable to write CIS to card.", "",
  ""
},

{ERR_7122, XSEVERITY_WARNING, "Xilinx Image patched to remove WBSTAR=0.", "",
  "The bit file generated by the Xilinx software inadvertently includes the "
  "command to clear the WBSTAR register. This prevents the system from properly "
  "reporting the address of the file loaded from flash.\n"
  "When this patch is made the clear of the WBstart register is replaced by NOP's."
},

{ERR_7123, XSEVERITY_ERROR, "FPGA image does not return proper signature.", "",
  "Standard Pico bit files return a signature of 0x5397. This sanity check has failed. Possible "
  "reasons for this are:\n\n"
  "1. The image currently loaded on the card is not a standard image. \n  - For the E-12, E-14, "
  "or E-15 cards, try terminating any programs using the card,\n"
  "   ejecting the card (using the eject card icon), and reinserting it. This will reload\n"
  "   the primary image which should return the proper signature. \n  - For the E-16 card you "
  "can reload the PrimaryBoot.bit file.\n\n"
  "NOTE: This signature is also known as a \'magic number\'.\n"
  "NOTE: This signature is reported on port 0x0C for the E-12, E-14, E-15, and E-17 cards.\n"
  "      This signature is reported at memory address oxFFE0000C on the E-16 card."
},

{ERR_7124, XSEVERITY_WARNING, "Pico64.sys did not load correctly.", "",
  "The driver Pico64.sys was unable to allocate memory resources sufficient to interface to the "
  "Pico Card. This could be caused by:\n\n"
  "  The CIS on the Pico Card (which describes the card to the operating system) is corrupted "
  "to the point that the driver (Pico64.sys) was obliged to request an emergency set of resources. "
  "You will have to rewrite the CIS to correct this situation from a file such as Pico[model#].cis "
  "(eg PicoE12FX.tuple, PicoE14FX.cis) etc.\n\n"
  "  2. Your computer system does not in fact have such resources. If this problem occurs on a "
  "card which you have not written a new CIS please inform Pico Computing, Inc."
},

{ERR_7125, XSEVERITY_ERROR, "Unable to find file for command line request '%s'.", "file name",
  ""
},

{ERR_7126, XSEVERITY_ERROR, "Primary must be .bin or .bit file '%s'.", "file name",
  ""
},

{ERR_7127, XSEVERITY_ERROR, "Space is not available in file header for the changes requested '%s'.", "file name",
  "The space allocate for the file header will not be reallocated when a request is made to change "
  "the local flash ROM file name or the note field associated with the file. The new information "
  "must fit within the generous space already allocated for a file header. In order to fix this "
  "problem, you must:\n"
  "  1. Copy the file to the PC (file/save file to PC),\n"
  "  2. Delete the file from the flash ROM.\n"
  "  3. Add the file to the flash ROM system with the new local file name and notes.\n"
},

{ERR_7128, XSEVERITY_ERROR, "File does not have the proper header '%s'.", "file name",
  ""
},

{ERR_7129, XSEVERITY_ERROR, "Formatted output from keyhole caused system failure.", "",
  "The format string used in the function KPrintf() caused an error when it was executed on the "
  "PC side. This could be because the % arguments in the string do not correspond to the actual "
  "arguments supplied to KPrintf(). The print request is ignored."
},

{ERR_7130, XSEVERITY_ERROR, "Error writing flash ROM '%s'.", "file name",
  ""
},

{ERR_7131, XSEVERITY_ERROR, "Nothing to save.", "",
  ""
},

{ERR_7132, XSEVERITY_ERROR, "Error Opening: file %s.", "file name",
  ""
},

{ERR_7133, XSEVERITY_ERROR, "File is not an FPGA image file (.bit or .bin).", "",
  "A request has been made to boot a file that is not an FPGA bit image."
},

{ERR_7134, XSEVERITY_ERROR, "Image %s does not have keyhole interface.", "file name",
  "The FPGA file loaded as the current image does not support the keyhole interface. This error "
  "can also occur in the following situations:\n\n"
  "  1. The driver (Pico64.sys) and upper level software are not compatible with the PPC program "
  "Monitor. For example, the transition from V3.3.x.x to V3.4.x.x made this incompatible. To correct "
  "this problem upgrade the firmware on the Pico Card.\n"
  "  2. The image loaded was different from the current image because an ELF program specified "
  "a linked bit file which is different from the original.\n"
  "  3. A temporary error condition has occurred which shut down the keyhole. Restarting PicoUtil "
  "will normally correct this error."
},

{ERR_7135, XSEVERITY_ERROR, "File %s has errors and been removed.", "file name",
  ""
},

{ERR_7136, XSEVERITY_ERROR, "Pico Card to PC Keyhole protocol error.", "",
  "The keyhole debugging port has lost data from the Pico Card. This could be because the Pico "
  "Card and the application program are out of sync (as would happen when either was just restarted). "
  "You may either:\n\n"
  " 1. Restart PicoUtil. This will execute the startup self test.\n"
  " 2. Disable and re-enable the keyhole (menu / Debugging / Keyhole Polling / Disable & Normal)\n"
  " 3. Disable this test permanently (menu / Debugging / Keyhole Polling / Normal + 7136)"
},

{ERR_7137, XSEVERITY_ERROR, "Error deleting file %s.", "file name",
  ""
},

{ERR_7138, XSEVERITY_ERROR, "Sector reference is invalid", "",
  "The information returned from the peekaboo port cannot possible refer to a valid sector in "
  "the flash ROM file system."
},

{ERR_7139, XSEVERITY_ERROR, "Elf or data file %s has improper size.", "file name",
  "The size of an ELF or other data file in inconsistent with the number of sectors allocated "
  "to the file. You may correct this situation by deleting the file and re-writing it from the PC."
},

{ERR_7140, XSEVERITY_ERROR, "Error accessing driver.", "",
  "The driver did not return the number of bytes expected."
},

{ERR_7141, XSEVERITY_ERROR, "Unable to find Xilinx signature (FF FF FF FF AA 99 55 66) in file %s.", "",
  "The .bit and .bin files created by ISE have a signature which must be found in order for the "
  "FPGA image to load successfully. The file or data block provided does not have such a signature."
},

{ERR_7142, XSEVERITY_ERROR, "Notes exceed sector size.", "",
  ""
},

{ERR_7143, XSEVERITY_ERROR, "Primary boot file not found at logical sector 2.", "",
  "The primary boot file contains an image required by the FPGA at startup. This file should be "
  "the first file on the flash ROM. The first file on the flash ROM is located on logical sector "
  "#2. If necessary other files occupying the space required for the boot image will be moved. "
  "This error indicates that the primary image could not be written at sector 2, or that the backup "
  "image could not be written immediately following sectors.\n\n"
  "You may be able to correct this problem by manually examining the allocation of each file (debugging/show "
  "allocation) and removing or copying files that are located near the beginning of the flash "
  "ROM. Then try to rewrite the primary image again.\n\n"
  "NOTE: without the primary image you will be unable to restart the Pico Card (ie power off and "
  "power on) without reloading the card using a JTAG cable."
},

{ERR_7144, XSEVERITY_ERROR, "Error reading file '%s'.", "file name",
  ""
},

{ERR_7145, XSEVERITY_ERROR, "Invalid File '%s'.", "file name",
  "This error occurs if the ascii header information on a sector does not meet syntax checks. "
  "This is most likely caused by data written to the flash ROM which is not part of a file. This "
  "also may also arise if hardware errors prevented a sector from being properly erased or written.\n\n"
  "The data will normally be gathered in a file marked \"!!invalid file #\" and may be removed "
  "using menu file/delete."
},

{ERR_7146, XSEVERITY_ERROR, "Invalid file signature '%s'.", "sector number",
  "The ascii header on the beginning of a flash file name must conform to the following syntax:\n"
  "    Bit File=fileName\\n)\n"
  "or\n"
  "    Elf File=fileName\\n\n"
  "or\n"
  "    Data File=fileName\\n\n"
  "and may be followed by other ascii fields specifying other "
  "characteristics of the file. A file header has been encountered which does not conform to these "
  "rules."
},

{ERR_7147, XSEVERITY_ERROR, "Unable to create file %s.", "file name",
  ""
},

{ERR_7148, XSEVERITY_ERROR, "Unable to locate backup image.", "",
  "The backup bit image file is normally located after the primary bit Image file on the Flash "
  "ROM, however, it may be located at a higher address. The system was unable to locate a bit "
  "image file at the usual location or in any other location."
},

{ERR_7149, XSEVERITY_ERROR, "Unable to create primary file %s.", "file name",
  "The primary boot file must be the first file physically on the flash ROM. This error is usually "
  "caused by the first allocation being used by another file."
},

{ERR_7150, XSEVERITY_ERROR, "Failed verify of %s.", "file name",
  "The request to verify a file against the copy on the PC has found differences. The verify has "
  "failed. This error may also occur during self-test when data read from the flash ROM is different "
  "upon subsequent reads."
},

{ERR_7151, XSEVERITY_ERROR, "Updated files is not the same size as the file being replaced '%s'.", "file name",
  "This error will only occur when you are trying to update the primary boot image or the backup "
  "boot image. This error indicates that you are replacing the boot image file with a larger file "
  "from the PC disk. Since the boot image files are located on specific sectors this is not permitted. "
  "Remove the files that are conflicting with the boot image and try the write operation again."
},

{ERR_7152, XSEVERITY_ERROR, "Unable to allocate space on flash ROM for file '%s'.", "file name",
  "PicoUtil was unable to allocate space for the file specified. For .bit or .bin files the space "
  "must be available in a contiguous space. For files other than types bit (or bin) the space "
  "can be available in discontiguous sectors."
},

{ERR_7153, XSEVERITY_ERROR, "ELF file %s does not have proper signature.", "file name",
  "A valid ELF file always starts with the signature 0x7F \'ELF\'. The file found in the dump "
  "file is not correctly signed. The dump file might still load correctly, however, it is unlikely "
  "that the ELF file will execute as expected. You may replace the ELF file when the dump file "
  "has been loaded onto the Pico Card."
},

{ERR_7154, XSEVERITY_ERROR, "Unable to find a primary boot file in %s.", "file name",
  "The dump file you are loading to the Pico Card does not have a primary boot image file. It "
  "is unlikely that this file will work correctly on the Pico card when it is loaded. You can "
  "write a primary boot file using File/Write Primary Boot from the menu of PicoUtil."
},

{ERR_7155, XSEVERITY_INFORMATION, "Incorrect number of bytes returned from Pico Card.", "",
  "A request to read or write a stream returned the incorrect number of bytes. This is a normal condition and should "
  "be handled by the application software. However, the application may report this error when "
  "it is expecting data but none was returned from the Pico Card."
},

{ERR_7156, XSEVERITY_WARNING, "Incompatible version (non-fatal) '%s'.", "bad version found",
  "The version reported by the one component of the system is not fully compatible with the version "
  "reported by another component. The system will work correctly, however, some functions will "
  "be limited. Generally versions can be expected to be incompatible if the major and minor version "
  "numbers are different. For example 3.1.4.0 and 3.2.6.9 should be incompatible since the "
  "major version (3) and the minor version (1) are the different.\n\n"
  "Look at the context of this error message for more detailed information about which pair of "
  "components caused offense. For example a context message such as\n\n"
  "  \'Error #7156: Incompatible version: Firmware: (FPGA) V3.5.0.1 versus PicoUtil.exe: V3.2.3.00.\'\n"
  "  indicates that the firmware in the FPGA image and PicoUtil.exe are incompatible."
},

{ERR_7157, XSEVERITY_ERROR, "Incompatible version (fatal) '%s'.", "bad version found",
  "The version reported by the one component of the system is not compatible with the version "
  "reported by another component. Generally versions can be expected to be compatible if the major "
  "and minor version numbers are the same. For example 3.1.4.0 and 3.1.6.9 should be compatible "
  "since the major version (3) and the minor version (1) are the same.\n\n"
  "Look at the context of this error message for more detailed information about which pair of "
  "components caused offense."
},

{ERR_7158, XSEVERITY_ERROR, "Pico Card requires Windows-7 or better", "",
  ""
},

{ERR_7159, XSEVERITY_ERROR, "Erase of flashROM sector timed out '%s'.", "data on flash rom",
  ""
},

{ERR_7160, XSEVERITY_ERROR, "Error reading flash ROM.", "",
  ""
},

{ERR_7161, XSEVERITY_ERROR, "Error writing PC file %s.", "file name",
  ""
},

{ERR_7162, XSEVERITY_ERROR, "Error Reading Flash ROM (%s)", "flash ROM address",
  "The address specified for a read from flash ROM is negative, or larger than the size of the "
  "flash ROM."
},

{ERR_7163, XSEVERITY_ERROR, "Primary boot file is improperly located '%s'.", "file name",
  "The primary boot file should be located on the third logical sector of the flash ROM dump file. "
  "The file may not operate properly if loaded onto the Pico Card."
},

{ERR_7164, XSEVERITY_ERROR, "%s is a read-only file.", "file name",
  ""
},

{ERR_7165, XSEVERITY_ERROR, "Error reading file %s.", "file name",
  ""
},

{ERR_7166, XSEVERITY_ERROR, "CIS data does not meet consistency checks.", "",
  ""
},

{ERR_7167, XSEVERITY_ERROR, "Invalid data in CIS.", "",
  "This error is caused when hex data in a Card Information Structure (CIS) is not found. A likely "
  "cause of this error is a length field in a specific tuple is wrong. An incorrect length fields "
  "causes the tuple syntax scanner to get out of sync. The length byte in each tuple should be "
  "the total length of the tuple minus 2 (2=the type and length field itself)."
},

{ERR_7168, XSEVERITY_ERROR, "Missing END marker (0xFF) in CIS data.", "",
  ""
},

{ERR_7169, XSEVERITY_ERROR, "/src/ not found in source file name:%s", "directory name",
  "The file name provided for a compilation step must conform to the syntax:\n"
  "   directoryName\\src\\fileName.c.\n"
  "This naming convention is used to simplify the automatic "
  "construction of the file script (build.bat) and the linker script (linkerScript.sh). The files "
  "in the compile directory are presumed to be organized as follows:\n\n"
  "    .\\src\\        contains the .c file(s) and script files.\n"
  "    .\\             will contain the output .exe or .elf file.\n"
  "    .\\src\\build.bat        is the cmd.exe script.\n"
  "    .\\src\\linkerScript.sh is the linker script file.\n\n"
  "    The script files will be created if they are not found.\n"
  "    You can modify these script files for you own environment."
},

{ERR_7170, XSEVERITY_WARNING, "Last tuple in Configuration is not the last tuple entry.", "",
  "The CISTPL_CONFIG (type=0x1A) defines the last tuple index for this configuration. However, "
  "this number did not correspond to the last configuration entry (type = 0x1B) in the CIS. This "
  "is probably not a fatal problem (operating systems tend to ignore this inconsistency), however, "
  "it should be corrected."
},

{ERR_7171, XSEVERITY_ERROR, "Error: Multiple linking tuples in CIS chain.", "",
  ""
},

{ERR_7172, XSEVERITY_ERROR, "Duplicate Card Configuration tuple.", "",
  ""
},

{ERR_7173, XSEVERITY_ERROR, "MFC CIS has only one linked tuple.", "",
  ""
},

{ERR_7174, XSEVERITY_ERROR, "Files are identical in a update request.", "",
  "The file on the PC and the file on the flash ROM are identical (except possibly for timestamps). "
  "This is not an error but will save you the time to actually update the file."
},

{ERR_7175, XSEVERITY_ERROR, "File sizes are different in an update request", "",
  "An attempt has been made to update a file from the PC to the flash ROM in which the number "
  "of sectors occupied by the new file will be different from the number of sectors currently "
  "allocated to the old file. This can be done, however, the following observations apply:\n\n"
  "  1. When updating a file with a smaller file, the unused space will be returned to the system. "
  "If a subsequent attempt to update the file is made with a file requiring the same amount of "
  "space, this will not be possible. In this subsequent case, the following observation applies:\n\n"
  "  2. The file cannot be replaced in-situ because the new file requires more sectors than the "
  "file it is replacing (ie new file is larger). If you proceed, the file will be deleted and "
  "reallocated at a randomly selected address on the flash ROM. This is almost certainly acceptable. "
  "In no cases can a primary boot file be replaced with a file of different size."
},

{ERR_7176, XSEVERITY_ERROR, "Invalid/duplicate environment setting", "",
 "The $environment statement in a sam program defines the platform on which the\n"
 "sam program will be executed. The format of $environment statement is:\n"
 "$environment [FPGA | hardware | Xilinx simulation | compile only | software emulation];\n"
 "   FPGA:               Object code is intended for an FPGA emulation of SAM.\n"
 "   hardware:           Object code is intended for a hardware implementation of SAM.\n"
 "   Xilinx Simulation:  Object code is intended for Xilinx's xsim.exe (simulation).\n"
 "   compile only:       sim.exe will stop after the object file is created.\n"
 "   software emulation: The object code file is intended for software emulation in sim.exe.\n"
 "Only one setting for the environment may be made in a source module."
},

{ERR_7177, XSEVERITY_ERROR, "invalid IoCtl buffer size", "",
  "System error: call to DeviceIoControl with insufficient output buffer size"
},

{ERR_7178, XSEVERITY_ERROR, "Error accessing driver", "",
  "The internal reference to a platform specific interface was NULL. This is probably because "
  "the PCMCIA card was removed without properly closing the driver, or because the hardware/firmware "
  "failed initialization tests."
},

{ERR_7179, XSEVERITY_ERROR, "PCMCIA Error: unsupported ioctl code", "",
  "System error: invalid control code passed to DeviceIoControl"
},

{ERR_7180, XSEVERITY_ERROR, "Expecting a string in print statement '%s'.", "actual parameter",
  "The syntax of the print statement is:\n"
  "    print \"string1\";\n"
  "or  print \"string1\" \"string2\" \"string3\",...;\n"
  "The compiler will concatenate the strings and generate an OP_PRINT code. The strings\n"
  "are stored ascii 0x00 delimitted in the file <samProgramName>.msgFile.\n\n"
  "The message number embedded in the OP_PRINT code is used by host software to create\n"
  "the message at run time; no strings are stored on the FPGA/hardware.\n\n"
  "The strings may contain parameters signalled by a '$' sign.\n"
  " $reg    specifying the content of the specified register.\n"
  " $line   the line number of the source code.\n"
  " $pc     the program counter in the emerging object code.\n"
  "For Example:\n"
  "   print \"register 5=0x$5 on line $line at pc=$pc\";"
  "NOTE: target strings used in the context of an OP_SCAN, OP_SCIN, of $reg = command\n"
  "      are not stored in <samProgramName>.msgFile. They are created as needed in\n"
  "      the SamPU opcode stream."
},

{ERR_7181, XSEVERITY_ERROR, "Unknown command in a message to the Pico Card '%s'.", "bad command",
  "A command (the first word) sent to the Pico Card is unknown. The command was not sent. Enter "
  "\'help\' in the \'Msg->Pico\' box to obtain a list of valid commands."
},

{ERR_7182, XSEVERITY_ERROR, "Number and type of parameter is inconsistent with command '%s'.", "bad parameter",
  "In a command sent to the Pico Card the number of parameters supplied is not correct for the "
  "command. For example \'version 123\' would generate this error because the version command "
  "does not expect a parameter. Enter \'help command\' (eg help version) for assistance with the "
  "particular command."
},

{ERR_7183, XSEVERITY_ERROR, "Expression has invalid syntax.", "",
  "In a command sent to the Pico Card the command analyzer has encountered an unexpected error "
  "such as duplicated operators (eg a * * b), or some other syntax error.\n"
  "Generally an expression has the form:\n"
  "   operand [operator operand]*.\n"
  "The operand may be a variable, literal, or may be replaced by an expression in parenthesis.\n"
  "For example, 1+2, 1*(3+4), 0xFFFF, vbl+1 are three valid expressions.\n"
  "The operators +, -, *, / (integer divide), "
  "% (modulus), ^ (exclusive or), | (inclusive or), & (and), and the comparison operators (> >= "
  "<, <=, =, !=) are implemented and have the same precedence as C.\n"
},

{ERR_7184, XSEVERITY_ERROR, "Missing or unbalanced parentheses () in an expression '%s'.", "expression",
  ""
},

{ERR_7185, XSEVERITY_ERROR, "Contiguous space required for bit image could not be found.", "",
  "Bit images must be allocated in contiguous space so the turbo loader on the Pico Card can load "
  "the image into the FPGA. Such a block of contiguous space could not be found. The system will "
  "move files on the flash ROM to find the sufficient space."
},

{ERR_7186, XSEVERITY_ERROR, "Space required for primary boot image is already occupied by %s.", "file name",
  "The file will be moved to make room for the primary boot image."
},

{ERR_7187, XSEVERITY_ERROR, "Space required for backup boot image is already occupied by %s.", "file name",
  "A backup boot image will not be written. If you wish to write a backup image, erase the aforementioned "
  "file and try the operation again."
},

{ERR_7188, XSEVERITY_ERROR, "Space required for backup boot image is already occupied by %s.", "file name",
  "The primary file will be written, but the backup will not."
},

{ERR_7189, XSEVERITY_ERROR, "invalid element in an expression.", "",
  "The command analyzer has encountered an unexpected item in an expression."
},

{ERR_7190, XSEVERITY_ERROR, "duplicate filename in flash ROM directory '%s'", "file name",
  "The name of the file being added to the flash ROM directory is already in use. You can correct "
  "this situation by changing the \'name used in flash ROM file system\' box displayed when you "
  "write a file to the Pico Card."
},

{ERR_7191, XSEVERITY_ERROR, "Invalid character in file name '%s'", "file name",
  "The file being added to the flash ROM directory has an invalid character. Acceptable characters "
  "are: $all alphanumeric characters` _.~!@#$%^&*()-{}\\/[]"
},

{ERR_7192, XSEVERITY_WARNING, "User cancelled", "",
  "The current action was not completed because the user cancelled the process."
},

{ERR_7193, XSEVERITY_ERROR, "Information only.", "",
  "The message associated with this error is informational only. For example, this error number "
  "might be used to report help information from an internal function."
},

{ERR_7194, XSEVERITY_ERROR, "Flash file name and PC file name must be the same type '%s'", "file name",
  "The type of a file is determined from its extension (characters following the \'.\').\n"
  "  .bin, .bit, and .bak are FPGA bit image files.\n"
  "  .elf and .exe are ELF images.\n"
  "All other extensions are treated as data files.\n"
  "This error indicates that an attempt was "
  "made to write a flash file named inconsistently with these rules. For example, changing file "
  "extensions from .elf, to .bit would provoke this error."
},

{ERR_7195, XSEVERITY_ERROR, "Pico Card is not ready or not running a primary image.", "",
  "The flash ROM on the Pico Card is not visible to the application program. Possible reasons "
  "for this are:\n\n"
  "  1. The bit Image running on the Pico Card does not support flash ROM access.\n"
  "  2. The card is not inserted.\n"
  "  3. The driver (Pico64.sys) is not loaded or not available (it should be located in Windows\\system32\\drivers).\n"
  "  4. The Pico Card was not installed properly.\n\n"
  "To correct this situation try the following:\n"
  "  1. On the menu select Startup/Boot primary bit file. This will reload the primary image which "
  "should be capable of accessing the flash ROM.\n"
  "  2. Unload the card from the computer (using the unload button in windows). "
  "If you have replaced the primary image then you may NOT BE ABLE TO RECOVER "
  "from this situation under windows. Refer to the documentation for emergency recover procedures."
},
#ifdef COMPILED_FOR_MRC_200
{ERR_7196, XSEVERITY_ERROR, "Board at address %s is not available.", "requested board number",
  "The specified card is not available. \n\n"
  "An attempt has been made to switch the focus of MRC-FlashUpdate (using ALT-num sequence) "
  "to a card which is not available.\n\n"
  "The IP address specified by the ALT-num sequence must specify a value between 1 and 254. "
  "NOTE: Card numbers are assigned sequentially and are not changed unless that card is removed "
  "and reinserted. It is possible to have non-contiguous card numbers (eg. 1,2,4,5,6 etc)."
},
#else
{ERR_7196, XSEVERITY_ERROR, "%s Card is not available.", "requested card number",
  "The specified card (Pico1, Pico2, etc) was not found. \n\n"
  "When multiple cards are loaded into a machine they are referred to by the device name "
#ifdef WIN32
  "  \\\\.\\Pico1, \\\\.\\Pico2, etc.\n\n"
#else
  " /dev/pico0, /dev/pico1, etc. There may also be supporting devices such as /dev/pico0m etc.\n\n"
#endif
  "An attempt has been made to switch the focus of PicoUtil to a card which is not available.\n\n"
  "NOTE: Card numbers are assigned sequentially and are not changed unless that card is removed "
  "and reinserted. It is possible to have non-contiguous card numbers (eg. 1,2,4,5,6 etc)."
},
#endif
{ERR_7197, XSEVERITY_ERROR, "Pico Card setting (/i) not available on simulated file (/z).", "",
  "The options to PicoCommand /z (simulate) and /i (specify Pico Card) cannot be used in combination. "
  "Simulation is restricted to a single file (eg, /z simulate.fyl), and /i # specifies a particular "
  "Pico Card. These cannot be used simultaneously."
},

{ERR_7198, XSEVERITY_ERROR, "Feature (%s) is not available in this release.", "name of feature",
  "The named feature is not available in this release. This error usually means that the feature "
  "in question is available on other hardware, firmware, or software releases and is intended "
  "to be released on this platform. Check the Website for updates to software and firmware."
},

{ERR_7199, XSEVERITY_ERROR, "Feature (%s) is not available on this hardware.", "name of feature",
  "The named feature is not available on this hardware. This error means that the feature cannot "
  "be implemented on the hardware and/or there are no plans to implement it."
},

{ERR_7200, XSEVERITY_ERROR, "Waited too long for reply from Keyhole Test program.", "",
  "A message send from the PC to the PPC over the keyhole port did not elicit a response from "
  "the PPC in a timely fashion. This could be caused by:\n"
  "  1. The current image in the Pico Card does not support the keyhole protocol.\n"
  "  2. The monitor program, or other program currently running on the Pico Card has locked up."
},

{ERR_7201, XSEVERITY_ERROR, "ELF file does not handle this file type.", "",
  "The file specified for loading did not have valid ELF file header. The file was not loaded "
  "by the program loader. The requirements for a valid ELD header are:\n"
  "  1. The file did not begin with the signature character sequence \'\\x7F\' \'ELF\'. \n"
  "  2. The file type in the header was not == 2 (Executable file).\n"
  "  3. The size of the ELF header (34 bytes) was incorrect.\n"
  "  4. The size of the program segment header (20 bytes) was incorrect.\n"
  "  5. The version number in the ELF header was not 1."
},

{ERR_7202, XSEVERITY_ERROR, "The file '%s' must be an elf file (.elf).", "file name",
  ""
},

{ERR_7203, XSEVERITY_ERROR, "Invalid header on ELF file.", "",
  ""
},

{ERR_7204, XSEVERITY_INFORMATION, "Process on PPC finished.", "",
  ""
},

{ERR_7205, XSEVERITY_ERROR, "Invalid Entry address in ELF file", "",
  "The Entry address specified in the ELF file is outside the range of RAM on the Pico Card. Check "
  "the following:\n"
  "  1. The ELF file is correct.\n"
  "  2. The base address specified for the file is in the range of RAM for this card.\n"
  "  3. The linker control file (nominally .ld file) specifies the proper RAM addresses."
},

{ERR_7206, XSEVERITY_INFORMATION, "PPC program has terminated (information).", "",
  ""
},

{ERR_7207, XSEVERITY_ERROR, "Flash ROM has been changed by another program.", "",
  ""
},

{ERR_7208, XSEVERITY_ERROR, "Pico Card has been rebooted.", "",
  "This is an event generated by Pico." DLL " to inform the calling program that reboot has completed."
},

{ERR_7209, XSEVERITY_INFORMATION, "Program %s started.", "program name",
  "This is an event generated by the Pico card to inform the calling program that the program "
  "has started."
},

{ERR_7210, XSEVERITY_ERROR, "PPC Timeout waiting for input from PC; input status=%s.", "fifo status",
  "No input was received by the PPC monitor program within the timeout interval. This can be overridden "
  "by sending a timeout command (in PicoUtil) with a larger timeout period, or zero to indicate "
  "no timeout."
},

{ERR_7211, XSEVERITY_ERROR, "No input received from PC; input fifo status=%s.", "fifo status",
  "The command field from the PC to the PPC monitor program was invalid. The first command field "
  "should always be type 0 (KH_BEGIN) with a small integer value in the subCommand field (such "
  "as 2 for version). The input stream is flushed up to the next command=15 (KH_END)."
},

{ERR_7212, XSEVERITY_ERROR, "PPC command (%s) to PC timed out.", "fifo status",
  "The command received from the PC by the monitor program running on the PPC did not report completion "
  "in a timely fashion. This usually means that the command received by the Pico Card is badly "
  "formed, ie. the length field is incorrect, or the message is not delimited by a start-of-message "
  "and end-of-message."
},

{ERR_7213, XSEVERITY_ERROR, "PPC Out of sequence. Received word count=%s", "message sequence number",
  "The commands received from the PC by the program KeyholeTest.elf running on the PPC are out "
  "of sequence."
},

{ERR_7214, XSEVERITY_WARNING, "Keyhole services limited (%s).", "reason",
  "PicoUtil will normally poll the keyhole interface and display data received from that port. "
  "This option has been disabled. The reason that keyhole polling has been disabled is specified "
  "in the message.\n\n"
  "NOTE: The keyhole interface will jam if the data is generated by the PPC on the Pico Card and "
  "not retrieved by the host. Specifying a keyhole protocol in PicoUtil.exe will not disable the "
  "generation of keyhole output from the Pico Card. This option is intended to keep PicoUtil from "
  "getting in the middle of the keyhole interface when a user program will be responsible for "
  "managing the keyhole."
},

{ERR_7215, XSEVERITY_ERROR, "Command to KeyholeTest.elf is incorrect. Command=%s", "hex of message",
  "The command should be cmd=0, packetType=2 (loopback). From this state you may need to reboot "
  "the image to force the program KeyholeTest.elf (of keyholetest.exe) out of memory."
},

{ERR_7216, XSEVERITY_ERROR, "Capability not found  '%s'.", "expected capability",
  "The firmware was not built with the capability to perform the task requested."
},

{ERR_7217, XSEVERITY_ERROR, "Error in Kprintf on PPC.", "",
  "The Kprintf has encountered an error."
},

{ERR_7218, XSEVERITY_ERROR, "Unable to locate file KeyholeTest.elf.", "",
  "The program KeyholeTest.elf is required for this operation. It was not found in the flash ROM "
  "directory. Write the file using \'menu/write file to Pico\'."
},

{ERR_7219, XSEVERITY_ERROR, "RAM test failed at location %s.", "address",
  "The test of RAM on the Pico Card failed at the specified address. "
},

{ERR_7220, XSEVERITY_ERROR, "RAM address is outside memory available on Pico Card.", "",
  "The request to read a RAM address is outside the range of RAM on the Pico Card."
},

{ERR_7221, XSEVERITY_ERROR, "File must be type BIT to be rebooted; found to be of type %s.", "actual file type",
  "The file specified to a LoadBitFile(char*) function must be a bit file."
},

{ERR_7222, XSEVERITY_ERROR, "File must be type ELF to be executed.", "",
  "The file specified to a LoadElfFile(char*) function must be an elf file."
},

{ERR_7223, XSEVERITY_WARNING, "File size rounded up to %s bytes (ie. multiple of physical sector size)", "",
  "When a file is allocated with pre-allocation the data part of the file will be aligned to a "
  "physical sector boundary. The size of the file is therefore increased to be an exact multiple "
  "of the physical sector size (131076 bytes)."
},

{ERR_7224, XSEVERITY_WARNING, "Error reading RAM", "",
  "The firmware did not return the correct number of words when reading the DDR2 RAM."
},

{ERR_7230, XSEVERITY_ERROR, "Unable to load linked file", "",
  "In standalone mode a bit image was unable to locate the linked file to loading. The message "
  "reported from monitor.elf is:\n"
  "     Err #7230: Unable to locate file.\n"
  "     curBitFile=..., linkedFile=... fileType=..., peekaboo=\n"
  "- peekaboo= is the last address accessed in "
  "the flashROM and should be within the scope of the currently loaded bit file.\n"
  "- curBitFile= is the handle of the currently loaded bit file. This information is derived from the peekaboo "
  "by scanning backwards until a file encompassing the peekaboo value is found.\n"
  "- linkedFile= is the handle of the linked file. This information is derived by examining the header of the "
  "currently loaded bit file and attempting to open the linked file.\n"
  "fileType= is the type of the currently loaded bit file or the linked file (depending upon which was successfully loaded)."
},

{ERR_7231, XSEVERITY_ERROR, "Unexpected exit from a loaded program", "",
  "In standalone mode a bit or elf file and returned. Theoretically this is impossible. It indicates "
  "that the bit file did not load correctly.\n The message reported from monitor.elf is:\n "
  "    Err #7231: Unexpected exit from loaded program.\n"
  "    peekaboo=0x...., curBitFile=..., linkedFile=... fileType=...\n"
  " -peekaboo= is the last address accessed in the flashROM and should be within "
  "the scope of the currently loaded bit file.\n"
  "- curBitFile= is the handle of the currently loaded "
  "bit file. This information is derived from the peekaboo by scanning backwards until a file "
  "encompassing the peekaboo value is found.\n"
  "- linkedFile= is the handle of the linked file. "
  "This information is derived by examining the header of the currently loaded bit file and attempting "
  "to open the linked file.\n"
  "- fileType= is the type of the currently loaded bit file or the "
  "linked file (depending upon which was successfully loaded)."
},

{ERR_7234, XSEVERITY_ERROR, "Linked file not found", "",
  "A linked file was not specified in the file header."
},

{ERR_7250, XSEVERITY_ERROR, "Unable to add another custom tool", "",
  "A maximum of twelve user tools can be handled. Delete a tool that you no longer require"
},

{ERR_7251, XSEVERITY_ERROR, "Command field in a custom tool does not refer to a valid program name '%s'", "file name",
  "The command field of a custom tool must be present. The field should refer to a program name "
  "including the suffix (.exe, .com, or .bat). The command must be found either in the system "
  "path (use path in cmd.exe) or the initial directory specified for the command."
},

{ERR_7252, XSEVERITY_ERROR, "directory specified by a custom tool or wizard is not valid '%s'", "directory name",
  "The initial directory specified for a custom tool is not valid, or the directory specified "
  "by a wizard is not valid."
},

{ERR_7253, XSEVERITY_ERROR, "Unable to find %s in path.", "file name",
  "The specified program was not found in the environment value specified by the path. The error "
  "will also occur if the path variable could not be obtained from the environment. In Windows, "
  "you can add the appropriate path from Control Panel / System / Advanced /Environment."
},

{ERR_7254, XSEVERITY_WARNING, "Should be exactly one build.bat file near specified binary file.", "",
  "In order to compile an ELF file, or build a bit file an appropriate .BAT file must be found "
  "near the target file.\n"
  "  \'Near\' means that the .BAT file must exist in the directory containing the binary file, "
  "or its parent.\n"
  "  \'Appropriate\' means that the .BAT file name contains the word build. \n When PicoUtil "
  "searched the binary file directory or its parent it found either no .BAT files or more than "
  "one .BAT file.\n An acceptable directory structure is:\n"
  "   c:\\directory\\bin\\FPGAfile.bit\n"
  "   c:\\directory\\buildImage.bat\n or\n"
  "   c:\\directory\\Debug\\program.elf\n"
  "   c:\\directory\\buildprogram.bat\n or\n"
  "   c:\\directory\\Debug\\program.elf\n"
  "   c:\\directory\\Debug\\ProgramBuild.bat\n This structure is created if the project was created "
  "using one of the standard Wizards in PicoUtil.\n The command cmd.exe /c \".\\build.bat\" is "
  "initiated to perform the compilation. You may refer to the directory \'generic_ppc_program\' "
  "for an example of build.bat file, or you could invoke the ppc program wizard to create a test "
  "application in which the file build.bat will be generated."
},

{ERR_7255, XSEVERITY_ERROR, "Invalid script command:%s", "command",
  "The script command specified in a custom tool is invalid. Valid tools are:\n"
  " wp      WritePort\n"
  " wd      WriteDevice\n"
  " wr      WriteRam\n"
  " rp      ReadPort\n"
  " rd      ReadDevice\n"
  " rr      ReadRam\n"
  " debug   Debug\n"
  " goto    Goto\n"
  " verbose \n"
  " <       InputFile\n"
  " >       Overwrite\n"
  " >>      AppendFile\n"
},

{ERR_7290, XSEVERITY_ERROR, "Unable to locate :%s", DLL " file name",
  "A supporting library file was not found. The library file should be located in the directory specified by the "
  "environment variable picobase (in cmd.exe enter \'echo %picobase%\')."
},

{ERR_7291, XSEVERITY_ERROR, "Unable to locate entry point '%s' in library module.", "module file name",
  "A supporting library file was found, but does not contain the expected entry point. This is most likely "
  "because the library file is not the proper version. You may use Explorer to check the actual version "
  "of the library file (.dll under Windows .so under Linux).\n\n"
  "NOTE: Names are CASE SENSITIVE."
},

{ERR_7292, XSEVERITY_ERROR, "Unacceptable version of '%s'", "bad version",
  "A supporting library was found, but reports the wrong version. You may use Explorer to  check the "
  "actual version of library file (.dll under Windows .so under Linux)."
},

{ERR_7293, XSEVERITY_ERROR, "OS error loading executable module '%s'.", "executable file name",
  "The call to LoadLibrary(\"pico." DLL "\") has failed, because a " DLL " file which pico.dll depends upon "
  "is not present. You may be able to isolate the offending DLL using the standard system utility "
  "depends.exe. However, there are a number of special cases that will confuse the unwary."
},

{ERR_7294, XSEVERITY_ERROR, "OS error loading DLL '%s'.", DLL " file name",
  "An error has occurred when loading the specified executable module.\n"
  "This error can occur because:\n"
  "1. The specified executable module cannot be loaded, or\n"
  "2. Some dependent module referenced by the executable module cannot be loaded.\n\n"
  "Under Windows the standard utility \'depends.exe\' will help you elucidate "
  "this chain of dependent DLL\'s."
},

{ERR_7295, XSEVERITY_ERROR, "Incorrect side-by-side directories for Visual Studio Express", DLL " file name",
 "The requested DLL needs a different set of side-by-side directories than the one installed.\n"
 "When the Pico installer is executed you have the option of installing the Visual Studio Express (VSE) files.\n"
 "This executes VCredist_X86.exe (from Microsoft) which should have correctly installed the VSE side-by-side files\n"
 "Either download and execute VCredist_X86.exe from microsoft.com or\n"
 "Re-run the Pico Installer and check the 'Visual Studio Express Files' box.\n"
 "NOTE: As of version 4.1.0.0 Pico.dll and PicoD.dll are no longer compiled under Visual Studio Express, "
 "to avoid precisely this nonsense."
},

{ERR_7296, XSEVERITY_ERROR, "Invalid file name '%s'", "file name",
  "The file name specified on the command line to sim.exe is invalid. It should end in \".sam\"."
},

{ERR_7297, XSEVERITY_ERROR, "Write Permission denied '%s'", "file name",
  "Reading and writing the primary boot file or the backup boot file is a privilege reserved to "
  "PicoUtil."
},

{ERR_7298, XSEVERITY_ERROR, "Unable to determine name of current image.", "",
  "This error will occur under several circumstances:\n"
  "  1. The CPLD does not return the address used when the firmware was loaded from the flash ROM.\n\n"
  "  2. The directory on the flash ROM is corrupted and the system is unable to determine which "
  "flash ROM file was used to load the FPGA.\n\n"
  "  3. The Pico Card was being loaded from JTAG. This situation is normal and is not an error."
},

{ERR_7299, XSEVERITY_ERROR, "Unacceptable serial number '%s'.", "serial number found",
  "The serial number is not acceptable. The serial number must be exactly 12 hex digits and the "
  "first must be 44 99 43 (the Ethernet MAC address)."
},

{ERR_7300, XSEVERITY_ERROR, "Bad parameter to constructor cPicoStream() '%s'.", "",
  "The keyword or keyword=value supplied as the second parameter to the constructor "
  "for cPicoStream is unacceptable."
},

{ERR_7301, XSEVERITY_ERROR, "Pico Failed Installation Write test.", "",
  "The attempt to write to the Pico during installation Card failed. This error is caused when "
  "the PCMCIA interface hardware is incompatible with the firmware on the Pico Card. Please record "
  "the type and model of the PCMCIA interface (Control Panel / System / Hardware / Device Manager "
  "under PCMCIA devices and report this error the Pico Computing."
},

{ERR_7302, XSEVERITY_ERROR, "Pico Failed Installation Read test.", "",
  "The attempt to read the Pico Card during installation failed. This error is caused when the "
  "PCMCIA interface hardware is incompatible with the firmware on the Pico Card. Please record "
  "the type and model of the PCMCIA interface (Control Panel / System / Hardware / Device Manager "
  "under PCMCIA devices and report this error the Pico Computing."
},

{ERR_7303, XSEVERITY_WARNING, "E-12 card is operating in hobbled mode.", "",
  "The Pico E-12 card failed the Address Extension Test which is fundamental to accessing SDRAM. "
  "It has defaulted to a much slower access mechanism using the configuration memory of the PCMCIA."
},

{ERR_7304, XSEVERITY_ERROR, "Not connected to a Pico Card.", "",
  ""
},

{ERR_7305, XSEVERITY_ERROR, "Please select exactly one card/file for this operation.", "",
  "The operation you have selected only applies to a single Pico card or Single file on the flash ROM\n"
  "Please select a card/file from the upper window and try the operation again"
},

{ERR_7306, XSEVERITY_ERROR, "Unable to find help file '%s'", "chm file name",
  "Couldn\'t find the help file. Either you didn\'t install the documentation, or the installation "
  "has become corrupted. Please run the installer again to install the documentation."
},

{ERR_7307, XSEVERITY_DANGEROUS, "DCMs failed to lock.", "",
  "The DCMs (Digital Clock Managers) on the Pico card have not locked-on to the clock as they "
  "need to. This means that parts of the firmware are not receiving a valid clock signal, and "
  "will not work. You may remove the card and reinsert it to get back to the primary FPGA program."
},

{ERR_7308, XSEVERITY_DANGEROUS, "Pico signature not found in firmware.", "",
  "This firmware does not contain the Pico signature that identifies all FPGA programs for Pico "
  "cards. The FPGA is either running an invalid program, or not running at all. You may remove "
  "the card and reinsert it to get back to the primary FPGA program."
},

{ERR_7309, XSEVERITY_ERROR, "Powerdown rejected because another application has the device driver open.", "",
  "The Device Driver Pico64.sys is busy serving another application. It cannot power down the card "
  "until the other application terminates."
},

{ERR_7310, XSEVERITY_ERROR, "Error powering down JTAG device.", "",
  "Pico.executable module was unable to power down the JTAG parallel port. This may be because a program such "
  "as Impact.exe or EDK is using the JTAG port. Terminate any other program and try the operation "
  "again. If you have difficulties using either the JTAG port or the Pico port after this error, "
  "check that the devices are still enabled in the hardware device managed (Control Panel / System "
  "/ Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7311, XSEVERITY_ERROR, "Error powering up JTAG device.", "",
  "Pico." DLL " was unable to power up the JTAG parallel port. This may be because a program such "
  "as Impact.exe or EDK is using the JTAG port. Terminate any other program and try the operation "
  "again. If you have difficulties using either the JTAG port or the Pico port after this error, "
  "check that the devices are still enabled in the hardware device managed (Control Panel / System "
  "/ Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7312, XSEVERITY_ERROR, "Error powering down Pico device .", "",
  "Pico." DLL " was unable to power down the Pico parallel port. This may be because a program such "
  "as Viva, or Picoutil is using the Pico port. Terminate any other program and try the operation "
  "again. If you have difficulties using either the JTAG port or the Pico port after this error, "
  "check that the devices are still enabled in the hardware device managed (Control Panel / System "
  "/ Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7313, XSEVERITY_ERROR, "Error powering up Pico device .", "",
  "Pico." DLL " was unable to power up the Pico parallel port. This may be because a program such "
  "as Impact.exe or EDK is using the JTAG port. Terminate any other program and try the operation "
  "again. If you have difficulties using either the JTAG port or the Pico port after this error, "
  "check that the devices are still enabled in the hardware device managed (Control Panel / System "
  "/ Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7314, XSEVERITY_ERROR, "Error powering down MF device .", "",
  "Pico." DLL " was unable to power down the Multi-function driver. This may be because a either the "
  "JTAG or Pico ports did not successfully power down. Terminate any other program and try the "
  "operation again. If you have difficulties using either the JTAG port or the Pico port after "
  "this error, check that the devices are still enabled in the hardware device managed (Control "
  "Panel / System / Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7315, XSEVERITY_ERROR, "Error powering up MF device .", "",
  "Pico." DLL " was unable to power up the Multi-function driver. This may be because a program such "
  "as Impact.exe or EDK is using the JTAG port. Terminate any other program and try the operation "
  "again. If you have difficulties using either the JTAG port or the Pico port after this error, "
  "check that the devices are still enabled in the hardware device managed (Control Panel / System "
  "/ Hardware / Device Manager tab / ports or / Pico Driver)."
},

{ERR_7316, XSEVERITY_ERROR, "Current FPGA file (%s) does not provide JTAG spy.", "FPGA file name",
  "The FPGA file loaded as the current image does not support the JTAG spy interface. Refer to "
  "menu / tool / capabilities for a summary of the capabilities of the current FPGA image.\n"
  "To correct this problem:\n"
  "1. Load an FPGA with JTAG spy capabilities\n"
  "2. Create an FPGA image using BuildProject.bat with option --jtagspy."
},

{ERR_7317, XSEVERITY_ERROR, "Attempt to write a read only Stream %s", "Stream number",
  "The specified Stream was determined to be writeOnly because the read Pacing register returned an invalid "
  "signature when the Stream was opened. The value that should have been returned in the read pacing register "
  "should have had the high six bits == 0x26, ie (value & 0xFC000000) == 0x98000000.\n"
  "In Picoutil you may use the menu command Debug / Stream Status to read the pacing registers"
 },

{ERR_7318, XSEVERITY_ERROR, "Attempt to read a write only Stream %s", "Stream number",
  "The specified Stream was determined to be readOnly because the write Pacing register returned an invalid "
  "signature when the Stream was opened. The value that should have been returned in the write pacing register "
  "should have had the high six bits == 0x22, ie (value & 0xFC000000) == 0x88000000.\n"
  "In Picoutil you may use the menu command Debug / Stream Status to read the pacing registers"
 },

{ERR_7319, XSEVERITY_WARNING, "Pico card is opened in read only mode '%s'", "read only mode",
  "The Device Driver Pico64.sys has been opened in read/write mode by another application. The current "
  "application can only open the card in readonly mode. To correct this problem, terminate the "
  "other application which has the Pico Card open. If the application is running against a simulation "
  "file, this error can occur when more than one application has the file open."
},

{ERR_7320, XSEVERITY_ERROR, "Pico card or simulation file is opened in read only mode.", "",
  "The Pico Card was opened in read only mode and cannot therefore write a file to the flash ROM. "
  "Programs such as viva.exe or chipper.exe expect to be able to write FPGA files to the flash "
  "ROM. In the frequent event that the files are already written to the flash ROM and are identical "
  "this misdemeanor will be overlooked. However, if the file is not present or different from "
  "the PC file this error will be generated.\n"
  "A Pico Card will be opened in read only mode when another user of the card has it opened in "
  "read/write mode. Exit the offending program and try again."
},

{ERR_7321, XSEVERITY_ERROR, "Timeout waiting for Stream interrupt.", "",
  "The call to Stream.WaitForInterrupt() has not received an interrupt within the time specified "
  "when it was called.\n"
  "The interrupt must be generated by the user supplied firmware, by raising the interrupt line "
  "in the Picobus or PicoStream interface. At the time the interrupt line is raised, the value "
  "returned by the WaitForInterrupt() routine should be made available in the INTERRUPT_STATUS_REG."
},

{ERR_7322, XSEVERITY_ERROR, "Error Creating interrupt event.", "",
  "The call to the operating system channel to create an event handle for user interrupts failed."
},

{ERR_7330, XSEVERITY_ERROR, "Bus Master interface not ready.", "",
  "A Bus Master operation was initiated from the but did not report \'done\' in a timely fashion."
},

{ERR_7331, XSEVERITY_ERROR, "Bus Master counter test register did not increment properly.", "",
  "The device BMtestCounter.v is included in the standard build to facilitate testing the Bus "
  "Mastering logic. A read single word (32bit on the E14, 32bit on the E12) from this counter "
  "should increment the register. However, the logic failed this test."
},

{ERR_7332, XSEVERITY_ERROR, "Bus Master Read or Write to counter register failed.", "",
  "The device BMtestCounter.v is included in the standard build to facilitate testing the Bus "
  "Mastering logic. A ReadFile or WriteFile operation did not report the correct number of bytes "
  "transferred."
},

{ERR_7333, XSEVERITY_WARNING, "Interrupts are not operational. System is dropping back to non interrupt mode.", "",
  "The interrupts from the Bus Mastering logic have not caused an interrupt in the operating system. "
  "The operating mode of the Bus Mastering logic has been shifted to polled mode. Polled mode "
  "is usually considerably slower than interrupt mode. Please report the type of Cardbus controller "
  "(use Control panel / System / Device manager) to Pico Computing."
},

{ERR_7334, XSEVERITY_ERROR, "Data error in Bus Master counter test.", "",
  "The device BMtestCounter.v is included in the standard build to facilitate testing the Bus "
  "Mastering logic. Data read from or written to this register has not had the values expected. "
  "For example, repeated reads of this register should increment the register. A write to this "
  "register should reflect the written value."
},

{ERR_7335, XSEVERITY_ERROR, "Bus Master pacing register did not return proper signature.", "",
  "The device readStatus or writeStatus registers did not return values with the proper signature. "
  "The status registers for a device must implement a status register which returns the following "
  "32bit structure:\n"
  "   struct {UINT32 signature:6,\n"
  "                  nu       :6,\n"
  "                  available:20;\n"
  "          }\n"
  "The signature for the read status register must be 0x26, and the write status register "
  "must be 0x22. Refer to the manual picodll.chm or picodll.pdf for details on implementing DMA."
},

{ERR_7336, XSEVERITY_ERROR, "Bus Master timeout test did not return the correct number of bytes.", "",
  "The device BMtestCounter.v is included in the standard build to facilitate testing the Bus "
  "Mastering logic. This error occurs when this device is being tested for conformance with the "
  "timeout specifications. The device should return something less than the requested number of "
  "bytes because the operation timed-out."
},

{ERR_7337, XSEVERITY_ERROR, "Bus Master interrupt not specified.", "",
  "The bus mastering logic requests an interrupt from the operating system when the Pico Card "
  "is inserted. This error occurs when no such interrupt has been assigned. You may inspect the "
  "resources assigned to the Pico card from Control Panel / System / hardware / device manager"
},

{ERR_7338, XSEVERITY_ERROR, "Interrupts not received at the expected rate.", "",
  "The stream should interrupt on each I/O completed. This error indicates that not enough interrupts "
  "have been received during this test."
},

{ERR_7339, XSEVERITY_ERROR, "Bus Master did not timeout correctly.", "",
  "The bus mastering software in Pico64.sys did not timeout the operation as expected."
},

{ERR_7340, XSEVERITY_ERROR, "Unable to determine number of JTAG devices or number of devices is incorrect.", "",
  "The JTAG interface built into the standard FPGA image was unable to determine how many JTAG "
  "devices were on the internal JTAG chain of the Pico Card. The most likely cause of this error "
  "is that the external JTAG connector is plugged into the Pico Card. When the external cable "
  "is plugged into the card the internal JTAG connector is disabled. Either:\n"
  "  1. Unplug the JTAG connector,\n"
  "  2. Use software such as Impact from Xilinx to obtain the information you need. Pico Computing "
  "does not provide a JTAG interface to the external (parallel port) cable."
},

{ERR_7341, XSEVERITY_ERROR, "Invalid scan location provided to the function PicoMface::JtagGetIdCode().", "",
  "The number of devices on the internal JTAG cable of the Pico Card is three. The value passed "
  "as the first parameter to this function is not in the range 1-3. The most likely cause of this "
  "error is that the external JTAG connector is plugged into the Pico Card. When the external "
  "cable is plugged into the card the internal JTAG connector is disabled. Either:\n"
  "  1. Unplug the JTAG connector,\n"
  "  2. Use software such as Impact from Xilinx to obtain the information you need. Pico Computing "
  "does not provide a JTAG interface to the external (parallel port) cable."
},

{ERR_7342, XSEVERITY_ERROR, "Incorrect ID code returned from device %s on JTAG chain.", "device Name",
  "The acceptable ID codes for the devices on the JTAG chain are limited to the devices built "
  "onto the Pico Card. these are:\n"
  "   FX12 = 0x01E58093\n"
  "   LX12 = 0x9167C093\n"
  "   FX20 = 0x01E64093 or 0xD1E64093\n"
  "   FX60 = 0x01EB4093 or 0xD1EB4093\n"
  "   FX70T= 0x037C6093 or 0xD1EB4093\n"
  "   CPLD = 0x06E5D093\n"
  "   Ethernet = 0x000003D3\n The most likely cause of this error is that the external JTAG connector "
  "is plugged into the Pico Card. When the external cable is plugged into the card the internal "
  "JTAG connector is disabled. Either:\n"
  "  1. Unplug the JTAG connector,\n"
  "  2. Use software such as Impact from Xilinx to obtain the information you need. Pico Computing "
  "does not provide a JTAG interface to the external (parallel port) cable."
},

{ERR_7343, XSEVERITY_ERROR, "Too many open channels.", "",
  "This is an error from the operating system occasioned when too many C-runtime file handles "
  "are opened. This occurs when Pico Streams are opened but not closed.\n"
  "If you are using direct file calls (read and write) to access a pico Stream use the _close() "
  "function to close unnecessary Streams (handles).\n"
  "If you are using the class cPicoStream, delete the Stream."
},

{ERR_7344, XSEVERITY_ERROR, "Unable to open Stream.", "reason",
  "The system was unable to open a stream to access a Pico Card.\n"
  "This may be because the requirements specified in the cPicoStream constructor "
  "were too restrictive, or because there are in fact no free cards available."
},

{ERR_7345, XSEVERITY_ERROR, "Unable to set Stream number '%s'.", "",
 "In the constructor of the cPicoStream class, the Stream number specified is invalid "
 "(ie outside the range 1-1791)."
},

{ERR_7346, XSEVERITY_WARNING, "Unable to set base address.", "",
 "In the constructor of the cPicoStream class, the Stream conflicts with a Stream that is already in use.\n\n"
 "The most likely cause of this warning is an attempt to run the same program against the same Pico Card.\n"
 "You should shut down the program you no longer require.\n"
 "Note: If you set the size of the Stream larger than 0x100000 it will occupy multiple "
 "Stream 'slots' and may provoke this error unexpectedly, for example:\n"
 "    ch1P = new cPicoStream(10, \"StreamSize=0x200000\")\n"
 "    ch2P = new cPicoStream(11)\n"
 "will provoke this error because Stream 10 'spills over' into Stream 11."
},

{ERR_7347, XSEVERITY_ERROR, "Unable to set Stream size.", "",
 "In the constructor of the cPicoStream class, the Stream conflicts with a Stream that is already in use.\n"
 "Note: If you set the size of the Stream larger than 0x100000 it will occupy multiple "
 "Stream 'slots' and may provoke this error unexpectedly, for example:\n"
 "    ch1P = new cPicoStream(10, \"size=0x200000\")\n"
 "    ch2P = new cPicoStream(11)\n"
 "will provoke this error."
},

{ERR_7350, XSEVERITY_WARNING, "Board is unserialized, or has an invalid serial number.", "",
  "The serial number is written to the Pico Card at the last step of product manufacture, after "
  "it has passed all manufacturing tests. The card does not have a serial number, indicating that "
  "it has not been tested. The secure sector also contains the model information for the FPGA "
  "on the Pico Card and the serial number also embeds the IEEE Ethernet number."
},

{ERR_7351, XSEVERITY_ERROR, "Board does not have model number information in secure sector.", "",
  "The serial number is written to the Pico Card at the last step of product manufacture, after "
  "it has passed all manufacturing tests. The card does not have a serial number, indicating that "
  "it has not been tested. The secure sector also contains the model information for the FPGA "
  "on the Pico Card and the serial number also embeds the IEEE Ethernet number."
},

{ERR_7352, XSEVERITY_ERROR, "Serial number cannot be written without a valid primary image .", "",
  "The FPGA model number is written when the serial number is applied to the board. This information "
  "is obtained from the PrimaryBoot.Bit file. The PrimaryBoot.bit file could not be found or the "
  "model information was not found within this file."
},

{ERR_7353, XSEVERITY_ERROR, "FPGA file is not a valid bit file or is not compatible with the FPGA (%s) on the Pico Card.", "FPGA model",
  "The FPGA model requires an FPGA file exactly tailored to the part. To correct this problem:\n"
  "\n"
  "  1. Replace the Pico Card with one of the proper model.\n"
  "or\n"
  "  2. Change the setting in EDK or ISE to the proper part number.\n"
  "or\n"
  "  3. Change the pc file name to something other than .bit and write this file to the flash "
  "under this name. However, the changed file will be written as \'data file\' and will not be "
  "allocated contiguously. Contiguous allocation is required to load the FPGA image at startup.\n "
  "or\n"
  "  4. Write this file to the flash system under the name file.bit, using PicoCommand, command "
  "line: PicoCommand /wo pcfile.bit\n From PicoUtil, the content of the file can be examined "
  "using right click / display first sector of file."
},

{ERR_7354, XSEVERITY_ERROR, "Unable to read the part number from the JTAG port.", "",
  "This error will occur if the JTAG port is unable to read the part number information from the "
  "Pico Card. This could arise for two reasons:\n"
  "  1. The external JTAG cable is plugged into the card.\n"
  "  2. The firmware loaded onto the Pico Card does not support the JTAG interface.\n This error "
  "may also occur when the serial number is read from the Pico Card, and a valid part number is "
  "not found in the secure sector of the card. In this case the JTAG interface will be interrogated "
  "for the part number."
},

{ERR_7355, XSEVERITY_ERROR, "data in secure sector conflicts with current data and cannot be updated.", "",
  "The serial number or part number current in the secure sector of the FPGA conflicts with the "
  "data being written. The write was aborted."
},

{ERR_7356, XSEVERITY_ERROR, "data did not update secure sector correctly.", "",
  "The verify read back of the secure sector did not compare with the data written."
},

{ERR_7357, XSEVERITY_WARNING, "Unable to determine FPGA model. Action Performed notwithstanding.", "",
  "The FPGA model requires an FPGA file exactly tailored to the part. Pico." DLL " was unable to determine "
  "the model of the FPGA. The action requested (file write or reboot) will be performed regardless "
  "of the potential incompatibility. Possible causes for this error are:\n"
  "1. A valid serial number (which includes the FPAG model number) was not written to the Pico "
  "Card.\n"
  "or\n"
  "2. Pico " DLL " was unable to determine the model from the JTAG port, possible because the external "
  "JTAG port is connected.\n"
  "     Remove the external JTAG connection to obviate this difficulty."
},

{ERR_7358, XSEVERITY_ERROR, "Unknown variable name:%s", "variable name",
  "The specified variable is not known in picocommand /a command."
},

{ERR_7359, XSEVERITY_ERROR, "Register $0 invalid in this context", "",
  "Register $0 cannot be used in this context."
},

{ERR_7380, XSEVERITY_HINT, "Clearing RAM before initiating a program will make dump considerably faster", "",
  "The dump RAM operation is optimized for long runs of zero in the Pico Card RAM. You may find "
  "it advantageous to execute menu / Debugging / Clear RAM before the operation you just performed. "
  "For example, if you wish to compare RAM before and after some operation is performed, clearing "
  "memory before either operation will make the dump MEM operation much quicker."
},
#ifdef WIN32
{ERR_7381, XSEVERITY_WARNING, "Debug switches are not implemented in release version of Pico64.sys", "",
  "Debug switches will not generate output in the release version of Pico64.sys.\n"
  "Copy %PICOBASE%\\bin\\picoD.sys to %WINDIR%\\system32\\drivers\\Pico64.sys,\n"
  "Run a debugging tool such as DbgView.exe from www.sysinternals.com to view the output,"
  "Eject/insert the Pico Card to reload the debugging version of Pico64.sys.\n"
},
#else
{ERR_7381, XSEVERITY_WARNING, "Debug switches are not implemented in release version of Pico.ko", "",
},
#endif
{ERR_7400, XSEVERITY_ERROR, "Directory name is invalid or too short (three characters) '%s'", "directory name",
  ""
},

{ERR_7401, XSEVERITY_ERROR, "Unable to find or create specified directory '%s'", "directory name",
  ""
},

{ERR_7402, XSEVERITY_ERROR, "Non empty directory already exists '%s'", "directory name",
  "The directory specified exists and there are already files in it. You must specify a nonexistent "
  "or empty directory into which a new project is written by the wizard."
},

{ERR_7403, XSEVERITY_ERROR, "Type field must be three or four characters in length '%s'", "file name",
  "The type field is typically a file extension. This is either three or four characters long "
  "(such as .exe, .txt, etc). The type field of files on the flash ROM system follows the same "
  "general style."
},

{ERR_7404, XSEVERITY_ERROR, "Invalid command in WizardCommand.txt '%s'", "cmd name",
  "The command fields in the file WIzardCOmmands.txt must be one of: message, copy, xcopy, startup, "
  "parameter, or startup."
},

{ERR_7449, XSEVERITY_ERROR, "%s command does not apply to this Pico Card", "command",
  "The command specified to picocommand.exe does not apply to the current FPGA.\n"
  "For example, any command related to the flashROM cannot be applied to Pico Cards "
  "which do not have flash ROM."
},
{ERR_7450, XSEVERITY_ERROR, "Error in PLX chip interface, function=%s.", "function name",
  "An Error has occurred accessing the PLX chip in the function specified."
},

{ERR_7451, XSEVERITY_ERROR, "DONE did not go high after sending configuration bitstream", "",
  "DONE is expected to go to a high state, signaling that the FPGA accepted the data from the "
  "bit file. This did not occur. This condition will cause the software to retry the operation "
  "several times and is reported only when the problem is not corrected after these retries."
},

{ERR_7452, XSEVERITY_WARNING, "FPGA is not correctly loaded.", "",
  "The FPGA card must have the FPGA loaded from a valid bit file.\n"
  "This error occurs on Pico Cards that do not have a flash ROM.\n"
  "These Pico Cards must be loaded from the host processor, for example: "
  "From PicoUtil this can be achieved using menu/Startup/Load FPGA.\n"
  "or\n"
  "From PicoCommand / PicoCmd using the /b option."
},

{ERR_7453, XSEVERITY_ERROR, "INIT didn't go inactive when PROG was driven low", "",
  ""
},

{ERR_7454, XSEVERITY_ERROR, "INIT didn't go high after PROG was driven high", "",
  ""
},

{ERR_7455, XSEVERITY_ERROR, "gpio state unexpected:%s", "hex of gpio",
  ""
},

{ERR_7456, XSEVERITY_ERROR, "Error from PlxPci.sys:%s", "hex of errorCode",
  ""
},

{ERR_7457, XSEVERITY_ERROR, "PLX_CHANNEL0_DONE not asserted when writing FPGA load information:%s", "hex of GPIO reg",
  ""
},

{ERR_7458, XSEVERITY_CATASTROPHIC, "Pico64.sys was unable to locate the PCI bridge.", "",
#ifdef WIN32
  "The PCI - PCIe bridge interfaces between the PCI bus of your computer and the PCIe bus used "
  "by the Pico E-16 card. Information about this bridge can be located in the system devices tree "
  "of the device manager \n (right click My computer / properties / Hardware / Device manager, "
  "\n"
  "then\n"
  "select system devices and page down to the PCI Standard PCI-PCI bridge. The bridge "
  "pointing to vendor=0x10B5, device=0x8111 is the one of interest). \n\n"
  "There are certain registers in this bridge device that must be manipulated in order to load "
  "the E-16 FPGA. Unfortunately, Pico64.sys was unable to locate this bridge device the massage "
  "these registers.\n\n"
  "You may try powering down your machine completely and restarting it. However, the problem will "
  "likely persist. Please report this problem to Pico Computing. It would be particularly interesting "
  "to us to know the following information from the device manager:\n\n"
  "1. The general, detail, and resource information for the Pico E-16 card (9056 chip).\n"
  "2. The general, detail, and resource information for the PCI Standard PCI-PCI bridge (8111 chip)."
#else
  "The PCI - PCIe bridge interfaces between the PCI bus of your computer and the PCIe bus used "
  "by the Pico E-16 card. Information about this bridge can be displayed using lspci.\n"
  "The only know solution to this problem is to power down the system and restart."
#endif
},

{ERR_7459, XSEVERITY_ERROR, "Error Accessing EEprom on %s", "E16 card",
  "An error occurred trying to read or write the eeProm on the specified card. The software automatically "
  "retries this operation, so the likelihood that manually retrying the operation successfully "
  "is slim. However, that is you only chance."
},

{ERR_7460, XSEVERITY_ERROR, "Error Loading FPGA", "",
  "An error occurred trying to load the FPGA from the specified file."
},

{ERR_7461, XSEVERITY_ERROR, "Error Loading FPGA after several retries", "",
  "An error occurred trying to load the FPGA from the specified file after several retries. "
  "Verify that the bit file being loaded is correct."
},
//Chipper Errors
{ERR_7500, XSEVERITY_ERROR, "No compatible pico card found!", "",
  "Pico Card(s) installed in the system must be Pico-LX25 (aka Pico-LO), Pico-E14, or Pico-E16 "
  "in order to be used by chipper.exe."
},

{ERR_7501, XSEVERITY_ERROR, "Bluetooth maximum pin length exceeded", "",
  "User defined maximum pin length is too small. Please enter a larger value. If no value is defined, "
  "the Bluetooth default pin length is the absolute maximum of 16 digits."
},

{ERR_7502, XSEVERITY_ERROR, "Missing imported information file", "",
  "An information file must be imported in order to perform a crack. Please select valid information "
  "file to import."
},

{ERR_7503, XSEVERITY_ERROR, "Missing user defined information addresses", "",
  "Bluetooth pincrack requires user defined values for m_bd_addr and s_bd_addr. Please define "
  "these addresses."
},

{ERR_7504, XSEVERITY_ERROR, "No Pico Card found meeting specified criteria %s", "param string to cPicoStream()",
  "No Pico Card was found meeting the criteria specified when the Stream was created.\n"
  "These criteria include model=..., ipAdr=..., PicoCardNum=..., serial=..., etc.\n"
  "This error could be arise from a number of causes:\n"
  "1. The Pico Card is not inserted into the PCIe slot.\n"
  "2. The Pico Card was not recognized by the system. This too can arise from a number of causes. "
  "however, these are all usually corrected by (re)installing the software.\n"
  "3. If the criteria is file=<bitfileName>, verify that the bit file corresponds to the "
  "type of FPGA on your Pico Card.\n"
  "4. If the Pico Card you are addressing is on a remote machine, make sure that the "
  "machine is visible over the TCP/IP network, and that the target machine is running "
  "picoDaemon, or picoChannelServer.exe.\n"
  "If you get this error running Chipper, please insert a Pico Card and restart Chipper."
},

{ERR_7505, XSEVERITY_ERROR, "File is not a valid .zip file", "",
  "Please ensure the selected file is a valid .zip file in order to perform a winzip crack."
},

{ERR_7506, XSEVERITY_ERROR, "unable to find encrypted files in (%s).zip!", "file name",
  "There may not be any encrypted files in this .zip file."
},

{ERR_7507, XSEVERITY_ERROR, "Unable to find password.", "",
  "The password for this encrypted .zip file does not exist in the selected dictionary file."
},

{ERR_7508, XSEVERITY_ERROR, "No dictionary selected.", "",
  "Please import and select a dictionary file in order to perform this crack."
},

{ERR_7509, XSEVERITY_ERROR, "Unable to identify the PSK from the dictionary file.", "",
  "Try expanding your passphrase list and double-check the SSID. Sorry it didn\'t work out."
},

{ERR_7510, XSEVERITY_ERROR, "Encrypted file not selected.", "",
  "Please select an encrypted file in order to do a winzip crack. If there are no filenames under "
  "encrypted files, there are no encrypted files in the imported folder."
},

{ERR_7511, XSEVERITY_ERROR, "Specified SSID and the SSID in the output file do not match.", "",
  "Create a new file, or change SSID to match."
},

{ERR_7512, XSEVERITY_ERROR, "Unable to open capture file.", "",
  "File may not be a valid capture file or it may not exist."
},

{ERR_7513, XSEVERITY_ERROR, "End of pcap capture file.", "",
  "Incomplete TKIP four-way exchange.  Try using a different capture."
},

{ERR_7514, XSEVERITY_ERROR, "Invalid word length.", "",
  "Problem in CoWPAtty nexthashrec."
},

{ERR_7515, XSEVERITY_ERROR, "Found a record that was too short.", "",
  "This shouldn\'t happen in practice! Error in CoWPAtty hashfile_attack.\""
},

{ERR_7516, XSEVERITY_WARNING, "Invalid passphrase length.", "",
  "CoWPAtty passphrase must be between 8 and 63 characters."
},

{ERR_7517, XSEVERITY_WARNING, "No BSSID selected.", "",
  "Please select a BSSID file in the BSSID box. If there are no BSSIDs in the box, "
  "please import another pcap file."
},

{ERR_7518, XSEVERITY_WARNING, "Please select only one dictionary name to change.", "",
  "Dictionary names are changed by double clicking on the name. It is only possible "
  "to change one dictionary name at a time. Please ensure that only one dictionary "
  "name is selected."
},

{ERR_7519, XSEVERITY_WARNING, "No PicoCard found. Using PC for cracks.", "",
   "There is no valid PiocCard in this system. In order to use a PicoCard in Chipper, "
   "please insert a PicoCard and restart Chipper."
},

{ERR_7520, XSEVERITY_ERROR, "Cannot connect to internet. Email notification cannot be sent.", "",
   "There is a problem with your internet connection. Please ensure that you are connected to "
   "the internet. In order to send email notifications, there must be a valid internet connection."
},

{ERR_7521, XSEVERITY_ERROR, "SMTP error.", "",
   "Unable to send email through specified SMTP server. Please designate a valid SMTP server."
},

{ERR_7522, XSEVERITY_ERROR, "Cannot run more than one of the same kind of crack at a time.", "",
  "Chipper is unable to perform more than one crack of each algorithm at a time."
},

{ERR_7523, XSEVERITY_ERROR, "A crack is already running in a selected row.", "",
  "There is already a crack running on one of the cards selected. Check the server."
},

{ERR_7524, XSEVERITY_ERROR, "Pico Card must be E16 or EX300.", "",
  "When running Chipper on a Linux based machine, the only supported Pico Cards are the E16 or EX300. "
  "Please ensure that the Pico Cards are valid E16 or EX300's."
},

{ERR_7525, XSEVERITY_WARNING, "This algorithm is not supported on the E101 yet.", "",
   "The Pico E101 only supports Bluetooth cracks at this time. "
   "Sometime in the near future, it will support all cracks."
},

{ERR_7526, XSEVERITY_ERROR, "No Pico Cards found", "",
   "No pico cards were found attached to this machine.\n"
   "This error could arise for a number of reasons:\n"
   "1. There are in fact no cards in this machine !\n"
   " You can explore this issue further by opening the device manager "
   " (control panel / system / device manage). You should see the pico "
   " cards under 'PicoComputing Drivers'.\n"
   " Under Linux you can enter 'ls /dev/pico*' to see which cards are available.\n"
   "2. The driver pico64.sys or pico64.sys is not loaded. You should verify"
   " that the driver was installed correctly (see install.pdf or install.chm)"
   " for more information. If necessary uninstall the driver from the device"
   " manager and reinstall.\n"
   "3. A pico card with flash ROM (E-12, E-14, E-15, E-17, E-18, M501, or M503)"
   " did not load from flash ROM correctly at power up. This will not load the"
   " PCIe interface and the operating system will be unable to 'see the card'.\n"
   "Try ejecting the card and reinserting. If the problem persists you may have to"
   " rewrite the primary boot file on the flash ROM."
},

{ERR_7600, XSEVERITY_ERROR, "DMA failed to complete in timely fashion", "",
  "After waiting the prescribed amount of time a DMA operation failed to complete."
},

{ERR_7601, XSEVERITY_ERROR, "ReadPLX8111Regs failed to read", "",
  "ReadPLX8111Regs failed to read."
},

{ERR_7602, XSEVERITY_ERROR, "GetPicoConfig failed to do ioctl", "",
  "GetPicoConfig failed to do ioctl."
},

{ERR_8000, XSEVERITY_ERROR, "ReadPLX8111Regs failed to read", "",
  "ReadPLX8111Regs failed to read."
},

{ERR_8001, XSEVERITY_ERROR, "ReadPLX8111Regs failed to read", "",
  "ReadPLX8111Regs failed to read."
},

{ERR_8002, XSEVERITY_ERROR, "GetPicoConfig failed to do ioctl", "",
  "GetPicoConfig failed to do ioctl."
},

{ERR_8003, XSEVERITY_ERROR, "FindPico didn't find any matching cards", "",
  "FindPico didn't find any matching cards. Are you looking for the right kind of card ? "
  "Has your computer detected the card ?"
},

{ERR_8004, XSEVERITY_ERROR, "Bitfile %s could not be opened", "bitFileName",
   "The bitfile could not be opened for reading.\n\n"
   "NOTE: On some platforms you have the option of booting from either a host file or the flash ROM.\n"
   "In these cases the syntax flash:bitFileName refers to a flash ROM file.\n"
   "Otherwise the name is presumed to refer to a host file. "
},

{ERR_8005, XSEVERITY_ERROR, "Bitfile %s could not be read", "bitFileName",
   "The bitfile specified by 'file=bitFileName' in the second parameter to cPicoStream() "
   "could not be read, or the file is too small to be a bit file."
},

{ERR_8006, XSEVERITY_ERROR, "FPGA download of %s to E101 completed with errors", "filename",
  "FPGA download to E101 over USB port of specified file did not run to completion."
},

{ERR_8007, XSEVERITY_ERROR, "BitFileCompat: PICO_CONFIG argument is NULL", "",
  "BitFileCompat: PICO_CONFIG argument is NULL"
},

{ERR_8008, XSEVERITY_ERROR, "Unable to open cPicoStream with the specified properties '%s'", "propertyName",
  "No Pico Card with the restrictions specified in the second (ascii) parameter to cPicoStream() "
  "could be found. If a fileName is specified the cPicoStream must match the type of FPGA for which "
  "the bit file was built.\n"
  "You can enumerate the Pico cards available as follows:\n"
  " 1. In PicoCommand using the /y command\n"
  " 2. In you C++ program using the (static) function AvailablePicoCards(). "
  "You can further explore the properties of those cards using the GetPicoConfig() function for "
  "each card thus enumerated."
},

{ERR_8009, XSEVERITY_ERROR, "Bitfile could not be opened", "bitFileName",
   "The bitfile specified by 'file=bitFileName' in the second parameter to cPicoStream() "
   "could not be opened for reading."
},

{ERR_8010, XSEVERITY_ERROR, "undeclared identifier: %s", "", "undeclared identifier"},

{ERR_8011, XSEVERITY_ERROR, "Driver doesn't support slot queries. Unable to determine the Pico's slot.", "",
  ""
},
{ERR_8012, XSEVERITY_ERROR, "Insufficient buffer space provided for slot string. Unable to report the Pico's slot.", "",
  ""
},
{ERR_8013, XSEVERITY_ERROR, "FPGA and .bit are incompatible %s", "file name",
  "The FPGA type that this .bit file is for was not recognized. Are you sure this .bit file is for a Pico card?"
},
{ERR_8014, XSEVERITY_ERROR, "All compatible Pico cards are in use.", "",
  "At least one compatible Pico card was found, but it's being used by another program."
  " Please close the program using one of the cards, or order more cards."
},
{ERR_8015, XSEVERITY_ERROR, "BitFileFPGAType doesn't have enough buffer space to handle this .bit file '%s'.", ".bit file name",
  "BitFileFPGAType doesn't have enough buffer space to handle this .bit file."
  " This could be due to an invalid .bit file, a change in the .bit file format, or an impossibly long filename."
},
{ERR_8016, XSEVERITY_ERROR, "Specified UserClk doesn't exist.", "",
  "The UserClk index you specified doesn't exist. Are you running the correct .bit file?"
},
{ERR_8017, XSEVERITY_ERROR, "Can't generate requested frequency.", "",
  "The clock generator (eg DCM, PLL, etc) can't produce the frequency you specified."
  " Please check the FPGA documentation for a list of the possible frequency range."
},
{ERR_8018, XSEVERITY_ERROR, "Invalid DCM multiply or divide parameter.", "",
  "This is an internal error. It means the Pico code has inconsistent information about the DCM."
  " Please report this error to Pico."
},
{ERR_8019, XSEVERITY_ERROR, "DCM failed to lock at requested clock speed.", "",
  "The DCM clock manager wasn't able to produce the requested clock speed. Have you requested a frequency that"
  " is supported by the speed grade of FPGA you have? You can find these numbers in the Xilinx 'Switching Characteristics'"
  " datasheet for your FPGA. Another possible cause of this failure is an overheated FPGA."
},
{ERR_8020, XSEVERITY_ERROR, "Couldn't read from .bit file '%s'", "file name",
  "The .bit file was opened, but couldn't be read from."
},
{ERR_8021, XSEVERITY_ERROR, "The .bit file is invalid '%s'", "file name",
  "The .bit file is invalid. The initial sync sequence (AA995566) isn't preceded by the width-detection sequence and padding."
},
{ERR_8022, XSEVERITY_ERROR, "The .bit file is invalid '%s'", "file name",
  "The .bit file is invalid. It doesn't contain the required initial sync sequence (AA995566)."
},

{ERR_8023, XSEVERITY_ERROR, "Internal error: driver BYTES_AVAILABLE called on firmware too old to support it!", "",
  ""
},

{ERR_8024, XSEVERITY_ERROR, "Can't call GetBytesAvailable on a stream that's not open", "",
  ""
},

{ERR_9000, XSEVERITY_SYSTEM, "CYapi.lib not available for 64bit platforms", "",
  "E101: CYapi.lib and driver are available for 64bit platforms.\n"
  "You cannot revert to a 32bit version of the program you are running because "
  "the driver for the USB port is not available from Cypress."
},

{ERR_9001, XSEVERITY_ERROR, "Failed to send initial message from host to E101", "",
  "E101: PICO_CONFIG argument is NULL"
},

{ERR_9002,  XSEVERITY_ERROR, "File %s did not pass syntax check", "filename",
  "Hex file used to load USB controller has errors."
},

{ERR_9003, XSEVERITY_ERROR, "Miscommunication between CyAPI.lib and cPicoStream_e101", "",
  "Miscommunication between CyAPI.lib and pico_drv_e101 firmware."
},

{ERR_9004, XSEVERITY_ERROR, "Failure to USB re-enumerate correctly", "",
  "Failure to remunerate USB correctly."
},

{ERR_9005, XSEVERITY_ERROR, "Could not establish a valid endpoint loading E101", "",
  "Could not establish a valid endpoint loading E101.\n"
  "This error signals a failure of the USB interface.\n"
  "Disconnect the E1010 board and re-connect."
},

{ERR_9006, XSEVERITY_ERROR, "Endpoint is invalid", "",
  "At least one USB Endpoint is invalid. This is usually because the E101 has been unplugged."
},

{ERR_9007, XSEVERITY_ERROR, "Unable to find '%s' in environment", "sambase",
  "environment variable %s was not found in sysem environment.\n"
},

{ERR_9008, XSEVERITY_ERROR, "FPGA download completed with errors in %s", "module name",
  "FPGA download completed with errors in specified module. The FPGA appeared to load "
  "however, the DONE pin was not asserted as would be expected. \n"
  "If this problem persists, cycle the power on  the FPGA card."
},

{ERR_9009, XSEVERITY_ERROR, "Invalid software call", "",
  "The classes cPicoStream_xxx may not be instantiated directly.\n"
  "The proper call sequence is 'new cPicoStream(n, model=...'\n"
  "The second string parameter contains selection criteria for the Pico Card and "
  "other supporting parameters. For example:\n"
  " cPicoStream StreamOne(1, \"model=E17, file=E17-primaryboot.bit)\n"
  "If you do not specify a model or select the appropriate Pico Card in some other way "
  "the new CPicoStream call will select the first Pico Card available.\n"
  "This is change was instituted in version 6.0.0.0."
},

{ERR_9010, XSEVERITY_ERROR, "Invalid PSRAM controller signature", "",
  "The driver did not return the expected signature from the PSRAM controller on your E16."
  "Make sure you built your firmware with the RAM enabled (ENABLE_RAM=y in your *.fwproj file."
},

{ERR_9011, XSEVERITY_ERROR, "Processor architecture not recognized", "",
  "The value of the environment variable %Processor_Architecture% was not recognized."
  "the cPicoStream class expected i386, amd64, or ia64."
},

{ERR_9012, XSEVERITY_ERROR, "Failed to create to modify a registry record", "",
  "The Stream class failed to create a registry record of volatile driver data and could not perform reboot."
  "Check to make sure that the following keyname exists and is not write protected, and then shut your PC down"
  "HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\services\\PicoComputingPicoSrvc (for 32-bit systems)"
  "HKEY_LOCAL_MACHINE\\SYSTEM\\ControlSet001\\services\\PicoService           (for 64-bit systems)"
},

{ERR_9013, XSEVERITY_ERROR, "After reboot, the PCIe address failed to reappear.", "",
  "During reboot of a Pico card the card was disabled and re-enabled. Despite the enable, the Pico Card never became visible "
  "to the system again, or if it did failed to load the driver correctly. \n\n"
  "This can occur for a number of reasons:\n\n"
  "Make sure that the loaded firmware has a PCIe core.\n\n"
  "Check the resources requested from the PCIe core:\n"
  "Under Windows, this error occurs if a Pico Card is loaded with an image that requests a "
  "different set of resource than those allocated at system startup. In other words "
  "the image on the flash ROM used to load the Pico Card has a different set of resources "
  "than the image you just loaded.\n\n"
  "You can correct this problem by performing a warm reboot:\n"
  "      For example: shutdown -r -t 0\n\n"
  "To correct this permanently you need to write your image to the flash ROM on the offending card, "
  "or change the resources you request in the firmware image to correspond to the flash boot image."
},

{ERR_9014, XSEVERITY_ERROR, "function not in debug version only", "",
  "The requested operation is only included in the debug version of this software. "
},
/*
{ERR_9015, XSEVERITY_ERROR, "The current user lacks the OS rights to perform a reboot", "",
	"The operating system has not granted the current user the right to alter device drivers, which the Stream class"
	" requires to perform a reboot. You may either elevate the current user's rights to Administrator, or alter this"
	" user's rights. (As an administrator, run GPedit.msc, and add the current user to the following key."
	" Computer Configuration\\Security Settings\\Local Polocies\\User Rights Assignment\\Load and Unload Device Drivers."
},
*/

{ERR_9015, XSEVERITY_ERROR, "The current user lacks the OS rights to perform a reboot", "",
	"The operating system has not granted the current user the right to alter device drivers, which the Stream class"
	" requires to perform a reboot. You must elevate the current user's rights to Administrator."
},

{ERR_9016, XSEVERITY_CATASTROPHIC, "Operating System error loading pico card driver", "os status code",
        "The operating system was unable to load pico64.sys. The OS status code "
        "should enable you to find the source of the error."
},

{ERR_9100, XSEVERITY_ERROR, "Bad read stream signature in firmware", "",
  "The stream signature in the stream status word is incorrect. Either the firmware is broken, or the read of the status word"
  " was corrupted."
},

{ERR_9101, XSEVERITY_ERROR, "Interrupted while waiting on stream", "",
  "The driver was interrupted while waiting on a stream, so it returned early. For example, the userspace program may have been killed."
},

{ERR_9102, XSEVERITY_ERROR, "Can't set exclusive mode - already exclusive to another process.", "",
  ""
},

{ERR_9103, XSEVERITY_ERROR, "Missed DMA IRQ.", "",
  "The driver didn't get the interrupt signaling the end of a data transfer, so the transfer may be incomplete."
},

{ERR_9104, XSEVERITY_ERROR, "Bad write stream signature in firmware", "",
  "The stream signature in the stream status word is incorrect. Either the firmware is broken, or the read of the status word"
  " was corrupted."
},

{ERR_9105, XSEVERITY_ERROR, "Interrupted while waiting on stream", "",
  "The driver was interrupted while waiting on a stream, so it returned early. For example, the userspace program may have been killed."
},

{ERR_9106, XSEVERITY_ERROR, "FPGA was not configured successfully.", "",
  "The FPGA didn't come back up after reconfiguration. This can be caused by an invalid bit file, power problems, or PCIe bus problems."
},

{ERR_9200, XSEVERITY_ERROR, "Can't locate EX500 for JTAG access.", "",
  "You're attempting to use the JTAG connection to the card, but this requires the card be used in the EX500 carrier, which you don't seem to have."
},

{ERR_9205, XSEVERITY_ERROR, "Can't connect to EX500 Spartan for JTAG access.", "",
  "Located an EX500, but can't connect to the Spartan chip on it for JTAG access. Is your EX500 Revision C or newer?"
},

{ERR_9206, XSEVERITY_ERROR, "Failed to load the EX500 Spartan for JTAG access.", "",
  "Failed loading the Spartan chip on the EX500."
},

{ERR_9207, XSEVERITY_ERROR, "EX500 Spartan for JTAG has bad status.", "",
  "The status register of the Spartan chip used for JTAG on the EX500 has an invalid status code."
},

{ERR_9208, XSEVERITY_ERROR, "Card didn't come back up after JTAG loading.", "",
  "When the FPGA is reloaded, it goes down and then comes back up with the new .bit file."
  " The OS needs to detect this and perform a hotswap cycle in order to connect to the new .bit file."
  " This usually has one of two causes: 1) The OS is not configured for hotswapping. Please see the hotswap section of the 'Getting Started Guide.'"
  " 2) The new .bit file is bad, and the FPGA didn't come back up and connect to PCIe properly."
},

{ERR_9209, XSEVERITY_ERROR, "Bad write data alignment or size.", "",
  "The data must be aligned appropriately for the card you're using, and the bus width of its firmware."
  " Please consult the documentation for your card for the alignment rules."
},

{ERR_9210, XSEVERITY_ERROR, "Internal driver error: bad card model in rctl/wctl.", "",
  ""
},

{ERR_9211, XSEVERITY_ERROR, "FPGA didn't report correct sequence number to host.", "",
  ""
},

{ERR_9212, XSEVERITY_WARNING, "Reloading the FPGA on a system that doesn't properly support hotswap is only supported with version 5.x firmware", "",
  ""
},

{ERR_9213, XSEVERITY_ERROR, "Bad PCI vendor ID after reload.", "",
  "The FPGA was loaded with an image that doesn't respond with the Pico PCI vendor ID. This usually means"
  " that the image is invalid. This is likely caused by not using the correct UCF file from Pico."
},

{ERR_9214, XSEVERITY_ERROR, "This FPGA is unconfigured.", "",
  "Please load the FPGA with a valid image before using it."
},

{ERR_9215, XSEVERITY_ERROR, "Couldn't re-allocate the interrupt after an FPGA reload without hotswapping.", "",
  "Your system didn't allow us to re-allocate the interrupt after rebooting."
  " If you are using Ubuntu-10.04 or earlier, your system may behave better if you enable hotswapping."
  " See the Getting Started Guide for instructions on how to enable hotswapping."
},

{ERR_9216, XSEVERITY_ERROR, "Memory reset command not properly written to stream.", "",
  "Unable to reset the firmware which connects the memory to the software API."
},

{ERR_9217, XSEVERITY_ERROR, "Invalid memory identifier.", "",
  "The memory ID is invalid. It should be something like PICO_DDR3_0, PICO_DDR3_1, etc."
},

{ERR_9218, XSEVERITY_ERROR, "Memory calibration timed out.", "",
  "The memory calibration process timed out. Please make sure you have memory installed in the card,"
  " and that it's the correct size and speed for the firmware you're using."
},

{ERR_9219, XSEVERITY_ERROR, "RAM command not properly written to stream.", "",
  "There was an error sending a memory operation to the firmware."
},

{ERR_9220, XSEVERITY_ERROR, "Error reading data from memory.", "",
  ""
},

{ERR_9221, XSEVERITY_ERROR, "Error writing data to memory.", "",
  ""
},

{ERR_9222, XSEVERITY_ERROR, "Size of parameter to iocontrol function is invalid.", "",
 ""
},
{ERR_9995, XSEVERITY_ERROR, "Internal software error: %s", "",
  "This error indicates that a serious error in the software has occurred.\n"
  "Look to the context of the message for more information."
},

{ERR_9996, XSEVERITY_ERROR, "Function %s is not implemented under this O/S", "functionName",
  "Function %s is not implemented under this operating system."
},

{ERR_9997, XSEVERITY_ERROR, "Function %s is not implemented on this platform", "functionName",
  "Function %s is not implemented on this platform."
},

{ERR_9998, XSEVERITY_ERROR, "%s Not yet coded", "what is not coded?",
  "Feature is planned for a future release, but is not coded in this release."
},

{ERR_9999, XSEVERITY_ERROR, "Not an error", "",
  ""
},

};// end of ErrorTable.

#if ERROR_HISTORY_DEPTH != 1
    char cSamError::m_errorContext [ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE],
         cSamError::m_errorLocation[ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE],
         cSamError::m_errorParam   [ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE];
    int  cSamError::m_lastErrorCode[ERROR_HISTORY_DEPTH] = {0},
         cSamError::m_lastErrInx;
#else
    SAM_LAST_ERROR     m_lastErr[ERROR_HISTORY_DEPTH];
    int m_lastErrInx;
    #define M_LAST_ERR m_lastErr[m_lastErrInx]
    #define M_LASTERR(ii) m_lastErr[i]
    #define m_lastErrInx 0
#endif

/*Log error: Save m_lastErrorCode, m_errorContext, and m_errorParam.
  erC      = error code,
  contextP = useful text to localize the error,
  paramP   = parameter to substitute in %s of error text.
NOTE: all strings are managed as 8 bits characters;They maybe converted to wide for output.
*/
int cSamError::LogError(int erC, const char *contextP, const char *paramP)
   {int ii; char *dP;

    //increment lastErrInx and wrap if necessary
    if (++m_lastErrInx >= ERROR_HISTORY_DEPTH) m_lastErrInx = 0;
    if (erC < 0) erC = 0 - erC;
    *m_errorContext[m_lastErrInx] = *m_errorParam[m_lastErrInx] = 0;
    m_lastErrorCode[m_lastErrInx] = erC;

    //Convert LPTSTR contextP to char *m_errorContext
    dP = m_errorContext[m_lastErrInx];
    if (!contextP) contextP = "";
    if ((ii=(int)strlen(contextP)) >= MAX_ERROR_PARAM_SIZE-1) ii = MAX_ERROR_PARAM_SIZE-1;
    if (ii != 0 && strlen(dP) == 0) //prevents wiping out good context when adding more
       for (dP[ii]=0; ii-- > 0; ) if ((*dP++ = (char)*contextP++) == 0) break;

    //convert LPTSTR paramP to char *m_errorParam
    dP = m_errorParam[m_lastErrInx];
    if (!paramP) paramP = "";
    if ((ii=(int)strlen(paramP)) >= MAX_ERROR_PARAM_SIZE-1) ii = MAX_ERROR_PARAM_SIZE-1;
    if (ii != 0 && strlen(dP) == 0) //prevents wiping out good param when adding more
       for (dP[ii]=0; ii-- > 0; ) if ((*dP++ = (char)*paramP++) == 0) break;
    return -erC;
   } //cSamError::g_err.LogError...

int cSamError::LocateError(int erC)
   {int ii; if (erC < 0) erC = 0 - erC;
    for (ii=sizeof(ErrorTable)/sizeof(ErrorTable[0]); --ii >= 0;)
       if (erC == ErrorTable[ii].erC) return ii;
    return LocateError(ERR_0002); //error not found
   } //cSamError::Severity...

const char *cSamError::LocateErrorP(int erC) {return ErrorTable[LocateError(erC)].fullErrorP;}

//Return severity level of error code erC.
int cSamError::Severity(int erC)
   {if (erC < 0) erC = 0 - erC;
    if (erC == 0) return XSEVERITY_WARNING;
    for (int ii=sizeof(ErrorTable)/sizeof(ErrorTable[0]); --ii >= 0;)
       if (erC == ErrorTable[ii].erC) return ErrorTable[ii].severity;
    return XSEVERITY_ERROR;
   } //cSamError::Severity...

static bool CRorLF(char ch) {return ch == '\n' || ch == '\r';}

int cSamError::PrepareError(int erC, char *explanationP, int explanationSz, char *contextP, int contextSz, char *locationP, int locationSz)
   {char        *pp;
    const char  *textP="";
    int          ii, len, erri, tblInx;

    if (explanationSz) explanationP[0] = 0;
    if (contextSz    ) contextP    [0] = 0;
    if (locationSz   ) locationP   [0] = 0;
    if (erC <  0) erC = 0 - erC;
    if (erC == 0) erC = m_lastErrorCode[m_lastErrInx];
    contextP[0] = 0;
    //We have as many as ERROR_HISTORY_DEPTH in the cache, see if one matches the requested error
    for (erri=m_lastErrInx, ii=0; ii < ERROR_HISTORY_DEPTH; ii++)
        if (m_lastErrorCode[ii] == erC)
            {strncpy(contextP, m_errorContext[ii], contextSz-1); contextP[contextSz-1] = 0;
             break;
            }
    tblInx       = LocateError(erC);
    textP        = ErrorTable[tblInx].errTextP;
    //if size of resultP is too small, get lost.
    if (explanationSz <= (int)strlen(textP) + (int)strlen("warning 1234: "))
       {strlcpy(explanationP, textP, explanationSz);
        return (int)strlen(explanationP);
       }
    snprintf(explanationP, explanationSz, "%s #%d: %s", (ErrorTable[tblInx].severity > XSEVERITY_WARNING) ? "Error" : "Warning", erC, textP);

    //if lastErrCode != erC,  context and param apply to a different error. Limit to text only.
    if ((m_lastErrorCode[erri] < 0 ? - m_lastErrorCode[erri] : m_lastErrorCode[erri]) != erC) goto xit;

    //Replace param if text of error message contains %s
    if (pp=strstr(explanationP, "%s")) //has replaceable param
       {explanationSz -= (int)(pp - explanationP); //space available in output buffer
        if (explanationSz < 0) {*pp++= ' '; *pp++ = ' '; goto xit;}
        strlcpy(pp, m_errorParam[erri], explanationSz);
        strlcat(explanationP, strstr(textP, "%s")+2, explanationSz);
       }
    else //make sure we don't lose m_errorParam if no %s found.
       {if (contextP[0] == 0 && m_errorParam[erri][0] != 0) strncpy(contextP, m_errorParam[erri], contextSz);
       }
    //prune trailing CR/LF from explanation and context.
    for (len=(int)strlen(explanationP); len > 0 && CRorLF(explanationP[--len]);) explanationP[len] = 0;
    for (;len > 0; len--)                     {if (CRorLF(explanationP[len]))    explanationP[len] = ' ';}
    for (len=(int)strlen(contextP);     len > 0 && CRorLF(contextP[--len]);)     contextP[len]     = ' ';
    //prepare context
    explanationP[explanationSz-1] = 0;
    len = (int)strlen(explanationP);
    if (ErrorTable[tblInx].severity == XSEVERITY_SYSTEM) ShowOSerror(&explanationP[len], explanationSz - len);

    strncpy(locationP, m_errorLocation[erri], locationSz-1); locationP[locationSz-1] = 0;
xit:return erC;
   } //cSamError::PrepareError...

//Build an error message including the parameters and context saved by cSamErrors::LogError.
//resultP should be at least TCHAR[MAX_ERROR_MESSAGE_SIZE+2*MAX_ERROR_PARAM_SIZE+25] to fit all messages
char *cSamError::ShortError(int erC, char *resultP, int resultSize)
   {char        *explanationP, *contextP, *locationP, *pp;                        //
    if (resultSize <= 1) return resultP;                                          //
    explanationP = (char*)calloc(resultSize, sizeof(char));                       //
    contextP     = (char*)calloc(resultSize, sizeof(char));                       //
    locationP    = (char*)calloc(resultSize, sizeof(char));                       //
    erC          = PrepareError(erC, explanationP, resultSize, contextP, resultSize, locationP, resultSize);
    snprintf(resultP, resultSize, "%s\n%s\n%s\n", explanationP, contextP, locationP);
    for (pp=&resultP[strlen(resultP)-1]; strchr("\r\n", *pp) != NULL;) *pp-- = 0; //trim trailing \r\n
    strcat(pp, "\n");                                                             //then put \n back on !
    free(explanationP);                                                           //
    free(contextP);                                                               //
    free(locationP);                                                              //
    resultP[resultSize-1] = 0;                                                    //
    #if 0 //def UNICODE
    //convert to TCHAR. sizeof(TCHAR) == 2.
    if ((ii=(int)strlen(resultP))*2 >= resultSize) resultP[ii=resultSize/2] = 0;
    for (; ii >= 0; ii--) {resultP[2*ii] = resultP[ii]; resultP[2*ii+1] = 0;}
#endif
    return resultP;
   } //cSamError::Interpret...

/*Build an error message including the parameters saved by PicoErrors_log
  plus the body of the error from the error file called fileNameP or RegisteredFileName
  NOTE: resultP should be at significantly larger than MAX_ERROR_MESSAGE_SIZE + MAX_ERROR_PARAM_SIZE
        to fit all messages, plus their explanations.*/
char *cSamError::FullError(int erC, char *resultP, int resultSize)
   {char *resP=resultP, *pp, *qq;
    const char *formalParmP;
    int   ii, jj, erri, lenParam;
    resultP[resultSize-1] = 0; //guarrantee a terminator
    if (erC < 0) erC = 0 - erC;
    for (erri=m_lastErrInx, ii=0; ii < ERROR_HISTORY_DEPTH; ii++) if (m_lastErrorCode[ii] == erC) {erri= ii; break;}
    if (erC == 0) {if ((erC = m_lastErrorCode[erri]) == 0) return (char*)"No Error";}
    ShortError(erC, resP, resultSize-5); strlcat(resP, "\n\n", resultSize);
    ii          = (int)strlen(resP)*((int)sizeof(resP[0]));
    resultSize -= ii;
    resP       += ii;
    if ((ii=LocateError(erC)) != ERR_0002) //2 == unknown error (table[2] == ERR_0002 !)
       {if ((pp=strstr(resultP, "%s")) && (formalParmP=ErrorTable[ii].parameterP) && formalParmP[0])
          {if ((int)(strlen(resP) + (lenParam=(int)strlen(formalParmP))) < resultSize)
              {jj    = (int)strlen(pp+2);  //length of message following '%s'
               *pp++ = '<';                //followed by formal parameter
               //right shift remainder of message
               for (qq=&resultP[strlen(resultP)]; jj-- >= 0; qq--) qq[lenParam] = *qq;
               //insert format parameter
               strncpy(pp, formalParmP, lenParam); pp[lenParam] = '>';
               resP       += jj=(int)strlen(resP);
               resultSize -= jj;
          }   }
        strncat(resP, ErrorTable[ii].fullErrorP ? ErrorTable[ii].fullErrorP : "", resultSize-strlen(resP)-2);
       }
#if 0 //def UNICODE
    //convert to TCHAR...
    ii = strlen(resP); if (ii > resultSize) resP[ii=resultSize/2] = 0;
    for (; ii >= 0; ii--) {resP[2*ii] = resP[ii]; resP[2*ii+1] = 0;}
#endif
    return resultP;
   } //cSamError::FullError...

int cSamError::PrintError(int erC, const char *contextP, const char *paramP)
   {char buf[256]; 
    Printf("%s\n", ShortError(LogError(erC, contextP, paramP), buf, sizeof(buf))); 
    return erC < 0 ? erC : -erC;
   } //cSamError::PrintError...

void cSamError::Clear(void)
  {memset(m_errorContext, 0, sizeof(m_errorContext));
   memset(m_errorParam,   0, sizeof(m_errorParam));
   memset(m_lastErrorCode,0, sizeof(m_lastErrorCode));
  } //cSamError::Clear...

//erri = 0 thru (ERROR_HISTORY_DEPTH-1) will cycle through last errors.
int cSamError::GetLastErrorInfo(char **paramPP, char **contextPP, int erri) //== -1
   {if (erri < 0 || erri >= ERROR_HISTORY_DEPTH) erri = m_lastErrInx;
    if (paramPP)   *paramPP   = m_errorParam[erri];
    if (contextPP) *contextPP = m_errorContext[erri];
    return m_lastErrorCode[erri];
   } //cSamError::GetLastErrorInfo...

int cSamError::GetLastError(void) {return m_lastErrorCode[m_lastErrInx];}

void cSamError::AddContext(const char *moreContextP)
   {int sz=sizeof(m_errorContext[m_lastErrInx])-1;
    strncat(m_errorContext[m_lastErrInx], moreContextP, sz - strlen(m_errorContext[m_lastErrInx]));
    m_errorContext[m_lastErrInx][sz] = 0;
   } //cSamError::AddContext...

void cSamError::AddLocation(const char *locationP)
   {strncpy(m_errorLocation[m_lastErrInx], locationP, sizeof(m_errorLocation[m_lastErrInx]) - 1);
    m_errorLocation[m_lastErrInx][sizeof(m_errorLocation[m_lastErrInx])-1] = 0;
   } //cSamError::AddLocation...

char *cSamError::ShowOSerror(char *bufP, int bufSize) {
#if defined(_WIN32) && defined(FORMAT_MESSAGE_FROM_SYSTEM)
     unsigned long erC;
     bufP[0] = 0;
     if (erC=errno)
       FormatMessageA
          (FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
           NULL,
           erC,
           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
           bufP, bufSize, NULL);
     bufP[bufSize-1] = 0;
#else
    strlcpy(bufP, strerror(errno), bufSize);
#endif
     return bufP;
    } //cSamError::ShowOSerror...

#endif //_C3_ERRORS_CPP_INCLUDED_

//End of file
