#ifndef IMPLICANT_HPP_
#define IMPLICANT_HPP_

#include <algorithm>
#include <cassert>
#include <iostream>
#include <istream>
#include <vector>

#define T 1
#define F 0
// don't care
#define DC -1

struct Implicant {
    using Values = std::vector<char>;
    Values values;

    inline Implicant(Values values) : values(values) {}
    inline Implicant(Implicant const& rhs) : values(rhs.values) {}

    // Constructs a new 'Implicant'
    static Implicant read_from(std::istream& is, int nvars);

    // Returns the number of positive literals
    size_t num_pos_lits() const;

    // Returns the number of non-DC literals
    size_t num_lits() const;

    void print_raw(std::ostream& os) const;

    // Defaults to soring by number of positive literals
    inline bool operator<(const Implicant& imp) const {
        return num_pos_lits() < imp.num_pos_lits();
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
