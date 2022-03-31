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
#include "implicant.hpp"

std::optional<Implicant> try_reduce(Implicant a, Implicant b);
std::vector<size_t> get_part_start_indexes(std::vector<MarkedImplicant>& table,
                                           size_t section_start,
                                           size_t section_end);

template <typename I> void dbg_vector(std::vector<I> const& vec) {
    size_t i = 0;
    for (auto elem : vec) {
        std::cerr << i << ": " << elem << "\n";
        i += 1;
    }
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
    dbg_vector(implicants);

    // Sort initial implicants
    std::sort(implicants.begin(), implicants.end());

    std::cerr << "\nSorted implicants:\n";
    dbg_vector(implicants);

    // Implicants, along with marker of whether it's been reduced or not
    std::vector<MarkedImplicant> table;

    // Insert initial implicants into the table
    std::transform(implicants.begin(), implicants.end(),
                   std::back_inserter(table),
                   [](Implicant imp) { return MarkedImplicant(imp); });

    std::cerr << "\nMarked implicants:\n";
    dbg_vector(table);

    bool done = false;
    // index of start of this section (inclusive)
    size_t section_start = 0;
    while (done == false) {
        // whether there's some terms reduced in this iteration
        bool has_progress = false;

        // index of end of this section (non-inclusive)
        size_t section_end = table.size();

        auto const part_start_indexes =
            get_part_start_indexes(table, section_start, section_end);
        std::cerr << "\nPart start indexes:\n";
        dbg(part_start_indexes);

        // run through each neighboring parts
        //
        // NOTE: This part uses indexes instead of iterators because iterators
        // are invalidated by 'push_back'
        for (size_t prev_part = 0; prev_part < part_start_indexes.size() - 1;
             prev_part += 1) {
            size_t next_part = prev_part + 1;
            bool next_part_is_last = next_part == part_start_indexes.size() - 1;

            size_t prev_part_start_index = part_start_indexes[prev_part];
            size_t prev_part_end_index = part_start_indexes[prev_part + 1];
            size_t next_part_start_index = part_start_indexes[next_part];
            size_t next_part_end_index =
                next_part_is_last ? section_end
                                  : (part_start_indexes[next_part + 1]);

            for (size_t i = prev_part_start_index; i < prev_part_end_index;
                 i += 1) {
                for (size_t j = next_part_start_index; j < next_part_end_index;
                     j += 1) {
                    if (auto imp = try_reduce(table[i].imp, table[j].imp)) {
                        Implicant reduced_imp = *imp;

                        // Reduced
                        std::cerr << "Reducing " << table[i] << " and "
                                  << table[j] << " into " << reduced_imp
                                  << "\n";
                        table.push_back(MarkedImplicant(reduced_imp));
                        table[i].mark_reduced();
                        table[j].mark_reduced();
                        has_progress |= true;
                    }
                }
            }
        }

        // Sort and dedup the new section
        std::sort(table.begin() + section_end, table.end());
        table.erase(std::unique(table.begin() + section_end, table.end()),
                    table.end());

        // Update starting point of the next section
        section_start = section_end;

        // break if stall
        if (has_progress == false) {
            std::cerr << "No progress, breaking the loop\n";
            break;
        }
    }

    std::cerr << "\nPrimary implicants in table:\n";
    for (auto marked_imp : table) {
        if (marked_imp.reduced == false)
            dbg(marked_imp);
    }

    // Write output
    std::ofstream outfile(argv[2]);

    size_t prime_count = 0;
    size_t lit_count = 0;
    for (auto const& mimp : table) {
        if (mimp.reduced) {
            continue;
        }
        prime_count += 1;
        lit_count += mimp.imp.num_lits();
    }

    outfile << lit_count << "\n";
    outfile << prime_count << "\n";

    for (auto const& mimp : table) {
        if (mimp.reduced == false) {
            mimp.imp.print_raw(outfile);
            outfile << "\n";
        }
    }

    return 0;
}

// Try to reduce two implicants if they're only different in one bit
std::optional<Implicant> try_reduce(Implicant a, Implicant b) {
    size_t nvars = a.values.size();
    size_t num_diff = 0;
    size_t diff_index = 0;

    // Find number of different variables
    for (size_t i = 0; i < nvars; i += 1) {
        if (a.values[i] != b.values[i]) {
            num_diff += 1;
            diff_index = i;

            // Early return if there's too many differences
            if (num_diff > 1) {
                return std::nullopt;
            }
        }
    }

    if (num_diff == 0) {
        assert(false);
    } else if (num_diff == 1) {
        // Return new implicant with variable at the only one different place
        // set as don't care
        Implicant imp = a;
        imp.values[diff_index] = DC;
        return imp;
    } else {
        assert(false);
    }
}

// Get starting indexes of each consecutive part of the table, by their pos lit
// counts
std::vector<size_t> get_part_start_indexes(std::vector<MarkedImplicant>& table,
                                           size_t section_start,
                                           size_t section_end) {
    std::vector<size_t> part_start_indexes{};

    std::optional<size_t> last_num_pos_lits = std::nullopt;
    for (size_t i = section_start; i < section_end; i += 1) {
        size_t num_pos_lits = table[i].imp.num_pos_lits();

        if (num_pos_lits != last_num_pos_lits) {
            part_start_indexes.push_back(i);
            last_num_pos_lits = num_pos_lits;
        }
    }

    return part_start_indexes;
}
