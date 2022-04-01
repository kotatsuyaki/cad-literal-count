#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "dbg.h"
#include "implicant.hpp"

using std::unordered_set;
using std::vector;

std::optional<Implicant> try_reduce(Implicant a, Implicant b);
vector<size_t> get_part_start_indexes(vector<MarkedImplicant>& table,
                                      size_t section_start, size_t section_end);
std::tuple<vector<Implicant>, int, int> read_implicants(char const* filename);
vector<Implicant> find_prime_implicants(vector<MarkedImplicant>& table);
vector<Implicant> select_prime_implicants(vector<Implicant>& primes,
                                          size_t nvars);
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

template <typename I> void sort_dedup_reverse(vector<I>& vec) {
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
    std::reverse(vec.begin(), vec.end());
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

    // Select a subset of prime implicants that covers the on-set
    auto answers = select_prime_implicants(primes, nvars);

    // Write output
    write_implicants(argv[2], answers);

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

vector<Implicant> select_prime_implicants(vector<Implicant>& primes,
                                          size_t nvars) {
    // mappings between implicants and their covered covertices
    vector<unordered_set<size_t>> itov{};
    vector<unordered_set<size_t>> vtoi{};
    itov.resize(primes.size());
    vtoi.resize(ipow(2, nvars));

    // construct tables
    for (size_t i = 0; i < primes.size(); i += 1) {
        auto const& prime = primes[i];
        prime.for_each_covered([&itov, &vtoi, i](size_t vertice) {
            itov[i].insert(vertice);
            vtoi[vertice].insert(i);
        });
    }
    dbg(itov, vtoi);

    // find those vertices with only one coverer (i.e. essential prime
    // implicants)
    vector<size_t> ess_prime_indexes{};
    vector<size_t> ess_vertice_indexes{};
    for (size_t j = 0; j < itov.size(); j += 1) {
        if (vtoi[j].size() == 1) {
            ess_prime_indexes.push_back(*vtoi[j].begin());
            ess_vertice_indexes.push_back(j);
        }
    }

    vector<Implicant> ess_primes{};

    // delete prime entries from the back
    // NOTE: this invalidates the 'itov' and 'vtoi' tables
    sort_dedup_reverse(ess_prime_indexes);
    for (size_t i : ess_prime_indexes) {
        ess_primes.push_back(primes[i]);
        primes.erase(primes.begin() + i);
    }

    // Reset the already-invalidated tables
    itov.clear();
    itov.resize(primes.size());
    vtoi.clear();
    vtoi.resize(ipow(2, nvars));

    // set of yet-to-be-covered vertices
    unordered_set<size_t> vertices;

    // re-construct itov and vtoi tables
    for (size_t i = 0; i < primes.size(); i += 1) {
        auto const& prime = primes[i];
        prime.for_each_covered([&itov, &vtoi, &vertices, i](size_t vertice) {
            itov[i].insert(vertice);
            vertices.insert(vertice);
            vtoi[vertice].insert(i);
        });
    }

    vector<bool> prime_used(primes.size(), false);
    assert(prime_used.size() == primes.size());

    // loop until all vertices are covered
    while (vertices.empty() == false) {
        std::optional<size_t> best_i;
        float best_score = 0.0f;

        // find the best unused prime implicant
        for (size_t i = 0; i < primes.size(); i += 1) {
            if (prime_used[i]) {
                continue;
            }
            float score = static_cast<float>(itov[i].size()) /
                          static_cast<float>(primes[i].num_lits());
            if (score >= best_score) {
                // pick primes[i]
                best_i = i;
                best_score = score;
            }
        }

        if (best_i.has_value()) {
            // pick primes[i]
            size_t i = best_i.value();
            prime_used[i] = true;

            // remove vertices covered by this prime implicant
            // from the "covered sets" of other implicants
            for (size_t vertice : itov[i]) {
                vertices.erase(vertice);
                for (size_t impi : vtoi[vertice]) {
                    // NOTE: this check prevents itov[i] from being mutated
                    // (which results in UB)
                    if (impi != i) {
                        itov[impi].erase(vertice);
                    }
                }
            }
        } else {
            assert(false);
        }
    }

    // include essential prime implicants and other selected prime implicants in
    // the final answer
    vector<Implicant> answers(ess_primes);
    for (size_t i = 0; i < primes.size(); i += 1) {
        if (prime_used[i]) {
            answers.push_back(primes[i]);
        }
    }

    return answers;
}
