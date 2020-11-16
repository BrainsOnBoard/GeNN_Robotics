#include "generate_images.h"

// BoB robotics includes
#include "common/path.h"
#include "common/macros.h"
#include "navigation/perfect_memory.h"

// Third-party includes
#include "third_party/path.h"

// Standard C++ includes
#include <algorithm>
#include <fstream>
#include <iomanip>

#define STR(s) #s
#define XSTR(s) STR(s)
#define FILENAME (XSTR(NAVIGATION_ALGO) ".h")

int main(int, char **)
{
    generateImages();
    BoBRobotics::Navigation::NAVIGATION_ALGO pm{ TestImageSize };
    for (const auto &image : TestImages) {
        pm.train(image);
    }

    std::string filename = FILENAME;
    for (char &c : filename) {
        if (c == '<' || c == '>')
            c = '_';
    }

    const auto filepath = BoBRobotics::Path::getProgramDirectory() / filename;
    std::ofstream ofs{ filepath.str() };
    BOB_ASSERT(ofs.good());
    ofs << std::setprecision(12);
    ofs << "// Generated by generate.cc\n\nstatic const float trueDifferences["
        << NumTestImages << "][" << TestImageSize.width << "] = { \n";

    const auto &differences = pm.getImageDifferences(TestImages[0]);
    for (int row = 0; row < differences.rows(); row++) {
        ofs << "    { ";
        for (int col = 0; col < differences.cols(); col++) {
            ofs << differences(row, col) << ", ";
        }
        ofs << "},\n";
    }
    ofs << "};\n";
}