/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
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
#include <algorithm>
#include "dx_iface.h"
#include "../libdwgr.h"
#include "../libdxfrw.h"

bool dx_iface::openRead(const std::string& fileI, dx_data *fData)
{
    unsigned int found = fileI.find_last_of(".");
    std::string fileExt = fileI.substr(found + 1);
    std::transform(fileExt.begin(), fileExt.end(), fileExt.begin(), ::toupper);
    cData = fData;
    currentBlock = cData->mBlock;

    bool success = false;
    if (fileExt == "DXF")
    {
        //loads dxf
        dxfRW* dxf = new dxfRW(fileI.c_str());
       // dxf->setDebug(DRW::DEBUG);
        success = dxf->read(this, false);
        delete dxf;
    }
    else if (fileExt == "DWG")
    {
        //loads dwg
        dwgR* dwg = new dwgR(fileI.c_str());
       // dwg->setDebug(DRW::DEBUG);
        success = dwg->read(this, false);
        delete dwg;
    }

    return success;
}



#define PID(e) " hid:" << static_cast<DRW_Entity*>(e)->handle << " pid:" << static_cast<DRW_Entity*>(e)->parentHandle <<  " lyr:" << static_cast<DRW_Entity*>(e)->layer

void dx_iface::PrintEntity(DRW_Entity * &e)
{
    std::cout << ">> type: " << e->eType << PID(e) << std::endl;

    switch (e->eType)
    {
        case DRW::ARC:
            break;
        case DRW::LINE:
            //std::cout << "    (_Line): "
            //    << " fx:" << static_cast<DRW_Line*>(e)->basePoint.x
            //    << " fy:" << static_cast<DRW_Line*>(e)->basePoint.y
            //    << " fz:" << static_cast<DRW_Line*>(e)->basePoint.z
            //    << " tx:" << static_cast<DRW_Line*>(e)->secPoint.x
            //    << " ty:" << static_cast<DRW_Line*>(e)->secPoint.y
            //    << " tz:" << static_cast<DRW_Line*>(e)->secPoint.z
            //    << PID(e)
            //    << std::endl;
            break;
        case DRW::LWPOLYLINE:
            //std::cout << "    (_LWPolyline): " << static_cast<DRW_LWPolyline*>(e)->vertexnum << std::endl;
            break;
        case DRW::SPLINE:
            //std::cout << "    (_Spline): " << static_cast<DRW_Spline*>(e)->nknots << std::endl;
            break;
        case DRW::CIRCLE:
            //std::cout << "    (_Circle): " << static_cast<DRW_Circle*>(e)->radious << std::endl;
            break;
        case DRW::HATCH:
            //std::cout << "    (_Hatch): " << static_cast<DRW_Hatch*>(e)->name << std::endl;
            break;
        case DRW::DIMENSION:
            std::cout << "    (_Dimension): " << static_cast<DRW_Dimension*>(e)->type << PID(e) << std::endl;
            break;
        case DRW::MTEXT:
            std::cout << "  MText: " << static_cast<DRW_MText*>(e)->text << PID(e) << std::endl;
            break;
        case DRW::TEXT:
            if (static_cast<DRW_Text*>(e)->text == "101")
                int x = 1;
            std::cout << "  Text: " << static_cast<DRW_Text*>(e)->text << PID(e) << std::endl;
            break;
        case DRW::VIEWPORT:
            std::cout << "VIEWPORT: " << static_cast<DRW_Viewport*>(e)->vpID << std::endl;
            break;
        case DRW::INSERT:
            std::cout << "Insert: " << static_cast<DRW_Insert*>(e)->name 
                << " blockRecH: " << static_cast<DRW_Insert*>(e)->blockRecH.ref 
                << " blockRecH: " << ((duint32)static_cast<DRW_Insert*>(e)->blockRecH.code)
                << PID(e) << std::endl;
            break;
        case DRW::BLOCK:
            std::cout << "Block: " << static_cast<DRW_Block*>(e)->name << PID(e) << std::endl;
            break;
        default:
            std::cout << "> type: " << e->eType << PID(e) << std::endl;
            break;
    }
}

