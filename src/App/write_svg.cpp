#include "write_svg.h"

#include <cstdlib>     // General purpose utilities
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <stdarg.h>  // For va_start, etc.

#include "dx_iface.h"
#include "dx_data.h"
#include <algorithm>

const double pixelToMillimeterConversionRatio = 3.543299873306695;
const double strokeWidth = 0.1;

//#define ALLBLOCK true
//#define ALLBLOCK_SPLIT true

bool ignoreLayer(std::string layerName)
{
    //return false;
    if (layerName.find("0") != std::string::npos)
        return false;
    if (layerName.find("Komponenten") != std::string::npos)
        return false;
    if (layerName.find("Hoofdlijn") != std::string::npos)
        return false;
    return true;
}

const char* va(const char* pFormat, ...)
{
    //assert(pFormat);
    if (pFormat == nullptr)
        return "INVALID";
#define	VA_MAX_STRING 4096
    static char buffer[VA_MAX_STRING];

    va_list argList;
    va_start(argList, pFormat);
    int count = vsnprintf_s(buffer, VA_MAX_STRING, _TRUNCATE, pFormat, argList);
    va_end(argList);

    if (count < 0)
        throw;

    char *buf = &buffer[0];
    memcpy(buf, buffer, count + 1);
    return buf;
}

