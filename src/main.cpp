#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "dbg.h"
#include "implicant.hpp"

using std::vector;

std::optional<Implicant> try_reduce(Implicant a, Implicant b);
vector<size_t> get_part_start_indexes(vector<MarkedImplicant>& table,
                                      size_t section_start, size_t section_end);
std::tuple<vector<Implicant>, int, int> read_implicants(char const* filename);
vector<Implicant> find_prime_implicants(vector<MarkedImplicant>& table);
size_t literal_count_of(vector<Implicant>& imps);
void write_implicants(char const* filename, std::vector<Implicant> imps);

template <typename I> void dbg_vector(char const* msg, vector<I> const& vec) {
    std::cerr << "\n" << msg << "\n";
    size_t i = 0;
    for (auto elem : vec) {
        std::cerr << i << ": " << elem << "\n";
        i += 1;
    }
}

int main(int argc, char** argv) {
    assert(argc == 3);

    auto [input_implicants, nvars, nterms] = read_implicants(argv[1]);
    dbg_vector("Unsorted input_implicants:", input_implicants);

    // Sort initial implicants
    vector<Implicant> sorted_implicants{input_implicants};
    std::sort(sorted_implicants.begin(), sorted_implicants.end());
    dbg_vector("Sorted implicants:", sorted_implicants);

    // Insert initial implicants into the table
    // Implicants, along with marker of whether it's been reduced or not
    vector<MarkedImplicant> table;
    std::transform(sorted_implicants.begin(), sorted_implicants.end(),
                   std::back_inserter(table),
                   [](Implicant imp) { return MarkedImplicant(imp); });
    dbg_vector("Marked implicants:", table);

    // Find prime implicants
    auto primes = find_prime_implicants(table);
    dbg_vector("Prime implicants", primes);

    std::unordered_set<size_t> onset;
    for (auto& prime : primes) {
        dbg(prime);
        prime.for_each_covered([](size_t vertice) { dbg(vertice); });
    }

    // Write output
    write_implicants(argv[2], primes);

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
vector<size_t> get_part_start_indexes(vector<MarkedImplicant>& table,
                                      size_t section_start,
                                      size_t section_end) {
    vector<size_t> part_start_indexes{};

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

std::tuple<vector<Implicant>, int, int> read_implicants(char const* filename) {
    std::ifstream infile(filename);
    vector<Implicant> implicants{};

    int nvars, nterms;
    if (!(infile >> nvars >> nterms)) {
        throw std::runtime_error("Failed to read nvars and (or) nterms");
    }

    for (int term = 0; term < nterms; term += 1) {
        auto imp = Implicant::read_from(infile, nvars);
        std::cerr << imp << "\n";
        implicants.push_back(imp);
    }

    return {implicants, nvars, nterms};
}

size_t literal_count_of(vector<Implicant>& imps) {
    size_t lits = 0;
    for (auto imp : imps) {
        lits += imp.num_lits();
    }
    return lits;
}

vector<Implicant> find_prime_implicants(vector<MarkedImplicant>& table) {
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

    vector<Implicant> primes{};
    for (auto& mimp : table) {
        if (mimp.reduced) {
            continue;
        }
        primes.push_back(mimp.imp);
    }

    std::sort(primes.begin(), primes.end());
    primes.erase(std::unique(primes.begin(), primes.end()), primes.end());

    return primes;
}

void write_implicants(char const* filename, std::vector<Implicant> imps) {
    std::ofstream outfile(filename);

    outfile << literal_count_of(imps) << "\n";
    outfile << imps.size() << "\n";

    for (auto const& prime : imps) {
        prime.print_raw(outfile);
        outfile << "\n";
    }

    outfile.flush();
    outfile.close();
}
