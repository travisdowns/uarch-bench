/*
 * table.hpp
 *
 * Simple tabular output.
 */

#ifndef TABLE_HPP_
#define TABLE_HPP_

#include <vector>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <iomanip>

namespace table {

class Table;

struct ColInfo {
    enum { LEFT, RIGHT } justify;
    ColInfo() : justify(LEFT) {}
};

class Row {
    friend Table;
    using row_t = std::vector<std::string>;

    const Table* table_;
    row_t elems_;

    Row(const Table& table) : table_(&table) {}

    /** return a vector of sizes for each element */
    std::vector<size_t> getSizes() const {
        std::vector<size_t> sizes;
        for (const auto& e : elems_) {
            sizes.push_back(e.size());
        }
        return sizes;
    }

    inline void str(std::ostream& os, const std::vector<size_t> sizes) const;

    std::string justify(const ColInfo& cinfo, const std::string& e, size_t w) const {
        // left pad
        std::stringstream ss;
        ss << std::setw(w) << (cinfo.justify == ColInfo::LEFT ? std::left : std::right) << e;
        auto s = ss.str();
        assert(s.size() == w);
        return s;
    }

public:
    /** add a cell to this row with the given element, returns a reference to this row */
    template <typename T>
    Row& add(const T& elem) {
        std::stringstream ss;
        ss << elem;
        elems_.push_back(ss.str());
        return *this;
    }
};

class Table {
    friend Row;
    using table_t   = std::vector<Row>;
    using colinfo_t = std::vector<ColInfo>;

    table_t rows_;
    colinfo_t colinfo_;
public:

    /**
     * Get a reference to the ColInfo object for the given column, which lets you
     * set column-global info such as the justification.
     */
    ColInfo& colInfo(size_t col) {
        if (col >= colinfo_.size()) {
            colinfo_.resize(col + 1);
        }
        return colinfo_.at(col);
    }

    /* in the cost case, return a default ColInfo if it doesn't exist */
    ColInfo colInfo(size_t col) const {
        return col < colinfo_.size() ? colinfo_.at(col) : ColInfo{};
    }

    Row& newRow() {
        rows_.push_back(Row{*this});
        return rows_.back();
    }

    /** return the current representation of the table as a string */
    std::string str() const {

        // calculate max row sizes
        std::vector<size_t> max_sizes;
        for (const auto& r : rows_) {
            std::vector<size_t> sizes = r.getSizes();
            for (size_t c = 0; c < sizes.size(); c++) {
                size_t row_size = sizes[c];
                if (c >= max_sizes.size()) {
                    assert(max_sizes.size() == c);
                    max_sizes.push_back(row_size);
                } else {
                    max_sizes[c] = std::max(max_sizes[c], row_size);
                }
            }
        }

        std::stringstream ss;
        for (const auto& r : rows_) {
            r.str(ss, max_sizes);
            ss << "\n";
        }

        return ss.str();
    }

};

inline void Row::str(std::ostream& os, const std::vector<size_t> sizes) const
{
    bool first = true;
    for (size_t c = 0; c < elems_.size(); c++) {
        const auto& e = elems_[c];
        assert(c < sizes.size());
        if (!first) os << " "; // inter-cell padding
        first = false;
        os << justify(table_->colInfo(c), e, sizes[c]);
    }
}

}


#endif /* TABLE_HPP_ */