std::string string_format(const char *pFormat, ...)
{
    std::string retStr("");

    va_list argList;
    va_start(argList, pFormat);

    // Get formatted string length, adding one for NULL
    std::size_t len = _vscprintf(pFormat, argList) + 1;

    // Create a char vector to hold the formatted string.
    std::vector<char> buffer(len, '\0');

    if (vsnprintf_s(&buffer[0], buffer.size(), len, pFormat, argList) > 0)
        retStr = &buffer[0];

    va_end(argList);

    return retStr;
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string NameStr(std::string str)
{
    return ReplaceAll(ReplaceAll(str, "&", "+"), "\"", "'");
}

bool WriteEntity(dx_iface *input, DRW_Entity * e, std::stringstream &svg, const double &s, int alt)
{
    if (!e->visible)
        return false;
    if (ignoreLayer(e->layer))
        return false;

    auto strokeClrStr = (alt >= 4 ? " stroke=\"#FF00FF\""
                         : alt == 3 ? " stroke=\"#8000FF\""
                         : alt == 2 ? " stroke=\"#0000FF\""
                         : alt == 1 ? " stroke=\"#000080\""
                         : " stroke=\"#000040\"");

    switch (e->eType)
    {
        case DRW::VIEWPORT:
        {
            //auto vp = static_cast<DRW_Viewport*>(e);
            return false;
        }
        case DRW::BLOCK:
        {
            auto blck = static_cast<DRW_Block*>(e);

            svg << "<text " << string_format("x=\"%f\" y=\"%f\"",
                                             blck->basePoint.x * s, blck->basePoint.y * s)
                << " stroke=\"magenta\""
                << " alignment-baseline=\"center\""
                << " text-anchor=\"middle\""
                << ">BLOCK: " << blck->name << "</text>";

            return true;
        }
        case DRW::INSERT:
        {
            //dxfW->writeInsert(static_cast<DRW_Insert*>(e));
            auto inser = static_cast<DRW_Insert*>(e);
            auto hh = inser->blockRecH.ref;

            int iecnt = 0;
            std::stringstream svgin;
            for (std::list<dx_ifaceBlock*>::const_iterator it9 =
                 input->cData->blocks.begin(); it9 != input->cData->blocks.end();
                 ++it9)
            {
                auto be = *it9;
                if (be->parentHandle != hh)
                    continue;
                if (ignoreLayer(be->layer))
                    continue;

                svgin << "<g typename=\"INSBLOCK\"";
                svgin << " name=\"" << NameStr(be->name) << "\"";
                svgin << " posx=\"" << (be->basePoint.x * s) << "\"";
                svgin << " posy=\"" << (be->basePoint.y * s) << "\"";
                svgin << " layer=\"" << NameStr(be->layer) << "\"";
                svgin << " flags=\"" << (be->flags) << "\"";
                svgin << " visible=\"" << (be->visible) << "\"";
                svgin << ">\n";

                for (std::list<DRW_Entity*>::const_iterator it2 =
                     be->ent.begin(); it2 != be->ent.end();
                     ++it2)
                {
                    auto iee = (*it2);
                    if (iee->parentHandle == hh)
                    {
                        if (!iee->visible)
                            continue;
                        if (ignoreLayer(iee->layer))
                            continue;
                        bool we = WriteEntity(input, iee, svgin, s, alt + 1);
                        if (we)
                        {
                            iecnt++;
                            svgin << "\n";
                        }
                    }
                }

                svgin << "</g>\n";
            }

            if (iecnt > 0)
            {
                svg << "<g typename=\"DRW_Insert\"";
                svg << " name=\"" << NameStr(inser->name) << "\"";
                svg << " layer=\"" << NameStr(inser->layer) << "\"";
                svg << " posx=\"" << (inser->basePoint.x * s) << "\"";
                svg << " posy=\"" << (inser->basePoint.y * s) << "\"";
                svg << " angle=\"" << inser->angle << "\"";
                svg << " xscale=\"" << inser->xscale << "\"";
                svg << " yscale=\"" << inser->yscale << "\"";
                svg << " zscale=\"" << inser->zscale << "\"";
                svg << " colcount=\"" << inser->colcount << "\"";
                svg << " rowcount=\"" << inser->rowcount << "\"";
                svg << " colspace=\"" << inser->colspace << "\"";
                svg << " rowspace=\"" << inser->rowspace << "\"";
                /*
                auto transstr = string_format("translate(%f, %f) rotate(%d, %f, %f) scale(%f, %f)",
                                              inser->basePoint.x * s, inser->basePoint.y * s,
                                              ((int)(inser->angle * (180.0 / M_PI))), 0.0, 0.0,
                                              inser->xscale, inser->yscale);
                */
                //auto trrotstr = string_format("rotate(%d, %f, %f)", ((int)(inser->angle * (180.0 / M_PI))), inser->basePoint.x * s, inser->basePoint.y * s);
                //auto transstr = string_format("translate(%f, %f)", inser->basePoint.x * s, inser->basePoint.y * s);
                //auto trsclstr = string_format("scale(%f, %f)", inser->xscale, inser->yscale);


                auto sa = abs(sin(inser->angle));
                auto transstr = string_format("translate(%f, %f)", inser->basePoint.x * s * ((1 - sa) + inser->zscale * sa), inser->basePoint.y * s * (sa + inser->zscale * (1 - sa)));

                auto trrotstr = string_format("rotate(%f)", ((inser->angle * (180.0 / M_PI))));

                //auto trsclstr = string_format("scale(%f, %f)", inser->xscale * ((1 - sa) + inser->zscale * sa), inser->yscale * (sa + inser->zscale * (1 - sa)));
                auto trsclstr = string_format("scale(%f, %f)", inser->xscale, inser->yscale);

                svg << " transform=\"" << transstr << " " << trrotstr << " " << trsclstr << "\"";
                svg << ">";

                //svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                //                                 0 * s, 0 * s,
                //                                 inser->basePoint.x * s, inser->basePoint.y * s)
                //    << " stroke=\"yellow\""
                //    << " />";

                svg << svgin.str();
                svg << "</g>\n";
            }
            return iecnt > 0;
        }
        case DRW::POINT:
        {
            //auto ec = static_cast<DRW_Point*>(e);
            return false;
        }
        case DRW::LINE:
        {
            //dxfW->writeLine(static_cast<DRW_Line*>(e));
            auto line = static_cast<DRW_Line*>(e);
            svg << "<line ";
            svg << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                 line->basePoint.x * s, line->basePoint.y * s,
                                 line->secPoint.x * s, line->secPoint.y * s);
//svg << " stroke=\"blue\"";
//svg << (alt >= 2 ? " stroke=\"magenta\"" : alt == 1 ? " stroke=\"green\"" : " stroke=\"blue\"");
            svg << strokeClrStr;
            svg << " layer=\"" << NameStr(line->layer) << "\"";
            svg << " />";
            return true;
        }
        case DRW::CIRCLE:
        {
            //dxfW->writeCircle(static_cast<DRW_Circle*>(e));
            auto circle = static_cast<DRW_Circle*>(e);
            svg << "<circle " << string_format("cx=\"%f\" cy=\"%f\" r=\"%f\"",
                                               circle->basePoint.x * s, circle->basePoint.y * s,
                                               circle->radious * s)
                << " stroke=\"lime\" fill=\"none\""
                << " />";
            return true;
        }
        case DRW::ARC:
        {
            //dxfW->writeArc(static_cast<DRW_Arc*>(e));
            auto arc = static_cast<DRW_Arc*>(e);

            auto center = arc->center();
            auto radius = arc->radius();
            auto startangle = arc->startAngle() * (180.0 / M_PI);
            auto endangle = arc->endAngle() * (180.0 / M_PI);

            svg << "<g typename=\"DRW_Arc\"";
            svg << " isccw=\"" << arc->isccw << "\"";
            svg << " radius=\"" << arc->radius() << "\"";
            svg << " thick=\"" << arc->thick() << "\"";
            svg << " startAngle=\"" << arc->startAngle() << "\"";
            svg << " endAngle=\"" << arc->endAngle() << "\"";
            svg << " >";

            if (startangle > endangle)
                startangle -= 360.0;

            double anglelen = (startangle - endangle);
            double cnt = ceil(abs(anglelen) / 10.0);

            svg << "<path d=\"";
            for (int i = 0; i <= (int)cnt; i++)
            {
                double a1 = startangle + ((i / cnt) * anglelen) * (arc->isccw > 0 ? -1 : 1);
                auto x1 = center.x + radius * cos(M_PI * a1 / 180.0);
                auto y1 = center.y + radius * sin(M_PI * a1 / 180.0);

                if (i == 0)
                    svg << string_format("M %f %f ", x1 * s, y1 * s);
                else
                    svg << string_format("L %f %f ", x1 * s, y1 * s);
            }

            svg << "\"";
            svg << " fill=\"none\"";
            svg << (alt ? " stroke=\"magenta\"" : " stroke=\"green\"");
            svg << " />";

            /*
            svg << "<path d=\"";

            auto x1 = center.x + radius * cos(M_PI * startangle / 180.0);
            auto y1 = center.y + radius * sin(M_PI * startangle / 180.0);
            auto x2 = center.x + radius * cos(M_PI * endangle / 180.0);
            auto y2 = center.y + radius * sin(M_PI * endangle / 180.0);

            svg << string_format("M %f %f ", x1 * s, y1 * s);
            svg << string_format("A %f %f %d %d %d %f %f ",
                                 radius * s, radius * s, 0, (abs(anglelen) > 180 ? 1 : 0), 1, x2 * s, y2 * s);

            svg << "\"";
            svg << " fill=\"none\"";
            svg << " stroke=\"red\"";
            svg << " />";
            */

            svg << "</g>";
            return true;
        }
        case DRW::SOLID:
        {
            auto ec = static_cast<DRW_Solid*>(e);
            svg << "<g typename=\"DRW_Solid\">";
            svg << "<path d=\"";
            svg << string_format("M %f %f ", ec->firstCorner().x * s, ec->firstCorner().y * s);
            svg << string_format("L %f %f ", ec->secondCorner().x * s, ec->secondCorner().y * s);
            svg << string_format("L %f %f ", ec->thirdCorner().x * s, ec->thirdCorner().y * s);
            svg << string_format("L %f %f ", ec->fourthCorner().x * s, ec->fourthCorner().y * s);
            svg << "Z\" />";
            svg << "</g>";
            return true;
        }
        case DRW::ELLIPSE:
        {
            auto ell = static_cast<DRW_Ellipse*>(e);
            //   <ellipse cx="100" cy="50" rx="100" ry="50" />
            
            DRW_Polyline pl;
            ell->toPolyline(&pl, 32);

            svg << "<g typename=\"DRW_Ellipse\">";

            auto vertices = pl.vertlist;
            svg << "<path d=\"";
            svg << string_format("M %f %f ", vertices[0]->basePoint.x * s, vertices[0]->basePoint.y * s);
            for (unsigned int i = 1; i < vertices.size(); i++)
            {
                auto vertice1 = vertices[i];
                svg << string_format("L %f %f ", vertice1->basePoint.x * s, vertice1->basePoint.y * s);
            }
            svg << "Z\"";
            svg << " fill=\"#ffff0080\"";
            svg << " stroke=\"#999900\"";
            svg << " />";

            svg << "</g>";
            return false;
        }
        case DRW::LWPOLYLINE:
        {
            //dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(e));
            auto pl = static_cast<DRW_LWPolyline*>(e);
            svg << "<g typename=\"DRW_LWPolyline\">";

            auto vertices = pl->vertlist;
            /*for (int i = 0; i < vertices.size() - 1; i++)
            {
                auto vertice1 = vertices[i];
                auto vertice2 = vertices[i + 1];
                //svgSnippet += getLineSvg(vertice1.x, vertice1.y, vertice2.x, vertice2.y);
                svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                                 vertice1->x * s, vertice1->y * s,
                                                 vertice2->x * s, vertice2->y * s)
                    << " stroke=\"red\""
                    << " />";
            }*/

            svg << "<path d=\"";
            svg << string_format("M %f %f ", vertices[0]->x * s, vertices[0]->y * s);
            for (unsigned int i = 1; i < vertices.size(); i++)
            {
                auto vertice1 = vertices[i];
                svg << string_format("L %f %f ", vertice1->x * s, vertice1->y * s);
            }
            svg << "Z\"";
            svg << " fill=\"#00ffff80\"";
            svg << " stroke=\"#ff8000\"";
            svg << " />";

            svg << "</g>";
            return true;
        }
        case DRW::POLYLINE:
        {
            //dxfW->writePolyline(static_cast<DRW_Polyline*>(e));
            auto pl = static_cast<DRW_Polyline*>(e);
            svg << "<g typename=\"DRW_Polyline\">";
            auto vertices = pl->vertlist;
            /*for (int i = 0; i < ((int)vertices.size() - 1); i++)
            {
                auto vertice1 = vertices[i];
                auto vertice2 = vertices[i + 1];
                svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                                 vertice1->basePoint.x * s, vertice1->basePoint.y * s,
                                                 vertice2->basePoint.x * s, vertice2->basePoint.y * s)
                    << " stroke=\"orange\""
                    << " />";
            }
            */
            svg << "<path d=\"";
            svg << string_format("M %f %f ", vertices[0]->basePoint.x * s, vertices[0]->basePoint.y * s);
            for (unsigned int i = 1; i < vertices.size(); i++)
            {
                auto vertice1 = vertices[i];
                svg << string_format("L %f %f ", vertice1->basePoint.x * s, vertice1->basePoint.y * s);
            }
            svg << "Z\"";
            svg << " fill=\"#00ff0080\"";
            svg << " stroke=\"orange\"";
            svg << " />";

            svg << "</g>";
            return true;
        }
        case DRW::SPLINE:
        {
            auto spl = static_cast<DRW_Spline*>(e);
            svg << "<g typename=\"DRW_Spline\">";
            auto vertices = spl->fitlist;
            for (int i = 0; i < spl->nfit - 1; i++)
            {
                auto vertice1 = vertices[i];
                auto vertice2 = vertices[i + 1];
                svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                                 vertice1->x * s, vertice1->y * s,
                                                 vertice2->x * s, vertice2->y * s)
                    << " stroke=\"red\""
                    << " />";
            }
            svg << "</g>";
            return false;
        }
        //    case RS2::EntitySplinePoints:
        //        writeSplinePoints(static_cast<DRW_Point*>(e));
        //        break;
        //    case RS2::EntityVertex:
        //        break;
        case DRW::MTEXT:
        case DRW::TEXT:
        {
            //dxfW->writeText(static_cast<DRW_Text*>(e));
            auto txt = static_cast<DRW_Text*>(e);

            auto fontsize = ceil(abs(txt->height / 300.0) * 100);

            //svg << "<g typename=\"DRW_Text\">";
            svg << "<g typename=\"DRW_Text\"";
            svg << " eType=\"" << (txt->eType) << "\"";
            svg << " posx=\"" << (txt->basePoint.x) << "\"";
            svg << " posy=\"" << (txt->basePoint.y) << "\"";
            svg << " sposx=\"" << (txt->secPoint.x) << "\"";
            svg << " sposy=\"" << (txt->secPoint.y) << "\"";
            svg << " height=\"" << txt->height << "\"";
            svg << " widthscale=\"" << txt->widthscale << "\"";
            svg << " angle=\"" << txt->angle << "\"";
            svg << " style=\"font-size: " << fontsize << "%;\"";
            svg << " >";

            auto x1 = txt->basePoint.x;
            auto y1 = txt->basePoint.y;
            auto x2 = txt->secPoint.x;
            auto y2 = (txt->basePoint.y - txt->height);
            auto ws = txt->widthscale;


            std::string halignment = "text-anchor=\"";
            halignment +=
                (txt->alignH == DRW_Text::HLeft ? "start"
                 : txt->alignH == DRW_Text::HRight ? "end"
                 //: "middle"
                 : "start"
                 );
            halignment += "\" ";

            std::string valignment = "alignment-baseline=\"";
            valignment +=
                (txt->alignV == DRW_Text::VTop ? "top"
                 : txt->alignV == DRW_Text::VBottom ? "bottom"
                 : "center");
            valignment += "\" ";

            std::stringstream transf;
            transf << "transform=\"";
            transf << "rotate(";
            transf << (-180 + (txt->angle * (180.0 / M_PI)));
            transf << ", ";
            transf << (x1 * s);
            transf << ", ";
            transf << (y1 * s);
            transf << ")";
            transf << " scale(-1, 1)";
            transf << "\" ";

            svg << "<text " << halignment << valignment << transf.str() << string_format("x=\"%f\" y=\"%f\" width=\"%f\" height=\"%f\"",
                                                                                         -x1 * s, y1 * s,
                                                                                         (x2 - x1) * s, (y2 - y1) * s)
                << " stroke=\"black\""
                << ">" << txt->text << "</text>";

            //svg << "<path d=\"";
            //svg << string_format("M %f %f ", x1 * s, y1 * s);
            //svg << string_format("L %f %f ", x2 * s, y1 * s);
            //svg << string_format("L %f %f ", x2 * s, y2 * s);
            //svg << string_format("L %f %f ", x1 * s, y2 * s);
            //svg << "Z\"";
            //svg << " stroke=\"yellow\" fill=\"none\"";
            //svg << " />";
            //
            //svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
            //                                 x1 * s, y1 * s,
            //                                 x1 * s, y2 * s)
            //    << " stroke=\"red\""
            //    << " />";

            svg << "</g>";
            return true;
        }
        case DRW::DIMLINEAR:
        case DRW::DIMALIGNED:
        case DRW::DIMANGULAR:
        case DRW::DIMANGULAR3P:
        case DRW::DIMRADIAL:
        case DRW::DIMDIAMETRIC:
        case DRW::DIMORDINATE:
        case DRW::DIMENSION:
        {
            //dxfW->writeDimension(static_cast<DRW_Dimension*>(e));
            auto dms = static_cast<DRW_Dimension*>(e);
            auto dp = dms->getDefPoint();
            auto tp = dms->getTextPoint();

            auto x1 = tp.x;
            auto y1 = tp.y;

            svg << "<text " << string_format("x=\"%f\" y=\"%f\"",
                                             x1 * s, y1 * s)
                << " stroke=\"magenta\""
                << " alignment-baseline=\"center\""
                << " text-anchor=\"middle\""
                << ">Dimension: " << dms->getText() << "</text>";

            svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                             x1 * s, y1 * s,
                                             dp.x * s, dp.y * s)
                << " stroke=\"yellow\""
                << " />";
            return true;
        }
        case DRW::LEADER:
        {
            //dxfW->writeLeader(static_cast<DRW_Leader*>(e));
            auto ldr = static_cast<DRW_Leader*>(e);
            svg << "<g typename=\"DRW_Leader\">";
            auto vertices = ldr->vertexlist;
            for (int i = 0; i < ((int)vertices.size() - 1); i++)
            {
                auto vertice1 = vertices[i];
                auto vertice2 = vertices[i + 1];
                svg << "<line " << string_format("x1=\"%f\" y1=\"%f\" x2=\"%f\" y2=\"%f\"",
                                                 vertice1->x * s, vertice1->y * s,
                                                 vertice2->x * s, vertice2->y * s)
                    << " stroke=\"green\""
                    << " />";
            }
            svg << "</g>";
            return true;
        }
        case DRW::HATCH:
        {
            //dxfW->writeHatch(static_cast<DRW_Hatch*>(e));
            auto hatch = static_cast<DRW_Hatch*>(e);
            
            svg << "<g typename=\"Hatch\"";
            svg << " name=\"" << NameStr(hatch->name) << "\"";
            svg << " pox=\"" << hatch->basePoint.x << "\"";
            svg << " posy=\"" << hatch->basePoint.y << "\"";
            svg << " angle=\"" << hatch->angle << "\"";
            svg << " scale=\"" << hatch->scale << "\"";
            svg << " fill=\"#00ff0080\" stroke=\"darkorange\"";
            svg << ">";

            //hatch->basePoint.x
            auto llist = hatch->looplist;
            for (unsigned int j = 0; j < llist.size(); j++)
            {
                auto hloop = llist[j];
                for (unsigned int i = 0; i < hloop->objlist.size(); i++)
                {
                    auto he = hloop->objlist[i];
                    WriteEntity(input, he, svg, s, alt + 1);
                }
            }

            svg << "</g>";
            return true;
        }
        case DRW::IMAGE:
        {
            //dxfW->writeImage(static_cast<DRW_Image*>(e), static_cast<dx_ifaceImg*>(e)->path);
            return false;
        }
        default:
        {
            printf("");
            return false;
        }
    }
}


