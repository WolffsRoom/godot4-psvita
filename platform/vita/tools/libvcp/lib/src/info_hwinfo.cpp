#include "info_hwinfo.hpp"
#include <map>
#include "memstream.hpp"

namespace vita
{
namespace coredump
{
/*
    struct hwInfo_struct { // 0x4 + 0x7c
      uint32_t unk1; // min. coredump version

      uint32_t unk2;
      uint32_t product_code;
      uint32_t product_subcode;
      uint32_t qaf_flags[4];
      char qaf_name[16]; // ksceSblQafManagerGetQafName(&local_2c,0x10);
      uint8_t unk3[80]; // always zero?
    };
*/

    hwInfo::hwInfo(void* buf, uint32_t size)
    {
        memstream in((char*)buf, size);
        in.seekg(8, std::ios_base::beg);

        uint32_t product_code;
        uint32_t product_subcode;
        read_var(product_code);
        read_var(product_subcode);
        _product_code = static_cast<productCode>(product_code);
        _product_subcode = static_cast<productSubCode>(product_subcode);

        read_var(_qaf_flags[0]);
        read_var(_qaf_flags[1]);
        read_var(_qaf_flags[2]);
        read_var(_qaf_flags[3]);


        read_string(_qaf_name, 16);


    }

    productCode hwInfo::code()
    {
        return _product_code;
    }

    productSubCode hwInfo::subCode()
    {
        return _product_subcode;
    }

    std::string& hwInfo::qafName()
    {
        return _qaf_name;
    }

    std::string hwInfo::productType()
    {
        std::map<productCode, std::string> products = {
            {productCode::TEST, "Prototype"},
            {productCode::TOOL, "Devkit"},
            {productCode::DEX,  "Testkit"},
            {productCode::CEX_JP, "Retail (Japan)"},
            {productCode::CEX_US, "Retail (North America)"},
            {productCode::CEX_EU, "Retail (Europe/East/Africa)"},
            {productCode::CEX_KR, "Retail (South Korea)"},
            {productCode::CEX_GB, "Retail (United Kingdom)"},
            {productCode::CEX_MX, "Retail (Mexico/Latin America)"},
            {productCode::CEX_AU, "Retail (Australia/New Zealand)"},
            {productCode::CEX_HK, "Retail (Hong Kong/Macao/Singapore/Malaysia)"},
            {productCode::CEX_TW, "Retail (Taiwan)"},
            {productCode::CEX_RU, "Retail (Russia)"},
            {productCode::CEX_CH, "Retail (China)"},
        };

        return products.at(_product_code);;
    }

