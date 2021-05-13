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
    bool badState = false;
    //verify if input file exist
    std::ifstream ifs;
    ifs.open(inName.c_str(), std::ifstream::in);
    badState = ifs.fail();
    ifs.close();
    if (badState)
    {
        std::cout << "Error can't open " << inName << std::endl;
        //return false;
    }

    std::ofstream out("rik_out.txt");
    std::streambuf *coutbuf = std::cout.rdbuf(); //save old buf
    std::cout.rdbuf(out.rdbuf()); //redirect std::cout to out.txt!

    dx_data fData;
    dx_iface *input = new dx_iface();
    badState = input->printText(inName, &fData);

    std::cout.rdbuf(coutbuf); //reset to standard output again
    out.close();

    extractSvg(input);

    if (!badState)
    {
        std::cout << "Error reading file " << inName << std::endl;
    }
    delete input;

    return badState;
}

int main(int argc, char *argv[])
{
    std::string fileName;

    if (argc != 2)
        usage();
    else
        fileName = argv[1];

    bool ok = fileName.length() > 0 ? extractText(fileName) : false;
    if (ok)
        return 0;
    return 1;
}