bool extractSvgEnt(dx_iface *input, std::stringstream& svg, const double &s, DRW_Entity * e,
                   double & bminx, double & bminy, double & bmaxx, double & bmaxy)
{
    if (!e->visible)
        return false;

    if (ignoreLayer(e->layer))
        return false;

    bool we = WriteEntity(input, e, svg, s, 0);
    if (we)
        svg << "\n";

    bool known = false;
    known = false //e->eType == DRW::POINT
        || e->eType == DRW::LINE
        //|| e->eType == DRW::CIRCLE
        || e->eType == DRW::ARC
        //|| e->eType == DRW::ELLIPSE
        || e->eType == DRW::LWPOLYLINE
        || e->eType == DRW::POLYLINE
        //|| e->eType == DRW::MTEXT
        || e->eType == DRW::TEXT;
    if (known)
    {
        auto eline = dynamic_cast<DRW_Line*>(e);
        if (eline != nullptr)
        {
            bminx = std::min(bminx, std::min(eline->basePoint.x, eline->secPoint.x));
            bminy = std::max(bminy, std::max(eline->basePoint.y, eline->secPoint.y));
            bmaxx = std::max(bmaxx, std::max(eline->basePoint.x, eline->secPoint.x));
            bmaxy = std::min(bmaxy, std::min(eline->basePoint.y, eline->secPoint.y));
        }
        else
        {
            auto epnt = dynamic_cast<DRW_Point*>(e);
            if (epnt != nullptr)
            {
                bminx = std::min(bminx, epnt->basePoint.x);
                bminy = std::max(bminy, epnt->basePoint.y);
                bmaxx = std::max(bmaxx, epnt->basePoint.x);
                bmaxy = std::min(bmaxy, epnt->basePoint.y);
            }
        }
    }
    return true;
}

