#include "implicant.hpp"

#define NTHBIT(NUM, N) ((NUM >> N) & 1)

// print 'Implicant'
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

// print 'MarkedImplicant'
std::ostream& operator<<(std::ostream& os, const MarkedImplicant& mimp) {
    os << "MarkedImplicant(";
    os << (mimp.reduced ? "_, " : "O, ");
    os << mimp.imp;
    os << ")";
    return os;
}

Implicant Implicant::read_from(std::istream& is, int nvars) {
    Values values{};

    for (int i = 0; i < nvars; i += 1) {
        char ch;
        is >> ch;

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

Implicant Implicant::from_vertice(size_t nvars, size_t vertice) {
    Values values{};
    values.resize(nvars);
    assert(values.size() == nvars);

    for (size_t i = 0; i < nvars; i += 1) {
        values[i] = NTHBIT(vertice, i);
    }

    return Implicant(values);
}

size_t Implicant::num_pos_lits() const {
    return std::count_if(values.begin(), values.end(),
                         [](char value) { return value == 1; });
}

void Implicant::print_raw(std::ostream& os) const {
    for (auto value : values) {
        if (value == T) {
            os << "1";
        } else if (value == F) {
            os << "0";
        } else if (value == DC) {
            os << "-";
        } else {
            assert(false);
        }
    }
}

size_t Implicant::num_lits() const {
    return std::count_if(values.begin(), values.end(),
                         [](char value) { return value != DC; });
}

size_t ipow(size_t x, size_t p) {
    size_t i = 1;
    for (size_t j = 1; j <= p; j++)
        i *= x;
    return i;
}
