#include <algorithm>
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

    // Constructs a new 'Implicant'
    static Implicant read_from(std::istream& is, int nvars) {
        Values values{};

        for (int i = 0; i < nvars; i += 1) {
            char ch;
            is >> ch;

            std::cerr << "Read ch: " << ch << "\n";

            char value;
            if (ch == '1') {
                value = T;
            } else if (ch == '0') {
                value = F;
            } else if (ch == '-') {
                value = DC;
            } else {
                assert(false);
            }
            values.push_back(value);
        }

        return Implicant(values);
    }

    void dbg_print() const {
        std::cerr << "Implicant(";
        for (auto value : values) {
            if (value == DC) {
                std::cerr << "-";
            } else {
                std::cerr << static_cast<int>(value);
            }
        }
        std::cerr << ")\n";
    }

    // Returns the number of positive literals
    size_t num_pos_lits() const {
        return std::count_if(values.begin(), values.end(),
                             [](char value) { return value == 1; });
    }

    // Defaults to soring by number of positive literals
    bool operator<(const Implicant& imp) const {
        return num_pos_lits() < imp.num_pos_lits();
    }

    // Defaults to comparing the underlying literal values
    bool operator==(const Implicant& imp) const { return values == imp.values; }
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

    std::cerr << "\nUnsorted implicants:\n";
    for (auto imp : implicants) {
        imp.dbg_print();
    }

    // Sort initial implicants
    std::sort(implicants.begin(), implicants.end());

    std::cerr << "\nSorted implicants:\n";
    for (auto imp : implicants) {
        imp.dbg_print();
    }

    // TODO: find primary implicants
    std::vector<std::vector<Implicant>> table;
    table.push_back(implicants);

    return 0;
}