bool extractSvgBlock(dx_iface *input, std::stringstream &asvg, const double &s, dx_ifaceBlock * eb,
                     double & bminx, double & bminy, double & bmaxx, double & bmaxy)
{
    int ecnt = 0;
    std::stringstream svg;

    for (std::list<DRW_Entity*>::const_iterator it2 = eb->ent.begin(); it2 != eb->ent.end(); ++it2)
    {
        auto e = (*it2);

        if (extractSvgEnt(input, svg, s, e, bminx, bminy, bmaxx, bmaxy))
            ecnt++;
    }

    if (ecnt <= 0)
        return false;

    const double extr = 500.0f;
    bminx -= extr;
    bminy += extr;
    bmaxx += extr * 2;
    bmaxy -= extr * 2;

    auto cw = (bmaxx - bminx);
    auto ch = (bmaxy - bminy);

    //mw = std::max(mw, cw);
    //mh = std::max(mh, -ch);

    //if (cw * s * pixelToMillimeterConversionRatio > 3000)
    //    return false;
    //if (ch * s * pixelToMillimeterConversionRatio > 400)
    //    return false;

    auto svgWH = string_format(" width=\"%f\" height=\"%f\"",
                               //bmaxx * s * pixelToMillimeterConversionRatio,
                               //-bmaxy * s * pixelToMillimeterConversionRatio);
                               cw * s * pixelToMillimeterConversionRatio,
                               -ch * s * pixelToMillimeterConversionRatio);
    auto transstr = string_format("translate(%f %f)",
                                  -bminx * s,// * pixelToMillimeterConversionRatio,
                                  -bminy * s);// *pixelToMillimeterConversionRatio);

    //asvg << "<g name=\"BLOCK:" << eb->name << "\"";
    asvg << "<g typename=\"BLOCK\"";
    asvg << " name=\"" << NameStr(eb->name) << "\"";
    asvg << " posx=\"" << (eb->basePoint.x * s) << "\"";
    asvg << " posy=\"" << (eb->basePoint.y * s) << "\"";
    asvg << " layer=\"" << NameStr(eb->layer) << "\"";
    asvg << " flags=\"" << (eb->flags) << "\"";
    asvg << " visible=\"" << (eb->visible) << "\"";
    //asvg << " transform=\""  << "scale(" << pixelToMillimeterConversionRatio << ",-" << pixelToMillimeterConversionRatio << ") "  << transstr  << "\" "
    //asvg << svgWH;
    //asvg << " style=\"stroke:black; stroke-width:" << strokeWidth << "; "
    //    << "stroke-linecap:round; stroke-linejoin:round\"";
    asvg << ">\n" <<
        svg.str() <<
        "</g>\n";

    return true;
}


