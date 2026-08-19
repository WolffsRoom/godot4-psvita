#ifndef VITA_COREDUMP_HWINFO_H
#define VITA_COREDUMP_HWINFO_H

#include <cstdint>
#include <string>

namespace vita
{
namespace coredump
{

enum class productCode {
    TEST   = 0x100, // Test - Prototype / Test unit
    TOOL   = 0x101, // Tool - DevKit / Development Tool Kit - PDEL / DEM
    DEX    = 0x102, // DEX - TestKit / Testing Kit - PTEL
    CEX_JP = 0x103, // CEX - Consumer - J1 - Japan
    CEX_US = 0x104, // CEX - Consumer - UC2 - North America
    CEX_EU = 0x105, // CEX - Consumer - CEL - Europe/East/Africa // PCH-xx04
    CEX_KR = 0x106, // CEX - Consumer - KR2 - South Korea
    CEX_GB = 0x107, // CEX - Consumer - CEK - Great Britain/United Kingdom // PCH-xx03, VTE-1016
    CEX_MX = 0x108, // CEX - Consumer - MX2 - Mexico/Latin America
    CEX_AU = 0x109, // CEX - Consumer - AU3 - Australia/New Zealand
    CEX_HK = 0x10A, // CEX - Consumer - E12 - Hong Kong/Macao/Singapore/Malaysia
    CEX_TW = 0x10B, // CEX - Consumer - TW1 - Taiwan
    CEX_RU = 0x10C, // CEX - Consumer - RU3 - Russia - added in ????
    CEX_CH = 0x10D  // CEX - Consumer - CN9 - China - added in 2015
};



// from https://github.com/CelesteBlue-dev/PS-ConsoleId-wiki/blob/master/PS-ConsoleId-wiki.txt

enum class productSubCode {
    CEX_PROTO1 = 0x02, // Prototype CEX - never seen on any device yet except in the OS.
    DVT1       = 0x03, // Prototype DevKit (Tool DVT1) - never seen on any device yet except in the OS. Maybe CEM-3000E3 or DEM-3000G.
    DVT1_1     = 0x04, // Prototype DevKit - never seen on any device yet except in the OS.
    DVT2       = 0x05, // Prototype DevKit (Tool DVT2) - Motherboard IRT-001 (0-835-167-04) (Prototype DevKit DVT2 internal System Debugger DEM-3000H)
    CEX_PROTO2 = 0x07, // Prototype CEX - never seen on any device yet except in the OS.
    EVT1       = 0x09, // Prototype DevKit - Motherboard IRT-002 (Prototype DevKit New EVT1 DEM-3000K). Maybe also DEM-3000JEC.
    CEM        = 0x0A, // Prototype DEX - CEM-3000NE2 with min FW 0.990.
    NDVT1      = 0x0B, // Prototype - Motherboard IRT-002 (0-851-973-06) (Prototype DevKit NEW DVT1 DEM-3000L)
    PROTO1     = 0x0C, // Prototype - never seen on any device yet except in the OS. Maybe M chassis.
    PROTO2     = 0x0D, // Prototype - never seen. Maybe N chassis.
    PROTO3     = 0x0E, // Prototype - never seen. Maybe O chassis.
    CEX_PROTO3 = 0x0F, // Prototype CEX - Motherboard IRS-002 (0-845-846-U10) (CEM-3000NP1). Maybe DEM-3000P.
    FAT1       = 0x10, // FAT - Motherboard IRS-002 (PCH-10XX / PCH-11XX / TestKit PTEL-10XX) or Motherboard IRT-002 (DevKit PDEL-10XX) - factory FW 1.00 or 1.03
    FAT2       = 0x11, // FAT - Motherboard IRS-1001 (PCH-11XX) factory FW 1.81, min FW 1.80, 2012-09
    FAT3       = 0x12, // FAT - Motherboard IRS-1001 (PCH-10XX) factory FW 1.81, min FW 1.80, 2012-09
    SLIM1      = 0x14, // SLIM - Motherboard USS-1001 (PCH-20XX / TestKit PTEL-20XX) - factory FW 2.50, 2013-06
    SLIM2      = 0x18, // SLIM - Motherboard USS-1002 (PCH-20XX / TestKit PTEL-20XX) - factory FW 3.50 or 3.65, 2015-05
    PSTV1      = 0x201, // PS TV - Motherboard DOL-1001 (VTE-10XX) - factory FW 2.60 (?or 2.50?) white units 2013-10, factory FW 3.20 black units year 2014
    PSTV2      = 0x202, // PS TV - Motherboard DOL-1002 (VTE-10XX) - factory FW 3.30 black units year 2015
    PSTV_TOOL  = 0x408, // PS TV DevKit (Tool) - never seen on any device yet except in the OS.
    PSTV_DEX   = 0x602 // PS TV Prototype TestKit (Dolce DVT WLAN DEX) - Motherboard DOL-1001 (0-DOL-001-U3A) (CEM-3000P01 / THV-1000 D1) - factory FW 2.50, white color
};


class hwInfo {
    public:
        hwInfo(void* data, uint32_t size);
        productCode code();
        productSubCode subCode();
        std::string& qafName();

        std::string productType();
        std::string productSubType();

    private:
        productCode _product_code;
        productSubCode _product_subcode;
        std::string _qaf_name;
        uint32_t _qaf_flags[4];
}; // hwInfo

}; // coredump
}; // vita

#endif