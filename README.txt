DESCRIPTION:
This executable is used to load an xml recipe into enVision.
This will parse an xml file, 
load the eva file,
load the stdf parameters of the test program.

Currently the SDR and MIR STDF paramters are sent to the system 
software after the test program loads.
This can be found in the file userInterface.cpp
Functions CuserEvxaInterface::sendSDRParams()
and  CuserEvxaInterface::sendMIRParams()
The test program parameters are parsed but and sent to the system software.
This can be changed to not send the test program paramters in the function
CuserEvxaInterface::sendTPParams()

TO BUILD THE EXECUTAGBLE:
Prerequisite:
Need CURI_1.12_1.101 installed to build and run this executable.
Verify that c_makefile is linked to /opt/ateTools/curi/CURI/dev/makeTemplates/makefile.curi

From optool select Tools->xterm
In that xterm cd to the recipeHandler directory.
In the recipeHandler directory run the command 
Default Debug build:
make
Release build:
make CFG=Release

This will build recipeHandler in the sub-directory Debug or Release

TO LAUNCH THE EXECUTABLE:
In the xterm from optool cd to the directory that has the executable.
You can rename the executable to something more meaningful to you.
Execute the command to run the evcxaDemo executable.
recipeHandler -t <tester_name> -c
To see additional debug data execute
recipeHandler -t <tester_name> -c -d
This should be done after restart_tester but before PP_SELECT.


ADDING ADDITIONAL PARSING CAPABILITY
userInterface.cpp has all the parsing functions.
parseXML(const char*recipe_text) is the top level of the xml parser.
parseXML finds testerRecipe and finds the number of children from that node
and for each known child calls a function to parse that known child.
Then each child node finds it's children and for each known child
calls a function to parse that known child.
This continues until we get to a node that has the data.

While parsing the xml file an "unkown child" message will appear
when a child node is encountered and there no function to handle 
the parsing of that child.  This function is what is needed to be added.
The function is written based on what is in the xml file.
The same strategy of a for loop for the number of children 
should be used when adding new functions.  This will keep the program
modular and easier to add capability.

In userInterface.h you will see structure definitions for the data to be 
stored. Then a member of each structure is declared in CuserEvxaInterface 
and that is how the parsed data is stored to be used later.
When adding new elements in the xml data, add a new structure to hold the data
contained in the new element.
Also Add clear<element>Params and send<element>Params functions in 
userInterface.cpp to initialize and send the data to the tester.

A example would be clearMIRParams() and sendMIRParams().
See the structure that is used to hold the MIR data.
The function parseMIR(...) will stuff the data structure
with data from the xml file.
A similar approach needs to be taken for new data coming 
from the xml.


./Debug/recipeHandler -t localuser_u1703_DMDx_sim -d -conf ./recipeHandler_config.xml


Next Debug #goals20180413
- 	in updateTestProgramData(), sendEndOfWafer(), sendEndOfLot(), sendStartOfWafer(), sendStartOfLot() are commented out
	try enabling it and see its effects in ST. also try to understand what the hell these functions do...






