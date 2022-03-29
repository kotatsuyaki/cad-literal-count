#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
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

    friend std::ostream& operator<<(std::ostream& os, const Implicant& imp);
};

std::ostream& operator<<(std::ostream& os, const Implicant& imp) {
    os << "Implicant(";
    os << imp.num_pos_lits() << ", ";
    for (auto value : imp.values) {
        if (value == DC) {
            os << "-";
        } else {
            os << static_cast<int>(value);
        }
    }
    os << ")";
    return os;
}

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
        std::cerr << imp << "\n";
        implicants.push_back(imp);
    }

    std::cerr << "\nUnsorted implicants:\n";
    for (auto imp : implicants) {
        std::cerr << imp << "\n";
    }

    // Sort initial implicants
    std::sort(implicants.begin(), implicants.end());

    std::cerr << "\nSorted implicants:\n";
    for (auto imp : implicants) {
        std::cerr << imp << "\n";
    }

    // (implicant, removed?) pairs
    std::vector<std::pair<Implicant, bool>> table;

    // Insert initial implicants into the table
    std::transform(implicants.begin(), implicants.end(),
                   std::back_inserter(table),
                   [](Implicant imp) { return std::make_pair(imp, false); });

    std::cerr << "\nMarked implicants:\n";
    for (auto [imp, removed] : table) {
        std::cerr << (removed ? "(X) " : "(O) ") << imp << "\n";
    }

    // TODO: find primary implicants
    bool done = false;
    // index of start of this section (inclusive)
    size_t section_start = 0;
    while (done == false) {
        // index of end of this section (non-inclusive)
        size_t section_end = table.size();
        std::vector<size_t> part_start_indexes{};

        std::optional<size_t> last_num_pos_lits = std::nullopt;
        for (size_t i = section_start; i < section_end; i += 1) {
            size_t num_pos_lits = table[i].first.num_pos_lits();

            if (num_pos_lits != last_num_pos_lits) {
                part_start_indexes.push_back(i);
                last_num_pos_lits = num_pos_lits;
            }
        }

        std::cerr << "\nPart start indexes:\n";
        dbg(part_start_indexes);

        section_start = section_end;
        break;
    }

    return 0;
}
