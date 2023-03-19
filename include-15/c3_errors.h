/*
=============================================================================
     File: C3_errors.h. Ver , Mar 19 2023.

     This file contain error codes in the format:
          #define ERR_<4digitNumber> <4digitNumber> //<description>
     The first number is the erC field of the error in c3_Error.cpp.
     The <description> field is the errTextP field in the same file.

     The file is generated from ../source/c3_Error.cpp using
     the program BuildErrors.exe. >>>....>>> DO NOT CHANGE MANUALLY <<<...<<<

     Copyright Sadram, Inc., 2022. All rights reserved.

 =============================================================================
*/

#ifndef __SAM__ERRORS_FILE_INCLUDED
#define __SAM__ERRORS_FILE_INCLUDED

enum {XSEVERITY_NOERROR,      //not an error at all
      XSEVERITY_INFORMATION,  //information only
      XSEVERITY_HINT,         //helpful information
      XSEVERITY_WARNING,      //non-fatal error
      XSEVERITY_DANGEROUS,    //non-fatal error but degrading of performance
      XSEVERITY_ERROR,        //error that will stop the program
      XSEVERITY_MESSAGEBOX,   //                  "             and show wxMessageBox()
      XSEVERITY_SYSTEM,       //system level error
      XSEVERITY_CATASTROPHIC};//error with lethal consequences (lock up your children :)

#define SAM_ERROR_LOGGER(name)     int name(int erC, const char *contextP, const char *paramP)

//Following define is a comma delimitted list of error codes that are at or below the warning level.
#define WARNING_AND_INFORMATION_ERRNUMS\
          1031,1106,1561,1581,1793,1821,2003,2502,2504,2505,2506,2507,3037,3510,3512,3514,3516,3518,3523,3524,3525,3526,3527,3528,3529,3530,3531,3532,3533,3534,3535,\
          3536,3537,3538,3539,3540,3541,3542,3543,3544,3545,3546,3547,3548,3549,3038,6920,6947,7017,7018,7021,7022,7023,7049,7050,7051,7052,7103,7122,7124,7155,7156,\
          7170,7192,7204,7206,7209,7214,7223,7224,7254,7303,7319,7333,7346,7350,7357,7380,7381,7381,7452,7516,7517,7518,7519,7525,9212
#define ERROR_HISTORY_DEPTH 4                   //number of error codes in history. Used to get error parameter by ShortError.
static const int MAX_ERROR_PARAM_SIZE=256,      //maximum size of the param field to cPicoError::LogError
                 MAX_ERROR_MESSAGE_SIZE=  256;  //maximimum size of the text defined in error table.