    std::string hwInfo::productSubType()
    {
        std::map<productSubCode, std::string> subProducts = {
            {productSubCode::CEX_PROTO1, "Prototype"},
            {productSubCode::DVT1       , "Prototype DevKit (Tool DVT1)"},
            {productSubCode::DVT1_1     , "Prototype DevKit"},
            {productSubCode::DVT2       , "Prototype DevKit (Tool DVT2) - Motherboard IRT-001 (0-835-167-04) (Prototype DevKit DVT2 internal System Debugger DEM-3000H)"},
            {productSubCode::CEX_PROTO2 , "Prototype CEX"},
            {productSubCode::EVT1       , "Prototype DevKit - Motherboard IRT-002 (Prototype DevKit New EVT1 DEM-3000K)"},
            {productSubCode::CEM        , "Prototype DEX - CEM-3000NE2 with min FW 0.990."},
            {productSubCode::NDVT1      , "Prototype - Motherboard IRT-002 (0-851-973-06) (Prototype DevKit NEW DVT1 DEM-3000L)"},
            {productSubCode::PROTO1     , "Prototype"},
            {productSubCode::PROTO2     , "Prototype"},
            {productSubCode::PROTO3     , "Prototype"},
            {productSubCode::CEX_PROTO3 , "Prototype CEX - Motherboard IRS-002 (0-845-846-U10) (CEM-3000NP1)"},
            {productSubCode::FAT1       , "FAT - Motherboard IRS-002 (PCH-10XX / PCH-11XX / TestKit PTEL-10XX)"},
            {productSubCode::FAT2       , "FAT - Motherboard IRS-1001 (PCH-11XX)"},
            {productSubCode::FAT3       , "FAT - Motherboard IRS-1001 (PCH-10XX)"},
            {productSubCode::SLIM1      , "SLIM - Motherboard USS-1001 (PCH-20XX / TestKit PTEL-20XX)"},
            {productSubCode::SLIM2      , "SLIM - Motherboard USS-1002 (PCH-20XX / TestKit PTEL-20XX)"},
            {productSubCode::PSTV1      , "PS TV - Motherboard DOL-1001 (VTE-10XX)"},
            {productSubCode::PSTV2      , "PS TV - Motherboard DOL-1002 (VTE-10XX)"},
            {productSubCode::PSTV_TOOL  , "PS TV DevKit (Tool)"},
            {productSubCode::PSTV_DEX   , "PS TV Prototype TestKit (Dolce DVT WLAN DEX) - Motherboard DOL-1001 (0-DOL-001-U3A) (CEM-3000P01 / THV-1000 D1)"},
        };

        std::map<productSubCode, std::string> subProducts_TOOL = {
            {productSubCode::CEX_PROTO1, "Prototype"},
            {productSubCode::DVT1       , "Prototype DevKit (Tool DVT1)"},
            {productSubCode::DVT1_1     , "Prototype DevKit"},
            {productSubCode::DVT2       , "Prototype DevKit (Tool DVT2) - Motherboard IRT-001 (0-835-167-04) (Prototype DevKit DVT2 internal System Debugger DEM-3000H)"},
            {productSubCode::CEX_PROTO2 , "Prototype CEX"},
            {productSubCode::EVT1       , "Prototype DevKit - Motherboard IRT-002 (Prototype DevKit New EVT1 DEM-3000K)"},
            {productSubCode::CEM        , "Prototype DEX - CEM-3000NE2 with min FW 0.990."},
            {productSubCode::NDVT1      , "Prototype - Motherboard IRT-002 (0-851-973-06) (Prototype DevKit NEW DVT1 DEM-3000L)"},
            {productSubCode::PROTO1     , "Prototype"},
            {productSubCode::PROTO2     , "Prototype"},
            {productSubCode::PROTO3     , "Prototype"},
            {productSubCode::CEX_PROTO3 , "Prototype CEX - Motherboard IRS-002 (0-845-846-U10) (CEM-3000NP1)"},
            {productSubCode::FAT1       , "FAT - Motherboard IRT-002 (DevKit PDEL-10XX)"},
            {productSubCode::FAT2       , "FAT - Motherboard IRS-1001 (PCH-11XX)"},
            {productSubCode::FAT3       , "FAT - Motherboard IRS-1001 (PCH-10XX)"},
            {productSubCode::SLIM1      , "SLIM - Motherboard USS-1001 (PCH-20XX / TestKit PTEL-20XX)"},
            {productSubCode::SLIM2      , "SLIM - Motherboard USS-1002 (PCH-20XX / TestKit PTEL-20XX)"},
            {productSubCode::PSTV1      , "PS TV - Motherboard DOL-1001 (VTE-10XX)"},
            {productSubCode::PSTV2      , "PS TV - Motherboard DOL-1002 (VTE-10XX)"},
            {productSubCode::PSTV_TOOL  , "PS TV DevKit (Tool)"},
            {productSubCode::PSTV_DEX   , "PS TV Prototype TestKit (Dolce DVT WLAN DEX) - Motherboard DOL-1001 (0-DOL-001-U3A) (CEM-3000P01 / THV-1000 D1)"},
        };

        if (_product_code == productCode::TOOL)
            return subProducts_TOOL.at(_product_subcode);
        return subProducts.at(_product_subcode);
    }


}; // coredump
}; // vita