bool dx_iface::printText(const std::string& fileI, dx_data *fData)
{
	bool success = openRead(fileI, fData);

    if (success || true)
    {
        std::cout << "----------------" << std::endl;
        std::cout << "---- layers -----" << std::endl;

        for (std::list<DRW_Layer>::const_iterator it =
             cData->layers.begin(); it != cData->layers.end();
             ++it)
        {
            auto e = *it;
            std::cout << "  tType: " << e.tType << "; name:" << e.name << "; hid:" << e.handle << std::endl;
        }

        std::cout << "----------------" << std::endl;
        std::cout << "---- bocks -----" << std::endl;

        for (std::list<dx_ifaceBlock*>::const_iterator it =
             cData->blocks.begin(); it != cData->blocks.end();
             ++it)
        {
            auto e = *it;
            
            std::cout << " ### type: " << e->eType << "; name:" << e->name << "; hid:" << e->handle << "; pid:" << e->parentHandle << std::endl;

            for (std::list<DRW_Entity*>::const_iterator it2 =
                 e->ent.begin(); it2 != e->ent.end();
                 ++it2)
            {
                auto ee = (*it2);
                PrintEntity(ee);
            }
            std::cout << "--------" << std::endl;
        }
    }
    if (success || true)
    {
        std::cout << "----------------" << std::endl;
        std::cout << "---- mBlock -----" << std::endl;
        for (std::list<DRW_Entity*>::const_iterator it =
             cData->mBlock->ent.begin(); it != cData->mBlock->ent.end();
             ++it)
        {
            DRW_Entity* e = *it; 
            PrintEntity(e);
        }
    }
    return success;
}

void dx_iface::writeEntity(DRW_Entity* e)
{
    switch (e->eType)
    {
        case DRW::POINT:
            dxfW->writePoint(static_cast<DRW_Point*>(e));
            break;
        case DRW::LINE:
            dxfW->writeLine(static_cast<DRW_Line*>(e));
            break;
        case DRW::CIRCLE:
            dxfW->writeCircle(static_cast<DRW_Circle*>(e));
            break;
        case DRW::ARC:
            dxfW->writeArc(static_cast<DRW_Arc*>(e));
            break;
        case DRW::SOLID:
            dxfW->writeSolid(static_cast<DRW_Solid*>(e));
            break;
        case DRW::ELLIPSE:
            dxfW->writeEllipse(static_cast<DRW_Ellipse*>(e));
            break;
        case DRW::LWPOLYLINE:
            dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(e));
            break;
        case DRW::POLYLINE:
            dxfW->writePolyline(static_cast<DRW_Polyline*>(e));
            break;
        case DRW::SPLINE:
            dxfW->writeSpline(static_cast<DRW_Spline*>(e));
            break;
    //    case RS2::EntitySplinePoints:
    //        writeSplinePoints(static_cast<DRW_Point*>(e));
    //        break;
    //    case RS2::EntityVertex:
    //        break;
        case DRW::INSERT:
            dxfW->writeInsert(static_cast<DRW_Insert*>(e));
            break;
        case DRW::MTEXT:
            dxfW->writeMText(static_cast<DRW_MText*>(e));
            break;
        case DRW::TEXT:
            dxfW->writeText(static_cast<DRW_Text*>(e));
            break;
        case DRW::DIMLINEAR:
        case DRW::DIMALIGNED:
        case DRW::DIMANGULAR:
        case DRW::DIMANGULAR3P:
        case DRW::DIMRADIAL:
        case DRW::DIMDIAMETRIC:
        case DRW::DIMORDINATE:
            dxfW->writeDimension(static_cast<DRW_Dimension*>(e));
            break;
        case DRW::LEADER:
            dxfW->writeLeader(static_cast<DRW_Leader*>(e));
            break;
        case DRW::HATCH:
            dxfW->writeHatch(static_cast<DRW_Hatch*>(e));
            break;
        case DRW::IMAGE:
            dxfW->writeImage(static_cast<DRW_Image*>(e), static_cast<dx_ifaceImg*>(e)->path);
            break;
        default:
            break;
    }
}