#define ERR_0001    1 //Unknown error
#define ERR_0003    3 //File or directory not found (%s)
#define ERR_0002    2 //Unknown error code
#define ERR_0005    5 //Not enough memory (%s)
#define ERR_0006    6 //Memory allocation error
#define ERR_0997  997 //Feature not supported in this release %s.
#define ERR_0998  998 //Error in external module (%s).
#define ERR_1000 1000 //Error Number %s specified twice.
#define ERR_1001 1001 //Missing error 'number=' or 'summary=' field.
#define ERR_1002 1002 //Invalid number (%s).
#define ERR_1003 1003 //Unbalanced quotes in a string: %s
#define ERR_1004 1004 //Unable to find --> or < or > found in comment.
#define ERR_1005 1005 //Invalid .bit file format. Missing sync flag.
#define ERR_1006 1006 //Flash data verification failed.
#define ERR_1010 1010 //Can't detect flash chip for loading the FPGA.
#define ERR_1020 1020 //Can't set FPGA load address.
#define ERR_1021 1021 //Card hotswapping failed.
#define ERR_1029 1029 //Fan out exceeded.
#define ERR_1030 1030 //Unbalanced /* versus */ in a comment, begging with: %s
#define ERR_1031 1031 //Unknown #pragma directive: '%s'
#define ERR_1032 1032 //Duplicate parameter name '%s'
#define ERR_1033 1033 //Syntax error in #directive .... 
#define ERR_1034 1034 //Unknown #directive '%s'
#define ERR_1035 1035 //Unable to find #endif corresponding to this #if %s.
#define ERR_1036 1036 //#if ... , #set ... , #reset ...  directive inside #if  ....#endif (%s).
#define ERR_1037 1037 //#endif, #else, #elif, #endfor, or #endef found without matching opening directive ...  (%s).
#define ERR_1038 1038 //Variable in #if(%s) is not found in a #define directive or formal parameter list.
#define ERR_1039 1039 //Unknown #directive name (%s).
#define ERR_1040 1040 //File has already been included (%s)
#define ERR_1045 1045 //Invalid attribute in %s tag
#define ERR_1046 1046 //< found without a corresponding > in an XML node
#define ERR_1047 1047 //> found without a corresponding <
#define ERR_1048 1048 //<!-- found without --> in an XML comment
#define ERR_1049 1049 //Incomplete parameter list for a printf type statement
#define ERR_1095 1095 //Invalid characters in define macro name: '%s'
#define ERR_1096 1096 //Incomplete parameter list for define ... %s
#define ERR_1097 1097 //Invalid option in #pragma directive '%s'
#define ERR_1100 1100 //Unable to find card with access to EX500 board.
#define ERR_1101 1101 //Unacceptable input parameters:%s
#define ERR_1102 1102 //EE did not stabilize
#define ERR_1103 1103 //Signature on EE rom is invalid
#define ERR_1104 1104 //EE rom did not return proper status
#define ERR_1106 1106 //EE rom is unitialized
#define ERR_1159 1159 //Syntax error processing %s.
#define ERR_1379 1379 //Invalid '&' directive in XML.
#define ERR_1380 1380 //Unacceptable child element (%s) in XML group.
#define ERR_1404 1404 //single quoted string must be one character long.
#define ERR_1405 1405 //Missing or improper label.
#define ERR_1406 1406 //Missing close quote in string constant.
#define ERR_1407 1407 //Missing semicolon in command '%s'.
#define ERR_1408 1408 //Invalid option to Copy command in WizardCommands.txt '%s'.
#define ERR_1498 1498 //Error in call to Pico Computing Server %s.
#define ERR_1500 1500 //Error in hostname or portname to Pico_ipXface.
#define ERR_1501 1501 //Incorrect package sequence in TFTP message.
#define ERR_1502 1502 //TFTP server has returned an error packet.
#define ERR_1503 1503 //TFTP Error unknown
#define ERR_1504 1504 //TFTP Error file not found
#define ERR_1505 1505 //TFTP Error access
#define ERR_1506 1506 //TFTP Error target device full
#define ERR_1507 1507 //TFTP Error illegal op
#define ERR_1508 1508 //TFTP Error illegal port number
#define ERR_1509 1509 //File '%s' already exists.
#define ERR_1510 1510 //Unable to obtain buffer to send Ethernet message.
#define ERR_1526 1526 //Incorrect syntax of field name '%s'
#define ERR_1530 1530 //No Response from MRC-200.
#define ERR_1531 1531 //No Response to ARP generated by MRC-200 charger board
#define ERR_1544 1544 //Estimated file size exceeded.
#define ERR_1545 1545 //Data not properly written to SecSci.
#define ERR_1546 1546 //Data for SecSci is not properly formatted.
#define ERR_1547 1547 //%s Ip addresses are inconsistent.
#define ERR_1548 1548 //Error creating MRC-200.cfg file.
#define ERR_1549 1549 //Error Processing MRC-200.cfg file.
#define ERR_1550 1550 //Error accessing flash ROM.
#define ERR_1551 1551 //Error interfacing with flash ROM on MRC-200.
#define ERR_1552 1552 //Protocol error accessing MRC-200.
#define ERR_1553 1553 //Timeout transferring data to remote card.
#define ERR_1554 1554 //Module not found.
#define ERR_1555 1555 //Unable to obtain allocation information for flash ROM.
#define ERR_1556 1556 //Sector was not erased.
#define ERR_1557 1557 //Error capturing data from MRC-200.
#define ERR_1558 1558 //Error writing permanent information '%s'.
#define ERR_1559 1559 //Multiple occurrences of file '%s'.
#define ERR_1560 1560 //Incorrect password for flash access.
#define ERR_1561 1561 //Remember to change Maintenance Mode switch to '%s' before rebooting.
#define ERR_1562 1562 //File download module not present in bitfile.
#define ERR_1563 1563 //Transfer incomplete.
#define ERR_1564 1564 //File Checksum error.
#define ERR_1565 1565 //Header field %s has invalid format.
#define ERR_1566 1566 //Buffer is too small to perform requested operation.
#define ERR_1567 1567 //Error updating variable %s.
#define ERR_1568 1568 //Timeout waiting for sent data on TCP/IP socket
#define ERR_1569 1569 //AwaitSendPacket cannot be called from inside an interrupt.
#define ERR_1570 1570 //Error accessing flash
#define ERR_1571 1571 //PA signal temperature greater than PaOverTemp
#define ERR_1572 1572 //VSWR is too large
#define ERR_1573 1573 //Radio's drawing current, P/A is not
#define ERR_1574 1574 //An I2C device did not respond properly
#define ERR_1575 1575 //no information returned from PA
#define ERR_1576 1576 //internal batteries are too low
#define ERR_1577 1577 //missing internal battery
#define ERR_1578 1578 //BIT failure (multiple sources)
#define ERR_1579 1579 //MRC-200.cfg has errors.
#define ERR_1580 1580 //P/A is drawing large current (stuck in transmit)
#define ERR_1581 1581 //Running on internal batteries
#define ERR_1582 1582 //Timeout waiting for data from Power Amplifier.
#define ERR_1583 1583 //Flash Parameters are unknown.
#define ERR_1590 1590 //I2C device %s is babbling
#define ERR_1591 1591 //Not data returned on I2C device %s
#define ERR_1592 1592 //Invalid script command:%s
#define ERR_1593 1593 //Unknown variable:%s
#define ERR_1594 1594 //Error reading/writing variable to charger board:%s
#define ERR_1601 1601 //Unable to find UDP channel to communicate with MRC-200.
#define ERR_1602 1602 //RS232 test has failed.
#define ERR_1603 1603 //Unable to create process thread.
#define ERR_1604 1604 //Unable to open %s port.
#define ERR_1605 1605 //Invalid com port settings.
#define ERR_1606 1606 //Error accessing specified com port.
#define ERR_1607 1607 //Error Reading %s file.
#define ERR_1792 1792 //User defined error in preprocessor '%s'
#define ERR_1793 1793 //User defined warning in preprocessor '%s'
#define ERR_1800 1800 //Error starting Sockets
#define ERR_1801 1801 //Timeout waiting for data on TCP/IP socket
#define ERR_1803 1803 //TCP/IP connection gracefully closed
#define ERR_1804 1804 //Syntax error in command string to TCP / UDP driver
#define ERR_1805 1805 //Terminating sockets interface
#define ERR_1806 1806 //Unable to find unicast address to support multicast group
#define ERR_1807 1807 //Unable to start sockets.
#define ERR_1808 1808 //Error connecting to a socket %s
#define ERR_1809 1809 //Device type parameter is illegal.
#define ERR_1810 1810 //Unable to create a socket.
#define ERR_1811 1811 //Missing delimiter in hostName/serviceName field.
#define ERR_1812 1812 //Unable to resolve host name (%s) to an IP address.
#define ERR_1813 1813 //Unable to resolve service name to an IP port address.
#define ERR_1814 1814 //Unable to create a process to handle UDP access.
#define ERR_1815 1815 //Error encountered reading a socket.
#define ERR_1816 1816 //Error encountered writing a socket.
#define ERR_1817 1817 //Error binding socket to local address.
#define ERR_1818 1818 //Unable to join multicast group.
#define ERR_1819 1819 //Error setting a socket to broadcast mode.
#define ERR_1820 1820 //Missing or invalid destination address for Route Add '%s'
#define ERR_1821 1821 //Inter Record Gap is significantly changed %s
#define ERR_1822 1822 //Timeout waiting for reply from server
#define ERR_1830 1830 //Unable to ping target device
#define ERR_1851 1851 //Packet Buffer too small for received packet
#define ERR_1852 1852 //Packet directed at Pico board lacks 'PICO' signature
#define ERR_1853 1853 //Monitor-reserved command code is undefined '%s'
#define ERR_2000 2000 //Unable to build synopsis of file data on flash
#define ERR_2001 2001 //Invalid call sequence to ProgramAbuffer
#define ERR_2002 2002 //Physical parameters of flash ROM are not valid
#define ERR_2003 2003 //Last File on flash ROM
#define ERR_2004 2004 //File not found on Flash ROM '%s'
#define ERR_2005 2005 //Missing or unknown command line argument'%s'
#define ERR_2500 2500 //Invalid attribute syntax in A/P declaration attributes: '%s'
#define ERR_2501 2501 //Missing close > in declaration attribute.
#define ERR_2502 2502 //Attribute not implemented on specified platform: %s
#define ERR_2503 2503 //Invalid Initialization of variable:%s
#define ERR_2504 2504 //No bot containing an input parameter was encountered
#define ERR_2505 2505 //Unexpected qualifier to a MAD statement
#define ERR_2506 2506 //Feature %s is not available on current platform
#define ERR_2507 2507 //Invalid <attribute>: %s
#define ERR_2508 2508 //Incorrect signature in BOT file: %s
#define ERR_2509 2509 //Unable to find a record containing BOT code for specified botnumber: %s
#define ERR_2510 2510 ///* found without */ in a comment: %s
#define ERR_2511 2511 //End of file: %s
#define ERR_2512 2512 //Local variable name may not be the same as the BOT name: %s
#define ERR_2513 2513 //Global variable '%s' must be declared as const
#define ERR_2514 2514 //Cannot assign value to const variable:%s
#define ERR_2515 2515 //Missing equal sign in #directive: %s
#define ERR_2516 2516 //Opcode not available on this platform: %s
#define ERR_2520 2520 //Missing '{', '}', or ';' in Sam statement: '%s...'
#define ERR_2521 2521 //Invalid platform: %s in /p command or environment variable madPlatform
#define ERR_2522 2522 //'%s' is not a valid opcode.
#define ERR_2523 2523 //Missing semicolon at end of asm statement: %s
#define ERR_2524 2524 //Stack overflow
#define ERR_2525 2525 //Constant exceed register size of target
#define ERR_2526 2526 //No bot file specified
#define ERR_2527 2527 //Illegal opcode
#define ERR_2528 2528 //asm{... may not have two parameters to $printf()
#define ERR_2531 2531 //Requested bot number is larger than the available bots in the hardware
#define ERR_2532 2532 //Requested bits exceed capacity of underlying hardware
#define ERR_2533 2533 //include directory not specified (as required for the /p command).
#define ERR_2534 2534 //Requested data not found for specified BOT %s in file %s
#define ERR_2590 2590 //Duplicate filename on command line: %s
#define ERR_2600 2600 //Software error generating botcode from Madcode: %s
#define ERR_2601 2601 //Software error generating line number table from Madcode:
#define ERR_2602 2602 //Unresolved label: '%s'
#define ERR_2701 2701 //m_params != Computed structure sizes
#define ERR_2702 2702 //Inconsistent setting for params.testAdr:%s
#define ERR_2703 2703 //Sequence error in BRAM dump used
#define ERR_2704 2704 //stopAtLine must not be zero in params.txt
#define ERR_2705 2705 //target of opcode is invalid
#define ERR_2706 2706 //the number commands(=%s) in user.cmds > space available
#define ERR_2707 2707 //Unable to open 'simulate.log'
#define ERR_2708 2708 //#check message has invalid format: %s
#define ERR_2709 2709 //Row[%sd] out of sequence
#define ERR_2710 2710 //Unknown #check number: %s
#define ERR_2711 2711 //target is invalid: %s\n
#define ERR_2712 2712 //%s inconsistent: FPGA versus software
#define ERR_2713 2713 //Wrong duck: %s
#define ERR_2714 2714 //Invalid command on command: %2
#define ERR_2715 2715 //the row is out of sequence
#define ERR_2716 2716 //commands(=%s) in userCmds.data has invalid length
#define ERR_2717 2717 //commands(=%s) in userCmds.data contains invalid data
#define ERR_2718 2718 //commands(=%s) in userCmds.data has invalid opcode
#define ERR_2719 2719 //commands(=%s) in userCmds.data has invalid item field
#define ERR_2720 2720 //Unable to find %s
#define ERR_2721 2721 //%s is not consistent with value from previous simulation
#define ERR_2722 2722 //Ducks do not correspond to grpMask: %s
#define ERR_2723 2723 //Insert point is incorrect: %s%s
#define ERR_2724 2724 //Error reported by firmware: %s
#define ERR_2725 2725 //Data read from memory is incorrect, %s
#define ERR_2726 2726 //Else is not followed by {
#define ERR_2727 2727 //Missing comma (,) in $bug statement.
#define ERR_2728 2728 //Shift Amount was not found or is invalid
#define ERR_2729 2729 //Missing equal ('=') in assignment statement
#define ERR_2730 2730 //Register overflow
#define ERR_2731 2731 //Improper qualifer
#define ERR_2732 2732 //Unknown or improper target specified
#define ERR_2733 2733 //Invalid element in an expression (%s).
#define ERR_2734 2734 //Repeat field is too large.
#define ERR_2735 2735 //Invalid row override.
#define ERR_2736 2736 //Invalid register.
#define ERR_2737 2737 //Illegal placement of break statement.
#define ERR_2738 2738 //Register not found following arithmetic opcode
#define ERR_2739 2739 //#expect: <value> not equal to #actual: <value> in simulation.
#define ERR_2740 2740 //Address of call is beyond range of call OPcode.
#define ERR_2741 2741 //Sam source file not specified on command line.
#define ERR_3000 3000 //Missing name
#define ERR_3001 3001 //Duplicate name in declaration
#define ERR_3002 3002 //Variable '%s' has not been declared
#define ERR_3003 3003 //Missing semicolon in statement
#define ERR_3004 3004 //Missing opening square bracket '[' or brace '{'
#define ERR_3005 3005 //Missing closing square bracket ']' or brace '}'
#define ERR_3006 3006 //Invalid $variable '%s'
#define ERR_3007 3007 //Missing open parenthesis in if, while, for, or function call
#define ERR_3008 3008 //Invalid indexing expression for %s
#define ERR_3009 3009 //Only one instance of $input or $stop allowed
#define ERR_3010 3010 //Pre/post increment instruction must be followed by a variable name
#define ERR_3011 3011 //Cannot Pre and post increment a variable
#define ERR_3012 3012 //Invalid opcode in p-code
#define ERR_3013 3013 //First parameter to $display function should be a string
#define ERR_3014 3014 //Missing closing }, ) or ]
#define ERR_3015 3015 //Pre increment is not supported
#define ERR_3016 3016 //Unknown #directive or syntax error in #directive
#define ERR_3017 3017 //Missing comma
#define ERR_3018 3018 //Invalid option or duplicate command on command line '%s'
#define ERR_3019 3019 //Parameters to a BOT must be type input or type event
#define ERR_3020 3020 //Missing set membership operator (::)
#define ERR_3021 3021 //Syntax requires a constant or constant expression
#define ERR_3022 3022 //Invalid declaration or statement:%s
#define ERR_3023 3023 //Parameter in formatting phrase is inconsistent with the actual parameter
#define ERR_3024 3024 //Named parameter '%s' may not be mixed with positional parameters in a #define
#define ERR_3025 3025 //Named parameter to a #define must be followed by '='
#define ERR_3026 3026 //Number of parameters to a #define does not match the declaration
#define ERR_3027 3027 //Named parameters to a #define used twice
#define ERR_3028 3028 //Constant declaration must have an initial value
#define ERR_3029 3029 //Block define %s may not contain #define, #undef, or #include macros
#define ERR_3030 3030 //The #define %s already exist with a different define body
#define ERR_3031 3031 //Impermissible string operation in #directive
#define ERR_3032 3032 //Invalid integer declaration '%s'
#define ERR_3033 3033 //Define expansion level is too deep in '%s'
#define ERR_3034 3034 //The expression cannot be realized on the underlying BOT '%s'
#define ERR_3035 3035 //Too many  initializers for array: %s
#define ERR_3036 3036 //Illegal or out of sequence iterator in declaration: %s
#define ERR_3037 3037 //Too few initializers: %s
#define ERR_3500 3500 //STM error: %s
#define ERR_3501 3501 //Field '%s' not found in Verilog file
#define ERR_3502 3502 //Data read back from RAM does not compare with data written to RAM
#define ERR_3503 3503 //Requested block RAMs Exceeds total block RAMS on Zynq
#define ERR_3504 3504 //kcount (=%s) should be a 8, 16, 32, or 64.
#define ERR_3505 3505 //Record size (=%s) should be a power of two.
#define ERR_3506 3506 //kwidth should be numeric and non zero
#define ERR_3507 3507 //Unable to locate text '%s'
#define ERR_3508 3508 //End of line 'set_property BISTREAM.CONFIG...' not found
#define ERR_3509 3509 //Unacceptable keyword: %s in regen command
#define ERR_3510 3510 //Value(s) adjusted: %s
#define ERR_3511 3511 //Length requested for scope record (%s bytes) is too large
#define ERR_3512 3512 //Filter conflict: %s
#define ERR_3513 3513 //Compare error in loopback test.
#define ERR_3514 3514 //SCOPE_RECORD as defined in Verilog file does not comport with record returned from firmware
#define ERR_3515 3515 //Requested variable exceed absolute upper limit of FPGA implementation
#define ERR_3516 3516 //field SCOPE_FILLER is incorrectly formed.
#define ERR_3517 3517 //Invalid field name in scope record: %s
#define ERR_3518 3518 //SCOPE_FILLER size was adjusted
#define ERR_3519 3519 //Number of records reported does not match valuesin keyTable[]
#define ERR_3520 3520 //Patch not found: %s
#define ERR_3521 3521 //Incompatible parameters to generate function: %s
#define ERR_3522 3522 //Error in sort table (%s)
#define ERR_3523 3523 //Sort RAM overflow
#define ERR_3524 3524 //Error not documented: %s
#define ERR_3525 3525 //Error not documented: %s
#define ERR_3526 3526 //Error not documented: %s
#define ERR_3527 3527 //Error not documented: %s
#define ERR_3528 3528 //Error not documented: %s
#define ERR_3529 3529 //Error not documented: %s
#define ERR_3530 3530 //Error not documented: %s
#define ERR_3531 3531 //Error not documented: %s
#define ERR_3532 3532 //Error not documented: %s
#define ERR_3533 3533 //Error not documented: %s
#define ERR_3534 3534 //Error not documented: %s
#define ERR_3535 3535 //Error not documented: %s
#define ERR_3536 3536 //Error not documented: %s
#define ERR_3537 3537 //Error not documented: %s
#define ERR_3538 3538 //Error not documented: %s
#define ERR_3539 3539 //Error not documented: %s
#define ERR_3540 3540 //Cannot have competing states in state transition: %s
#define ERR_3541 3541 //Error not documented: %s
#define ERR_3542 3542 //Error not documented: %s
#define ERR_3543 3543 //Error not documented: %s
#define ERR_3544 3544 //Error not documented: %s
#define ERR_3545 3545 //Error not documented: %s
#define ERR_3546 3546 //Error not documented: %s
#define ERR_3547 3547 //Error not documented: %s
#define ERR_3548 3548 //Error not documented: %s
#define ERR_3549 3549 //Error not documented: %s
#define ERR_3038 3038 //Function, bot or ste declaration must be at the global level
#define ERR_6901 6901 //Platform flash failed to change ISC mode
#define ERR_6902 6902 //Block number(s) specified for Platform Flash is out of range
#define ERR_6903 6903 //Invalid parameter(s) to /fw command
#define ERR_6904 6904 //Invalid sub-command to /f<letter> command
#define ERR_6905 6905 //Jtag interface is busy
#define ERR_6906 6906 //Jtag interface was not opened
#define ERR_6907 6907 //Slot number proposed for JTAG interface is not valid
#define ERR_6908 6908 //LX45(T) is not loaded or failed to load when requested
#define ERR_6909 6909 //Device Position proposed for JTAG interface is invalid
#define ERR_6910 6910 //Error in TDO data returned from JTAG device or insufficient data returned
#define ERR_6911 6911 //GPIO interface failed
#define ERR_6912 6912 //GPIO Data strobe is not reflected properly by data strobe acknowledge
#define ERR_6913 6913 //Parameters to WriteLed are invalid
#define ERR_6914 6914 //Specified Pico card does not have platform flash
#define ERR_6915 6915 //Field specified is invalid
#define ERR_6916 6916 //Attempt to read platform flash failed after 10 attempts
#define ERR_6917 6917 //Command to platform flash requires the P/F be in ISC mode
#define ERR_6918 6918 //Invalid command to platform flash
#define ERR_6919 6919 //Unknown zw command 
#define ERR_6920 6920 //JTAG  interface is not available
#define ERR_6921 6921 //JTAG lines are being driven by an external source.
#define ERR_6930 6930 //Unknown FPGA manufacturer
#define ERR_6931 6931 //FPGA not found at specified slot
#define ERR_6940 6940 //Unable to open bit file required for %s
#define ERR_6941 6941 //Internal Software Error
#define ERR_6944 6944 //Unable to locate Xilinx Impact program in picomfr
#define ERR_6946 6946 //Unknown product
#define ERR_6947 6947 //Conflicting FPGA/model number
#define ERR_6950 6950 //Missing delimiter in log file %s
#define ERR_6951 6951 //Card%s is over temperature
#define ERR_7000 7000 //Flash ROM has no primary image
#define ERR_7001 7001 //Files are missing in Flash ROM directory
#define ERR_7002 7002 //Notice: Do you wish to write a backup of the primary image
#define ERR_7003 7003 //E-12 Pico card is operating in degraded mode.
#define ERR_7004 7004 //Error reading remote file system.
#define ERR_7005 7005 //Uncorrectable error reading remote file:
#define ERR_7006 7006 //Uncorrectable error reading NAND flash:
#define ERR_7007 7007 //Initialization incomplete
#define ERR_7008 7008 //Unacceptable load of pico_drv.ko
#define ERR_7009 7009 //Pico Card is not ready:
#define ERR_7010 7010 //Flash ROM not ready when expected.
#define ERR_7011 7011 //Error reading Pico Card Configuration information.
#define ERR_7012 7012 //Incorrect signature in %s registers.
#define ERR_7013 7013 //I/O access to AER register failed self test.
#define ERR_7014 7014 //Attrib memory access to AER register failed self test.
#define ERR_7015 7015 //The text 'Pico Computing' was not found in the manufacturer section of attrib memory.
#define ERR_7016 7016 //Data delivered from JTAG spy FIFO is not in the expected sequence.
#define ERR_7017 7017 //BAR registers have incorrect values.
#define ERR_7018 7018 //File %s also linked to BackupBoot.bit
#define ERR_7019 7019 //File %s cannot be linked to this file.
#define ERR_7020 7020 //Serial number cannot be written without a valid primary image .
#define ERR_7021 7021 //Cannot find linked file %s
#define ERR_7022 7022 //File %s Loaded because of AlwaysLoad property of PrimaryBoot.bit.
#define ERR_7023 7023 //RAM test will appear to stall.
#define ERR_7024 7024 //Invalid bits in self test mask (%s).
#define ERR_7025 7025 //Unknown Pico model number '%s'
#define ERR_7026 7026 //Firmware is out of sync with driver: Reload FPGA
#define ERR_7036 7036 //File does not checksum %s.
#define ERR_7037 7037 //Error deleting file %s.
#define ERR_7038 7038 //Error deleting directory %s.
#define ERR_7049 7049 //Invalid setting for debug flags '%s'.
#define ERR_7050 7050 //Pico64.sys or pico.ko is running in debug mode.
#define ERR_7051 7051 //No Tests specified.
#define ERR_7052 7052 //Debug setting will have minimal effect when driver is built in release mode.
#define ERR_7090 7090 //Error reading/writing to device.
#define ERR_7091 7091 //Compare error in picoStream loopback test.
#define ERR_7100 7100 //Unable to find systemRoot in environment.
#define ERR_7101 7101 //Error reported by operating system:
#define ERR_7102 7102 //Error in compilation (%s).
#define ERR_7103 7103 //FPGA has already been booted.
#define ERR_7104 7104 //Attempt to load .chm file failed '%s'
#define ERR_7105 7105 //Error deleting registry key '%s'
#define ERR_7106 7106 //Error accessing Pico Card:
#define ERR_7107 7107 //Error reported by operating system:
#define ERR_7108 7108 //Error reported by operating system:
#define ERR_7109 7109 //Unable to open driver %s
#define ERR_7110 7110 //Unable to reboot.
#define ERR_7111 7111 //Reboot port did not complete successfully.
#define ERR_7112 7112 //Unknown or wrong image loaded.
#define ERR_7113 7113 //Tuple in '%s' not present.
#define ERR_7114 7114 //CIS in '%s' has errors.
#define ERR_7115 7115 //Unable to read CIS.
#define ERR_7116 7116 //Unable to write CIS to file.
#define ERR_7117 7117 //Flash Status bit is misbehaving.
#define ERR_7118 7118 //External program already active.
#define ERR_7119 7119 //Flash ROM did not enter CFI mode correctly '%s'.
#define ERR_7120 7120 //Unable to read file '%s'.
#define ERR_7121 7121 //Unable to write CIS to card.
#define ERR_7122 7122 //Xilinx Image patched to remove WBSTAR=0.
#define ERR_7123 7123 //FPGA image does not return proper signature.
#define ERR_7124 7124 //Pico64.sys did not load correctly.
#define ERR_7125 7125 //Unable to find file for command line request '%s'.
#define ERR_7126 7126 //Primary must be .bin or .bit file '%s'.
#define ERR_7127 7127 //Space is not available in file header for the changes requested '%s'.
#define ERR_7128 7128 //File does not have the proper header '%s'.
#define ERR_7129 7129 //Formatted output from keyhole caused system failure.
#define ERR_7130 7130 //Error writing flash ROM '%s'.
#define ERR_7131 7131 //Nothing to save.
#define ERR_7132 7132 //Error Opening: file %s.
#define ERR_7133 7133 //File is not an FPGA image file (.bit or .bin).
#define ERR_7134 7134 //Image %s does not have keyhole interface.
#define ERR_7135 7135 //File %s has errors and been removed.
#define ERR_7136 7136 //Pico Card to PC Keyhole protocol error.
#define ERR_7137 7137 //Error deleting file %s.
#define ERR_7138 7138 //Sector reference is invalid
#define ERR_7139 7139 //Elf or data file %s has improper size.
#define ERR_7140 7140 //Error accessing driver.
#define ERR_7141 7141 //Unable to find Xilinx signature (FF FF FF FF AA 99 55 66) in file %s.
#define ERR_7142 7142 //Notes exceed sector size.
#define ERR_7143 7143 //Primary boot file not found at logical sector 2.
#define ERR_7144 7144 //Error reading file '%s'.
#define ERR_7145 7145 //Invalid File '%s'.
#define ERR_7146 7146 //Invalid file signature '%s'.
#define ERR_7147 7147 //Unable to create file %s.
#define ERR_7148 7148 //Unable to locate backup image.
#define ERR_7149 7149 //Unable to create primary file %s.
#define ERR_7150 7150 //Failed verify of %s.
#define ERR_7151 7151 //Updated files is not the same size as the file being replaced '%s'.
#define ERR_7152 7152 //Unable to allocate space on flash ROM for file '%s'.
#define ERR_7153 7153 //ELF file %s does not have proper signature.
#define ERR_7154 7154 //Unable to find a primary boot file in %s.
#define ERR_7155 7155 //Incorrect number of bytes returned from Pico Card.
#define ERR_7156 7156 //Incompatible version (non-fatal) '%s'.
#define ERR_7157 7157 //Incompatible version (fatal) '%s'.
#define ERR_7158 7158 //Pico Card requires Windows-7 or better
#define ERR_7159 7159 //Erase of flashROM sector timed out '%s'.
#define ERR_7160 7160 //Error reading flash ROM.
#define ERR_7161 7161 //Error writing PC file %s.
#define ERR_7162 7162 //Error Reading Flash ROM (%s)
#define ERR_7163 7163 //Primary boot file is improperly located '%s'.
#define ERR_7164 7164 //%s is a read-only file.
#define ERR_7165 7165 //Error reading file %s.
#define ERR_7166 7166 //CIS data does not meet consistency checks.
#define ERR_7167 7167 //Invalid data in CIS.
#define ERR_7168 7168 //Missing END marker (0xFF) in CIS data.
#define ERR_7169 7169 ///src/ not found in source file name:%s
#define ERR_7170 7170 //Last tuple in Configuration is not the last tuple entry.
#define ERR_7171 7171 //Error: Multiple linking tuples in CIS chain.
#define ERR_7172 7172 //Duplicate Card Configuration tuple.
#define ERR_7173 7173 //MFC CIS has only one linked tuple.
#define ERR_7174 7174 //Files are identical in a update request.
#define ERR_7175 7175 //File sizes are different in an update request
#define ERR_7176 7176 //Invalid/duplicate environment setting
#define ERR_7177 7177 //invalid IoCtl buffer size
#define ERR_7178 7178 //Error accessing driver
#define ERR_7179 7179 //PCMCIA Error: unsupported ioctl code
#define ERR_7180 7180 //Expecting a string in print statement '%s'.
#define ERR_7181 7181 //Unknown command in a message to the Pico Card '%s'.
#define ERR_7182 7182 //Number and type of parameter is inconsistent with command '%s'.
#define ERR_7183 7183 //Expression has invalid syntax.
#define ERR_7184 7184 //Missing or unbalanced parentheses () in an expression '%s'.
#define ERR_7185 7185 //Contiguous space required for bit image could not be found.
#define ERR_7186 7186 //Space required for primary boot image is already occupied by %s.
#define ERR_7187 7187 //Space required for backup boot image is already occupied by %s.
#define ERR_7188 7188 //Space required for backup boot image is already occupied by %s.
#define ERR_7189 7189 //invalid element in an expression.
#define ERR_7190 7190 //duplicate filename in flash ROM directory '%s'
#define ERR_7191 7191 //Invalid character in file name '%s'
#define ERR_7192 7192 //User cancelled
#define ERR_7193 7193 //Information only.
#define ERR_7194 7194 //Flash file name and PC file name must be the same type '%s'
#define ERR_7195 7195 //Pico Card is not ready or not running a primary image.
#define ERR_7196 7196 //Board at address %s is not available.
#define ERR_7196 7196 //%s Card is not available.
#define ERR_7197 7197 //Pico Card setting (/i) not available on simulated file (/z).
#define ERR_7198 7198 //Feature (%s) is not available in this release.
#define ERR_7199 7199 //Feature (%s) is not available on this hardware.
#define ERR_7200 7200 //Waited too long for reply from Keyhole Test program.
#define ERR_7201 7201 //ELF file does not handle this file type.
#define ERR_7202 7202 //The file '%s' must be an elf file (.elf).
#define ERR_7203 7203 //Invalid header on ELF file.
#define ERR_7204 7204 //Process on PPC finished.
#define ERR_7205 7205 //Invalid Entry address in ELF file
#define ERR_7206 7206 //PPC program has terminated (information).
#define ERR_7207 7207 //Flash ROM has been changed by another program.
#define ERR_7208 7208 //Pico Card has been rebooted.
#define ERR_7209 7209 //Program %s started.
#define ERR_7210 7210 //PPC Timeout waiting for input from PC; input status=%s.
#define ERR_7211 7211 //No input received from PC; input fifo status=%s.
#define ERR_7212 7212 //PPC command (%s) to PC timed out.
#define ERR_7213 7213 //PPC Out of sequence. Received word count=%s
#define ERR_7214 7214 //Keyhole services limited (%s).
#define ERR_7215 7215 //Command to KeyholeTest.elf is incorrect. Command=%s
#define ERR_7216 7216 //Capability not found  '%s'.
#define ERR_7217 7217 //Error in Kprintf on PPC.
#define ERR_7218 7218 //Unable to locate file KeyholeTest.elf.
#define ERR_7219 7219 //RAM test failed at location %s.
#define ERR_7220 7220 //RAM address is outside memory available on Pico Card.
#define ERR_7221 7221 //File must be type BIT to be rebooted; found to be of type %s.
#define ERR_7222 7222 //File must be type ELF to be executed.
#define ERR_7223 7223 //File size rounded up to %s bytes (ie. multiple of physical sector size)
#define ERR_7224 7224 //Error reading RAM
#define ERR_7230 7230 //Unable to load linked file
#define ERR_7231 7231 //Unexpected exit from a loaded program
#define ERR_7234 7234 //Linked file not found
#define ERR_7250 7250 //Unable to add another custom tool
#define ERR_7251 7251 //Command field in a custom tool does not refer to a valid program name '%s'
#define ERR_7252 7252 //directory specified by a custom tool or wizard is not valid '%s'
#define ERR_7253 7253 //Unable to find %s in path.
#define ERR_7254 7254 //Should be exactly one build.bat file near specified binary file.
#define ERR_7255 7255 //Invalid script command:%s
#define ERR_7290 7290 //Unable to locate :%s
#define ERR_7291 7291 //Unable to locate entry point '%s' in library module.
#define ERR_7292 7292 //Unacceptable version of '%s'
#define ERR_7293 7293 //OS error loading executable module '%s'.
#define ERR_7294 7294 //OS error loading DLL '%s'.
#define ERR_7295 7295 //Incorrect side-by-side directories for Visual Studio Express
#define ERR_7296 7296 //Invalid file name '%s'
#define ERR_7297 7297 //Write Permission denied '%s'
#define ERR_7298 7298 //Unable to determine name of current image.
#define ERR_7299 7299 //Unacceptable serial number '%s'.
#define ERR_7300 7300 //Bad parameter to constructor cPicoStream() '%s'.
#define ERR_7301 7301 //Pico Failed Installation Write test.
#define ERR_7302 7302 //Pico Failed Installation Read test.
#define ERR_7303 7303 //E-12 card is operating in hobbled mode.
#define ERR_7304 7304 //Not connected to a Pico Card.
#define ERR_7305 7305 //Please select exactly one card/file for this operation.
#define ERR_7306 7306 //Unable to find help file '%s'
#define ERR_7307 7307 //DCMs failed to lock.
#define ERR_7308 7308 //Pico signature not found in firmware.
#define ERR_7309 7309 //Powerdown rejected because another application has the device driver open.
#define ERR_7310 7310 //Error powering down JTAG device.
#define ERR_7311 7311 //Error powering up JTAG device.
#define ERR_7312 7312 //Error powering down Pico device .
#define ERR_7313 7313 //Error powering up Pico device .
#define ERR_7314 7314 //Error powering down MF device .
#define ERR_7315 7315 //Error powering up MF device .
#define ERR_7316 7316 //Current FPGA file (%s) does not provide JTAG spy.
#define ERR_7317 7317 //Attempt to write a read only Stream %s
#define ERR_7318 7318 //Attempt to read a write only Stream %s
#define ERR_7319 7319 //Pico card is opened in read only mode '%s'
#define ERR_7320 7320 //Pico card or simulation file is opened in read only mode.
#define ERR_7321 7321 //Timeout waiting for Stream interrupt.
#define ERR_7322 7322 //Error Creating interrupt event.
#define ERR_7330 7330 //Bus Master interface not ready.
#define ERR_7331 7331 //Bus Master counter test register did not increment properly.
#define ERR_7332 7332 //Bus Master Read or Write to counter register failed.
#define ERR_7333 7333 //Interrupts are not operational. System is dropping back to non interrupt mode.
#define ERR_7334 7334 //Data error in Bus Master counter test.
#define ERR_7335 7335 //Bus Master pacing register did not return proper signature.
#define ERR_7336 7336 //Bus Master timeout test did not return the correct number of bytes.
#define ERR_7337 7337 //Bus Master interrupt not specified.
#define ERR_7338 7338 //Interrupts not received at the expected rate.
#define ERR_7339 7339 //Bus Master did not timeout correctly.
#define ERR_7340 7340 //Unable to determine number of JTAG devices or number of devices is incorrect.
#define ERR_7341 7341 //Invalid scan location provided to the function PicoMface::JtagGetIdCode().
#define ERR_7342 7342 //Incorrect ID code returned from device %s on JTAG chain.
#define ERR_7343 7343 //Too many open channels.
#define ERR_7344 7344 //Unable to open Stream.
#define ERR_7345 7345 //Unable to set Stream number '%s'.
#define ERR_7346 7346 //Unable to set base address.
#define ERR_7347 7347 //Unable to set Stream size.
#define ERR_7350 7350 //Board is unserialized, or has an invalid serial number.
#define ERR_7351 7351 //Board does not have model number information in secure sector.
#define ERR_7352 7352 //Serial number cannot be written without a valid primary image .
#define ERR_7353 7353 //FPGA file is not a valid bit file or is not compatible with the FPGA (%s) on the Pico Card.
#define ERR_7354 7354 //Unable to read the part number from the JTAG port.
#define ERR_7355 7355 //data in secure sector conflicts with current data and cannot be updated.
#define ERR_7356 7356 //data did not update secure sector correctly.
#define ERR_7357 7357 //Unable to determine FPGA model. Action Performed notwithstanding.
#define ERR_7358 7358 //Unknown variable name:%s
#define ERR_7359 7359 //Register $0 invalid in this context
#define ERR_7380 7380 //Clearing RAM before initiating a program will make dump considerably faster
#define ERR_7381 7381 //Debug switches are not implemented in release version of Pico64.sys
#define ERR_7381 7381 //Debug switches are not implemented in release version of Pico.ko
#define ERR_7400 7400 //Directory name is invalid or too short (three characters) '%s'
#define ERR_7401 7401 //Unable to find or create specified directory '%s'
#define ERR_7402 7402 //Non empty directory already exists '%s'
#define ERR_7403 7403 //Type field must be three or four characters in length '%s'
#define ERR_7404 7404 //Invalid command in WizardCommand.txt '%s'
#define ERR_7449 7449 //%s command does not apply to this Pico Card
#define ERR_7450 7450 //Error in PLX chip interface, function=%s.
#define ERR_7451 7451 //DONE did not go high after sending configuration bitstream
#define ERR_7452 7452 //FPGA is not correctly loaded.
#define ERR_7453 7453 //INIT didn't go inactive when PROG was driven low
#define ERR_7454 7454 //INIT didn't go high after PROG was driven high
#define ERR_7455 7455 //gpio state unexpected:%s
#define ERR_7456 7456 //Error from PlxPci.sys:%s
#define ERR_7457 7457 //PLX_CHANNEL0_DONE not asserted when writing FPGA load information:%s
#define ERR_7458 7458 //Pico64.sys was unable to locate the PCI bridge.
#define ERR_7459 7459 //Error Accessing EEprom on %s
#define ERR_7460 7460 //Error Loading FPGA
#define ERR_7461 7461 //Error Loading FPGA after several retries
#define ERR_7500 7500 //No compatible pico card found!
#define ERR_7501 7501 //Bluetooth maximum pin length exceeded
#define ERR_7502 7502 //Missing imported information file
#define ERR_7503 7503 //Missing user defined information addresses
#define ERR_7504 7504 //No Pico Card found meeting specified criteria %s
#define ERR_7505 7505 //File is not a valid .zip file
#define ERR_7506 7506 //unable to find encrypted files in (%s).zip!
#define ERR_7507 7507 //Unable to find password.
#define ERR_7508 7508 //No dictionary selected.
#define ERR_7509 7509 //Unable to identify the PSK from the dictionary file.
#define ERR_7510 7510 //Encrypted file not selected.
#define ERR_7511 7511 //Specified SSID and the SSID in the output file do not match.
#define ERR_7512 7512 //Unable to open capture file.
#define ERR_7513 7513 //End of pcap capture file.
#define ERR_7514 7514 //Invalid word length.
#define ERR_7515 7515 //Found a record that was too short.
#define ERR_7516 7516 //Invalid passphrase length.
#define ERR_7517 7517 //No BSSID selected.
#define ERR_7518 7518 //Please select only one dictionary name to change.
#define ERR_7519 7519 //No PicoCard found. Using PC for cracks.
#define ERR_7520 7520 //Cannot connect to internet. Email notification cannot be sent.
#define ERR_7521 7521 //SMTP error.
#define ERR_7522 7522 //Cannot run more than one of the same kind of crack at a time.
#define ERR_7523 7523 //A crack is already running in a selected row.
#define ERR_7524 7524 //Pico Card must be E16 or EX300.
#define ERR_7525 7525 //This algorithm is not supported on the E101 yet.
#define ERR_7526 7526 //No Pico Cards found
#define ERR_7600 7600 //DMA failed to complete in timely fashion
#define ERR_7601 7601 //ReadPLX8111Regs failed to read
#define ERR_7602 7602 //GetPicoConfig failed to do ioctl
#define ERR_8000 8000 //ReadPLX8111Regs failed to read
#define ERR_8001 8001 //ReadPLX8111Regs failed to read
#define ERR_8002 8002 //GetPicoConfig failed to do ioctl
#define ERR_8003 8003 //FindPico didn't find any matching cards
#define ERR_8004 8004 //Bitfile %s could not be opened
#define ERR_8005 8005 //Bitfile %s could not be read
#define ERR_8006 8006 //FPGA download of %s to E101 completed with errors
#define ERR_8007 8007 //BitFileCompat: PICO_CONFIG argument is NULL
#define ERR_8008 8008 //Unable to open cPicoStream with the specified properties '%s'
#define ERR_8009 8009 //Bitfile could not be opened
#define ERR_8010 8010 //undeclared identifier: %s
#define ERR_8011 8011 //Driver doesn't support slot queries. Unable to determine the Pico's slot.
#define ERR_8012 8012 //Insufficient buffer space provided for slot string. Unable to report the Pico's slot.
#define ERR_8013 8013 //FPGA and .bit are incompatible %s
#define ERR_8014 8014 //All compatible Pico cards are in use.
#define ERR_8015 8015 //BitFileFPGAType doesn't have enough buffer space to handle this .bit file '%s'.
#define ERR_8016 8016 //Specified UserClk doesn't exist.
#define ERR_8017 8017 //Can't generate requested frequency.
#define ERR_8018 8018 //Invalid DCM multiply or divide parameter.
#define ERR_8019 8019 //DCM failed to lock at requested clock speed.
#define ERR_8020 8020 //Couldn't read from .bit file '%s'
#define ERR_8021 8021 //The .bit file is invalid '%s'
#define ERR_8022 8022 //The .bit file is invalid '%s'
#define ERR_8023 8023 //Internal error: driver BYTES_AVAILABLE called on firmware too old to support it!
#define ERR_8024 8024 //Can't call GetBytesAvailable on a stream that's not open
#define ERR_9000 9000 //CYapi.lib not available for 64bit platforms
#define ERR_9001 9001 //Failed to send initial message from host to E101
#define ERR_9002 9002 //File %s did not pass syntax check
#define ERR_9003 9003 //Miscommunication between CyAPI.lib and cPicoStream_e101
#define ERR_9004 9004 //Failure to USB re-enumerate correctly
#define ERR_9005 9005 //Could not establish a valid endpoint loading E101
#define ERR_9006 9006 //Endpoint is invalid
#define ERR_9007 9007 //Unable to find '%s' in environment
#define ERR_9008 9008 //FPGA download completed with errors in %s
#define ERR_9009 9009 //Invalid software call
#define ERR_9010 9010 //Invalid PSRAM controller signature
#define ERR_9011 9011 //Processor architecture not recognized
#define ERR_9012 9012 //Failed to create to modify a registry record
#define ERR_9013 9013 //After reboot, the PCIe address failed to reappear.
#define ERR_9014 9014 //function not in debug version only
#define ERR_9015 9015 //The current user lacks the OS rights to perform a reboot
#define ERR_9015 9015 //The current user lacks the OS rights to perform a reboot
#define ERR_9016 9016 //Operating System error loading pico card driver
#define ERR_9100 9100 //Bad read stream signature in firmware
#define ERR_9101 9101 //Interrupted while waiting on stream
#define ERR_9102 9102 //Can't set exclusive mode - already exclusive to another process.
#define ERR_9103 9103 //Missed DMA IRQ.
#define ERR_9104 9104 //Bad write stream signature in firmware
#define ERR_9105 9105 //Interrupted while waiting on stream
#define ERR_9106 9106 //FPGA was not configured successfully.
#define ERR_9200 9200 //Can't locate EX500 for JTAG access.
#define ERR_9205 9205 //Can't connect to EX500 Spartan for JTAG access.
#define ERR_9206 9206 //Failed to load the EX500 Spartan for JTAG access.
#define ERR_9207 9207 //EX500 Spartan for JTAG has bad status.
#define ERR_9208 9208 //Card didn't come back up after JTAG loading.
#define ERR_9209 9209 //Bad write data alignment or size.
#define ERR_9210 9210 //Internal driver error: bad card model in rctl/wctl.
#define ERR_9211 9211 //FPGA didn't report correct sequence number to host.
#define ERR_9212 9212 //Reloading the FPGA on a system that doesn't properly support hotswap is only supported with version 5.x firmware
#define ERR_9213 9213 //Bad PCI vendor ID after reload.
#define ERR_9214 9214 //This FPGA is unconfigured.
#define ERR_9215 9215 //Couldn't re-allocate the interrupt after an FPGA reload without hotswapping.
#define ERR_9216 9216 //Memory reset command not properly written to stream.
#define ERR_9217 9217 //Invalid memory identifier.
#define ERR_9218 9218 //Memory calibration timed out.
#define ERR_9219 9219 //RAM command not properly written to stream.
#define ERR_9220 9220 //Error reading data from memory.
#define ERR_9221 9221 //Error writing data to memory.
#define ERR_9222 9222 //Size of parameter to iocontrol function is invalid.
#define ERR_9995 9995 //Internal software error: %s
#define ERR_9996 9996 //Function %s is not implemented under this O/S
#define ERR_9997 9997 //Function %s is not implemented on this platform
#define ERR_9998 9998 //%s Not yet coded
#define ERR_9999 9999 //Not an error
#ifndef NULL
   #define NULL 0
