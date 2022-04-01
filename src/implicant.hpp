#ifndef IMPLICANT_HPP_
#define IMPLICANT_HPP_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <istream>
#include <vector>

#include "dbg.h"

#define T 1
#define F 0
// don't care
#define DC -1

#define NTHBIT(NUM, N) ((NUM >> N) & 1)

size_t ipow(size_t x, size_t p);

struct Implicant {
    using Values = std::vector<char>;
    Values values;

    inline Implicant(Values values) : values(values) {}
    inline Implicant(Implicant const& rhs) : values(rhs.values) {}

    // Constructs a new 'Implicant'
    static Implicant read_from(std::istream& is, int nvars);

    // Construct a new 'Implicant' containing a single vertice
    static Implicant from_vertice(size_t nvars, size_t vertice);

    // Returns the number of positive literals
    size_t num_pos_lits() const;

    // Returns the number of non-DC literals
    size_t num_lits() const;

    // Print the implicant in its raw form like the input
    //
    // For example "1010--11"
    void print_raw(std::ostream& os) const;

    // Check whether the implicant covers the vertice
    bool covers_vertice(size_t vertice) const;

    // Get all vertices covered by the implicant
    template <typename C> void for_each_covered(C callback) {
        // indexes of DCs
        std::vector<size_t> dcs{};
        size_t base = 0;
        for (size_t i = 0; i < values.size(); i += 1) {
            if (values[i] == DC) {
                dcs.push_back(i);
            } else if (values[i] == T) {
                // set i-th bit
                base |= 1 << i;
            }
        }

        for (size_t v = 0; v < ipow(2, dcs.size()); v += 1) {
            // put every bit of v to corresponding position in vertice
            size_t vertice = base;

            for (size_t i = 0; i < dcs.size(); i += 1) {
                size_t dc = dcs[i];

                // set dc-th bit of vertice to i-th bit of v
                if (NTHBIT(v, i)) {
                    vertice |= 1 << dc;
                } else {
                    vertice &= ~(1 << dc);
                }
            }

            callback(vertice);
        }
    }

    inline size_t nvars() const { return values.size(); }

    // Defaults to soring by number of positive literals
    inline bool operator<(const Implicant& imp) const {
        size_t la = num_pos_lits();
        size_t lb = imp.num_pos_lits();
        if (la == lb) {
            return values < imp.values;
        } else {
            return la < lb;
        }
    }

    // Defaults to comparing the underlying literal values
    inline bool operator==(const Implicant& imp) const {
        return values == imp.values;
    }

    friend std::ostream& operator<<(std::ostream& os, const Implicant& imp);
};

struct MarkedImplicant {
    Implicant imp;
    bool reduced = false;

    inline MarkedImplicant(Implicant imp) : imp(imp) {}
    inline MarkedImplicant(MarkedImplicant const& rhs)
        : imp(rhs.imp), reduced(rhs.reduced) {}

    inline void mark_reduced() { reduced = true; }

    // Compare the contained 'Implicant' and ignore reduced flag
    inline bool operator<(const MarkedImplicant& mimp) const {
        return imp < mimp.imp;
    }

    // Compare the contained 'Implicant' and ignore reduced flag
    inline bool operator==(const MarkedImplicant& mimp) const {
        return imp == mimp.imp;
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const MarkedImplicant& mimp);
};

// print 'Implicant'
std::ostream& operator<<(std::ostream& os, const Implicant& imp);

// print 'MarkedImplicant'
std::ostream& operator<<(std::ostream& os, const MarkedImplicant& mimp);

#endif // ifndef IMPLICANT_HPP_
