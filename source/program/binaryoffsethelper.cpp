#include "binaryoffsethelper.h"
#include "simpleio.h"

char sAppVersion[0x10] = {};
u32  sAppVersionIndex  = 0xffff'ffff;

namespace {

    void ParseAppVersion(char *region_lang_mask) {

        /* Reset app version */
        ::memset(sAppVersion, 0, sizeof(sAppVersion));
        //::OutputDebug("sd:/pre_app_version.txt", sAppVersion, false);

        /* Skip first 2 lines */
        while (*region_lang_mask != '\0' && *region_lang_mask != '\n') { region_lang_mask = region_lang_mask + 1; }
        if (*region_lang_mask == '\0') { return; }
        ++region_lang_mask;
        if (*region_lang_mask == '\0') { return; }
        while (*region_lang_mask != '\0' && *region_lang_mask != '\n') { region_lang_mask = region_lang_mask + 1; }
        if (*region_lang_mask == '\0') { return; }
        ++region_lang_mask;
        if (*region_lang_mask == '\0') { return; }
        //::OutputDebug("sd:/region_lang_debug.txt", region_lang_mask, false);

        /* Copy version string */
        sAppVersion[0] = region_lang_mask[0];
        sAppVersion[1] = region_lang_mask[1];
        sAppVersion[2] = region_lang_mask[2];
        sAppVersion[3] = '\0';
        //::OutputDebug("sd:/app_version_debug.txt", sAppVersion, false);

        return;
    }
}

u32 InitializeAppVersion() {

    /* Init guard */
    if (sAppVersionIndex != 0xffff'ffff) { return sAppVersionIndex; }

    /* Temporarily mount rom */
    nn::fs::MountRom("content");

    /* Read RegionLangMask */
    char region_lang_mask[0x300] = {};
    ::ReadFileFixed("content:/System/RegionLangMask.txt", region_lang_mask, sizeof(region_lang_mask));

    /* Unmount */
    nn::fs::Unmount("content");

    /* Parse app version */
    ParseAppVersion(region_lang_mask);

    /* Check for supported app version */
    sAppVersionIndex = 0xffff'ffff;

    if (::strcmp(sAppVersion, "100") == 0) {
        sAppVersionIndex = 0;
    } else if (::strcmp(sAppVersion, "110") == 0) {
        sAppVersionIndex = 1;
    } else if (::strcmp(sAppVersion, "111") == 0) {
        sAppVersionIndex = 2;
    } else if (::strcmp(sAppVersion, "112") == 0) {
        sAppVersionIndex = 3;
    } else if (::strcmp(sAppVersion, "120") == 0) {
        sAppVersionIndex = 4;
    } else if (::strcmp(sAppVersion, "121") == 0) {
        sAppVersionIndex = 5;
    }

    //char res_stats[nn::fs::MaxDirectoryEntryNameSize + 1] = {};
    //nn::util::SNPrintf(res_stats, sizeof(res_stats), "app_version: %d", sAppVersionIndex);
    //::OutputDebug("sd:/app_version_num.txt", res_stats);

    return sAppVersionIndex;
}