#endif 

#define ERR_WOOF_WOOF 0x40000000 //see MadErrorBark() forces error message to use wxMessageBox

//Structure to manage last error; this prevents multiple calls to logError losing context.
#ifdef __cplusplus 
class cSamError
   {public:
    int   GetLastError    (void);                                  //
    int   LogError        (int erC, const char *contextP=NULL, const char *paramP=NULL); //record an error; saves error#, contextP & parameter.
    int   PrintError      (int erC, const char *contextP=NULL, const char *paramP=NULL);
    int   Severity        (int erC);                               //return severity of error code (XSEVERITY_WARNING, XSEVERITY_ERROR etc).
    char *FullError       (int erC, char *resultP, int resultSize);//Return full error text with parameter substitution.
    char *ShortError      (int erC, char *resultP, int resultSize);//Return summary error text with parameter substitution.
    int   PrepareError    (int erC, char *explanationP, int sizeof_explanation, char *contextP, int sizeof_context, char *locationP, int sizeof_location);
    void  Clear           (void);                                  //Clear all error codes in m_errorContext[], & m_errorParam[].
    int   GetLastErrorInfo(char **paramPP, char **contextPP, int erri=-1);
    void  AddContext      (const char *msgP);                      //Concatenate msgP to context of last error.
    void  AddLocation     (const char *msgP);                      //Location in user source of error
    char *ShowOSerror     (char *bufP, int bufSize);               //Get error from operating system.
    int LocateError(int erC);
    const char *LocateErrorP(int erC);
    private:
    #if ERROR_HISTORY_DEPTH != 1
        int         m_lastErr;
        static char m_errorContext [ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE],
                    m_errorLocation[ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE],
                    m_errorParam   [ERROR_HISTORY_DEPTH][MAX_ERROR_PARAM_SIZE];
        static int  m_lastErrorCode[ERROR_HISTORY_DEPTH], // = {0};
                    m_lastErrInx;
    #else
        SAM_LAST_ERROR     m_lastErr[ERROR_HISTORY_DEPTH];
        int m_lastErrInx;
        #define M_LAST_ERR m_lastErr[m_lastErrInx]
        #define M_LASTERR(ii) m_lastErr[i]
        #define m_lastErrInx 0
    #endif
   }; //class cSamError...

#endif //__cplusplus ...
#endif //_SAM_ERROR_FILE_INCLUDED

//end of file
