/******************************************************************************
**  dwg2text - Program to extract text from dwg/dxf                          **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <stdarg.h>  // For va_start, etc.

#include "dx_iface.h"
#include "dx_data.h"
#include <algorithm>
#include "write_svg.h"

void usage()
{
    std::cout << "Usage: " << std::endl;
    std::cout << "   dwg2text <input>" << std::endl << std::endl;
    std::cout << "   input      dwg or dxf file to extract text" << std::endl;
}

bool extractText(std::string inName)
{
    //verify if input file exist
    std::ifstream ifs;
    ifs.open(inName.c_str(), std::ifstream::in);
    auto badState = ifs.fail();
    ifs.close();
    if (badState)
    {
        std::cout << "Error can't open " << inName << std::endl;
        return false;
    }

	bool ok = false;
	std::string outFileName = inName.c_str();
	int ext = outFileName.find_last_of('.');
	if (ext > 0) outFileName = outFileName.substr(0, ext);


#ifdef DEBUG
    std::ofstream out = std::ofstream((outFileName + ".txt"));
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!
#endif

    dx_data fData;
    dx_iface *input = new dx_iface();
#ifdef DEBUG
	ok = input->printText(inName, &fData);
#else
	ok = input->openRead(inName, &fData);
#endif

#ifdef DEBUG
    std::cout.rdbuf(coutbuf); //reset to standard output again
    out.close();
#endif


	//if (ok)
		extractSvg(input, outFileName.c_str());

    if (!ok)
        std::cout << "Error reading file " << inName << std::endl;

    delete input;
    return ok;
}

int main(int argc, char *argv[])
{
    std::string fileName;

    if (argc != 2)
        usage();
    else
        fileName = argv[1];

    bool ok = false;
    if (fileName.length() > 0)
    {
        std::cout << "reading " << fileName << std::endl;
        ok = extractText(fileName);
    }

	std::cout << "press any key to exit... " << std::endl;
    std::cin.get();
    if (ok)
        return 0;
    return 1;
}
