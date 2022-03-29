#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <istream>
#include <vector>

#include "dbg.h"

// don't care
#define T 1
#define F 0
#define DC -1

struct Implicant {
    using Values = std::vector<char>;
    Values values;

    Implicant(Values values) : values(values) {}
    static Implicant read_from(std::istream& is, int nvars) {
        Values values{};

        for (int i = 0; i < nvars; i += 1) {
            char ch;
            is >> ch;

            std::cerr << "Read ch: " << ch << "\n";

            char value;
            if (ch == '1') {
                value = 1;
            } else if (ch == '0') {
                value = 0;
            } else if (ch == '-') {
                value = -1;
            } else {
                assert(false);
            }
            values.push_back(value);
        }

        return Implicant(values);
    }

    void dbg_print() {
        std::cerr << "Implicant(";
        for (auto value : values) {
            if (value == -1) {
                std::cerr << "-";
            } else {
                std::cerr << static_cast<int>(value);
            }
        }
        std::cerr << ")\n";
    }
};

int main(int argc, char** argv) {
    assert(argc == 3);

    std::ifstream infile(argv[1]);

    int nvars, nterms;
    if (!(infile >> nvars >> nterms)) {
        std::cerr << "Failed to read nvars & nterms\n";
        return 1;
    }

    std::cerr << "nvars  = " << nvars << "\n";
    std::cerr << "nterms = " << nterms << "\n";

    std::vector<Implicant> implicants{};
    for (int term = 0; term < nterms; term += 1) {
        auto imp = Implicant::read_from(infile, nvars);
        imp.dbg_print();
        implicants.push_back(imp);
    }

    std::cerr << "Implicants:\n";
    for (auto imp : implicants) {
        imp.dbg_print();
    }

    return 0;
}
