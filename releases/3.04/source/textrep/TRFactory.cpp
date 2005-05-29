//======== (C) Copyright 2002 Charles G. Cleveland All rights reserved. =========
//
// The copyright to the contents herein is the property of Charles G. Cleveland.
// The contents may be used and/or copied only with the written permission of
// Charles G. Cleveland, or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile: TRFactory.cpp $
// $Date: 2002/08/16 02:28:25 $
//
//-------------------------------------------------------------------------------
// $Log: TRFactory.cpp,v $
// Revision 1.6  2002/08/16 02:28:25  Flayra
// - Added document headers
//
//===============================================================================

#include "textrep/TRTag.h"
#include "textrep/TRTagValuePair.h"
#include "textrep/TRDescription.h"
#include "textrep/TRFactory.h"
#include "util/STLUtil.h"

const int maxLineLength = 256;

bool TRFactory::ReadDescriptions(const string& inRelativePathFilename, TRDescriptionList& outDescriptionList)
{
	bool theSuccess = false;
	bool theDescriptionRead = false;

    // Open file specified by relative path name
    fstream infile;
    infile.open(inRelativePathFilename.c_str(), ios::in);

    if(infile.is_open())
    {
		do
		{
			// Try to read the next description in
			TRDescription theNextDescription;
			theDescriptionRead = ReadDescription(infile, theNextDescription);

	        // add it to the description list
			if(theDescriptionRead)
			{
				// Function is successful if at least one description was found
				outDescriptionList.push_back(theNextDescription);
				theSuccess = true;
			}

		} while(theDescriptionRead);

        infile.close();
    }
	return theSuccess;
}

bool TRFactory::WriteDescriptions(const string& inRelativePathFilename, const TRDescriptionList& inDescriptionList, const string& inHeader)
{
    bool theSuccess = false;

    // Open the file for output
    fstream theOutfile;
    theOutfile.open(inRelativePathFilename.c_str(), ios::out);

    if(theOutfile.is_open())
    {
        // Optional: write header
        theOutfile << inHeader << std::endl;

        //theOutfile << "; Generated by Half-life.  Do not edit unless you know what you are doing! " << std::endl;
        //theOutfile << std::endl;

        // For each description
        TRDescriptionList::const_iterator theIter;
        for(theIter = inDescriptionList.begin(); theIter != inDescriptionList.end(); theIter++)
        {
            // Write out that description
            const TRDescription& theDesc = *theIter;
            TRFactory::WriteDescription(theOutfile, theDesc);

            // Write out a blank line to make them look nice and separated
            theOutfile << std::endl;
        }

        theOutfile.close();
        theSuccess = true;
    }

    return theSuccess;
}

// TODO: Add case-insensitivity
bool TRFactory::ReadDescription(fstream& infile, TRDescription& outDescription)
{
    bool theSuccess = false;
    string currentLine;
    bool blockStarted = false;

    // for every line in the file
    while(!infile.eof())
    {
        if(readAndTrimNextLine(infile, currentLine))
        {
            // If line isn't a comment
            if(!lineIsAComment(currentLine))
            {
                // If we haven't started, is line of format: start <type> <name>?  If so, set started and set those tags.
                if(!blockStarted)
                {
                    blockStarted = readStartBlockLine(currentLine, outDescription);
                }
                // If we have started
				else
                {
                    // Is line an end?  If so, this function is over
                    if(readEndBlockLine(currentLine))
                    {
                        break;
                    }
                    // else is line of tag = property format?  If so, add it as pair
                    else
                    {
                        // If not, print error and proceed
                        if(readTagAndValueLine(currentLine, outDescription))
                        {
                            // Once we have read at least one tag-value pair, considered this a success
                            theSuccess = true;
                        }
                        else
                        {
			  //printf("Error reading line of length %d: %s\n", currentLine.length(), currentLine.c_str());
                        }
                    }
                }
            }
        }
    }

    return theSuccess;
}