bool extractSvg(dx_iface *input)
{
    double mw = 0;
    double mh = 0;

    const double s = 0.01;
    int cntr = 0;
    std::stringstream asvg;
    DRW_Coord* extmin = input->cData->headerC.vars["EXTMIN"]->content.v;
    DRW_Coord* extmax = input->cData->headerC.vars["EXTMAX"]->content.v;
    const double extr = 1000.0f;
    extmin->x -= extr;
    extmin->y -= extr;
    extmax->x += extr * 2;
    extmax->y += extr * 2;

    // svg file
    {
#if ALLBLOCK
        for (std::list<dx_ifaceBlock*>::const_iterator itb =
             input->cData->blocks.begin(); itb != input->cData->blocks.end();
             ++itb)
        {
            auto eb = *itb;
            cntr++;
#else
        for (int dummy = 0; dummy < 1; dummy++)
        {
            auto eb = input->cData->mBlock;
#endif
            if (!eb->visible)
                continue;

            double bminx = (1 << 28); //std::numeric_limits<double>::max();
            double bminy = -(1 << 28); //std::numeric_limits<double>::min();
            double bmaxx = 0;
            double bmaxy = 0;

            extractSvgBlock(input, asvg, s, eb, bminx, bminy, bmaxx, bmaxy);

            auto cw = (bmaxx - bminx);
            auto ch = (bmaxy - bminy);

            mw = std::max(mw, cw);
            mh = std::max(mh, -ch);

#if !ALLBLOCK_SPLIT
        } // blocks
#endif
        /*
        {
            double bminx = (1 << 28); //std::numeric_limits<double>::max();
            double bminy = -(1 << 28); //std::numeric_limits<double>::min();
            double bmaxx = 0;
            double bmaxy = 0;
            auto last = (input->cData->blocks.back());
            extractSvgBlock(asvg, s, last, bminx, bminy, bmaxx, bmaxy);
        }
        */

        double ss = s;// *pixelToMillimeterConversionRatio;

        auto osvgWH = string_format("width=\"%f\" height=\"%f\"",
                                   //bmaxx * s * pixelToMillimeterConversionRatio,
                                   //-bmaxy * s * pixelToMillimeterConversionRatio);
                                    mw * s * pixelToMillimeterConversionRatio,
                                    mh * s * pixelToMillimeterConversionRatio);
        osvgWH = "width=\"1vp\" height=\"1vp\"";

        std::stringstream osvg;
        osvg << "<svg " << osvgWH << " version=\"1.1\" xmlns=\"http://www.w3.org/2000/svg\"";
        osvg << " font-size=\"5px\"";
        osvg << " transform=\"scale(1, -1)\"";
        osvg << " style=\"stroke:black; stroke-width:" << strokeWidth << "; "
            << "stroke-linecap:round; stroke-linejoin:round\"";
        osvg
#if !ALLBLOCK
            << " viewBox=\"" << (extmin->x * ss) << " " << (extmin->y * ss) << " " << ((extmax->x - extmin->x) * ss) << " " << ((extmax->y - extmin->y) * ss) << "\""
#elif ALLBLOCK_SPLIT
            << " viewBox=\"" << (bminx * ss) << " " << (-bminy * ss) << " " << ((bmaxx - bminx) * ss) << " " << (-(bmaxy - bminy) * ss) << "\""
#endif
            << ">\n" <<
            //"<g transform=\"scale(" << pixelToMillimeterConversionRatio << ",-" << pixelToMillimeterConversionRatio << ") " << transstr << "\" " <<
            //" style=\"stroke:black; stroke-width:" << strokeWidth << "; " <<
            //"stroke-linecap:round; stroke-linejoin:round\">\n" <<
            //svg.str() <<
            //"</g>\n" <<
            asvg.str() <<
            "</svg>\n";

        std::stringstream filename;
        filename << "rik_out_";
        filename << cntr;
        filename << ".svg";

        std::cout << "written: " << filename.str() << std::endl;
        std::ofstream svgout(filename.str());
        svgout << osvg.str();
        svgout.close();
        }
#if ALLBLOCK_SPLIT
}
#endif

return true;
}
