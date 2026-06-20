#pragma once

/** @file font_lookup.hpp
 *  @brief System font lookup using fontconfig */

#include <filesystem>
#include <string>
#include <vector>

#if defined(__linux__)
#include <fontconfig/fontconfig.h>
#endif

namespace cppreact {
    /** @brief Look up font files on the system matching a given family name
     *
     * Uses fontconfig on Linux.  First pass collects exact family-name matches
     * from system and application font sets.  Second pass uses FcFontSort for
     * fallback fonts (emoji, script fallbacks, etc.).  On non-Linux platforms
     * the vector is returned empty.
     * @param family_name Name of the font family to look up
     * @return Unique list of file paths for matching font files */
    inline std::vector<std::filesystem::path> lookup_font(const std::string& family_name) {
      std::vector<std::filesystem::path> result;

#if defined(__linux__)

      // pass 1: all variants matching exact family name (Bold, Italic, etc.)
      {
        FcConfig* cfg = FcConfigGetCurrent();
        for (int set = FcSetSystem; set <= FcSetApplication; set++) {
          FcFontSet* fs = FcConfigGetFonts(cfg, (FcSetName)set);
          if (!fs) continue;
          for (int i = 0; i < fs->nfont; i++) {
            FcChar8* val = nullptr;
            for (int j = 0; FcPatternGetString(fs->fonts[i], FC_FAMILY, j, &val) == FcResultMatch; j++)
              if (family_name == (const char*)val) {
                FcChar8* file = nullptr;
                if (FcPatternGetString(fs->fonts[i], FC_FILE, 0, &file) == FcResultMatch) {
                  std::filesystem::path p((const char*)file);
                  bool dup = false;
                  for (auto& existing : result)
                    if (existing == p) { dup = true; break; }
                  if (!dup) result.push_back(std::move(p));
                }
                break;
              }
          }
        }
      }

      // pass 2: fallback chain via FcFontSort (emoji, script fallbacks, etc.)
      {
        FcResult fcresult;
        FcPattern* pat = FcPatternCreate();
        FcPatternAddString(pat, FC_FAMILY, (const FcChar8*)family_name.c_str());
        FcConfigSubstitute(nullptr, pat, FcMatchPattern);
        FcDefaultSubstitute(pat);
        FcFontSet* fs = FcFontSort(nullptr, pat, FcTrue, nullptr, &fcresult);
        FcPatternDestroy(pat);
        if (fs) {
          for (int i = 0; i < fs->nfont; i++) {
            FcChar8* file = nullptr;
            if (FcPatternGetString(fs->fonts[i], FC_FILE, 0, &file) == FcResultMatch) {
              std::filesystem::path p((const char*)file);
              bool dup = false;
              for (auto& existing : result)
                if (existing == p) { dup = true; break; }
              if (!dup) result.push_back(std::move(p));
            }
          }
          FcFontSetDestroy(fs);
        }
      }
#endif

      return result;
    }
}