bool TRFactory::WriteDescription(fstream& outfile, const TRDescription& inDescription)
{
    bool theSuccess = true;

    // Write out the start block
    outfile << "start" << " " << inDescription.GetType() << " " << inDescription.GetName() << std::endl;

    // Write out the property tags
    TRDescription::const_iterator theIter;
    for(theIter = inDescription.begin(); theIter != inDescription.end(); theIter++)
    {
        outfile << "    " << theIter->first << " = " << theIter->second << std:: endl;
    }

    // Write out the end block.
    outfile << "end" << std::endl;

    return theSuccess;
}

bool TRFactory::readAndTrimNextLine(istream& inStream, string& outString)
{
    char theLine[maxLineLength];
    bool theSuccess = false;

    inStream.getline(theLine, maxLineLength);
    outString = string(theLine);

    trimWhitespace(outString);

	// Return false if the line is empty when we're done
	if(outString.length() > 1)
	{
		theSuccess = true;
	}

    return theSuccess;
}

// Trim whitespace from string
void TRFactory::trimWhitespace(string& inString)
{
	// find first character that isn't a tab or space and save that offset
	//int		firstNonWSChar = 0;
	//int		i= 0;
	//int		stringLength = inString.length();
	//while(i != (stringLength - 1) && ())
	//{
	//}

	// find last character that isn't a tab or space and save that offset
	// Build a new string representing string without whitespace
	// Set new string equal to inString
}

bool TRFactory::charIsWhiteSpace(char inChar)
{
	bool theSuccess = false;

	if((inChar == ' ') || (inChar == '\t'))
	{
		theSuccess = true;
	}

	return theSuccess;
}

// Is the string a comment?
bool TRFactory::lineIsAComment(const string& inString)
{
    bool theLineIsAComment = false;
	int theLength = inString.length();

	if(theLength > 0)
	{
		// Loop through inString until a non-whitespace character is found
		for(int i = 0; i < theLength; i++)
		{
			char theFirstChar = inString[i];
			if(!charIsWhiteSpace(theFirstChar))
			{
				if((theFirstChar == '\'') || (theFirstChar == ';'))
				{
					theLineIsAComment = true;
				}
				break;
			}
		}
	}

    return theLineIsAComment;
}

// Read start block
// Set the name and type of the description
// Returns false if invalid format
bool TRFactory::readStartBlockLine(const string& inString, TRDescription& outDescription)
{
    bool theSuccess = false;
	char theType[maxLineLength];
	char theName[maxLineLength];

	memset(theType, ' ', maxLineLength);
	memset(theName, ' ', maxLineLength);

	// Read three tokens.  There should be "start" <type> <name>
	if(sscanf(inString.c_str(), "start %s %s", theType, theName) == 2)
	{
		outDescription.SetName(theName);
		outDescription.SetType(theType);
		theSuccess = true;
	}

    return theSuccess;
}

// Read end block
bool TRFactory::readEndBlockLine(const string& inString)
{
    bool theSuccess = false;

	// There are some CRLF issues on Linux, hence this bit
	if(inString.length() >= 3)
	{
		string theString = inString.substr(0,3);

		if(theString == "end")
		{
			theSuccess = true;
		}
		else
		{
			//printf("TRFactory::readEndBlockLine() failed, found (%s)\n", theString.c_str());
		}
	}

    return theSuccess;
}

bool TRFactory::readTagAndValueLine(const string& inString, TRDescription& outDescription)
{
    bool theSuccess = false;
	char theTag[maxLineLength];
	char theValue[maxLineLength];

	// Zero them out
	memset(theTag, ' ', maxLineLength);
	memset(theValue, ' ', maxLineLength);

	if((sscanf(inString.c_str(), "%s = %s", theTag, theValue)) == 2)
	{
		// Add it
		TRTagValuePair thePair(theTag, theValue);
		outDescription.AddPair(thePair);
		theSuccess = true;
	}

    return theSuccess;
}



