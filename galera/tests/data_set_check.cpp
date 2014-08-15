/* Copyright (C) 2013 Codership Oy <info@codership.com>
 *
 * $Id$
 */

#undef NDEBUG

#include "../src/data_set.hpp"

#include "gu_logger.hpp"
#include "gu_hexdump.hpp"

#include <check.h>

using namespace galera;

class TestBaseName : public gu::Allocator::BaseName
{
    std::string str_;

public:

    TestBaseName(const char* name) : str_(name) {}
    void print(std::ostream& os) const { os << str_; }
};

class TestRecord
{
public:

    TestRecord (size_t size, const char* str) :
        size_(size),
        buf_(reinterpret_cast<gu::byte_t*>(::malloc(size_))),
        str_(reinterpret_cast<const char*>(buf_) + sizeof(uint32_t)),
        own_(true)
    {
        if (0 == buf_) throw std::runtime_error("failed to allocate record");
        void* tmp = const_cast<void*>(buf_);
        *reinterpret_cast<uint32_t*>(tmp) = htog32(size_);
        ::strncpy (const_cast<char*>(str_), str, size_ - 4);
    }

    TestRecord (const void* const buf, ssize_t const size) :
        size_(TestRecord::serial_size(buf, size)),
        buf_(buf),
        str_(reinterpret_cast<const char*>(buf_) + sizeof(uint32_t)),
        own_(false)
    {}

    TestRecord (const TestRecord& t)
    : size_(t.size_), buf_(t.buf_), str_(t.str_), own_(false) {}

    virtual ~TestRecord () { if (own_) free (const_cast<void*>(buf_)); }

    const void* buf()     const { return buf_; }

    const char* c_str()   const { return str_; }

    ssize_t serial_size() const { return my_serial_size(); }

    static ssize_t serial_size(const void* const buf, ssize_t const size)
    {
        check_buf (buf, size, 1);
        return gtoh32 (*reinterpret_cast<const uint32_t*>(buf));
    }

    bool operator!= (const TestRecord& t) const
    {
        return (size_ != t.size_ || ::memcmp(buf_, t.buf_, size_));
    }

    bool operator== (const TestRecord& t) const
    {
        return (!(*this != t));
    }

private:

    size_t const            size_;
    const void* const buf_;
    const char* const       str_;
    bool const              own_;

    ssize_t my_serial_size () const { return size_; };

    ssize_t my_serialize_to (void* buf, ssize_t size) const
    {
        check_buf (buf, size, size_);

        ::memcpy (buf, buf_, size_);

        return size_;
    }

    static void check_buf (const void* const buf,
                           ssize_t const     size,
                           ssize_t           min_size)
    {
        if (gu_unlikely (buf == 0 || size < min_size))
            throw std::length_error("buffer too short");
    }

    TestRecord& operator= (const TestRecord&);
};


START_TEST (ver0)
{
    size_t const MB = 1 << 20;

    TestRecord rout0(128,  "abc0");
    TestRecord rout1(127,  "abc1");
    TestRecord rout2(126,  "012345");
    TestRecord rout3(125,  "defghij");
    TestRecord rout4(3*MB, "klm");
    TestRecord rout5(1*MB, "qpr");

    std::vector<TestRecord*> records;
    records.push_back (&rout0);
    records.push_back (&rout1);
    records.push_back (&rout2);
    records.push_back (&rout3);
    records.push_back (&rout4);
    records.push_back (&rout5);

    gu::byte_t reserved[1024];
    TestBaseName str("data_set_test");
    DataSetOut dset_out(reserved, sizeof(reserved), str, DataSet::VER1);

    size_t offset(dset_out.size());

    // this should be allocated inside current page
    offset += dset_out.append (rout0.buf(), rout0.serial_size(), true);
    fail_if (dset_out.size() != offset,
             "expected: %zu, got %zu", offset, dset_out.size());

    // this should trigger new page since not stored
    offset += dset_out.append (rout1.buf(), rout1.serial_size(), false);
    fail_if (dset_out.size() != offset);

    // this should trigger new page since previous one was not stored
    offset += dset_out.append (rout2.buf(), rout2.serial_size(), true);
    fail_if (dset_out.size() != offset);

    // this should trigger a new page, since not stored
    offset += dset_out.append (rout3.buf(), rout3.serial_size(), false);
    fail_if (dset_out.size() != offset);

    // this should trigger new page, because won't fit in the current page
    offset += dset_out.append (rout4.buf(), rout4.serial_size(), true);
    fail_if (dset_out.size() != offset);

    // this should trigger new page, because 4MB RAM limit exceeded
    offset += dset_out.append (rout5.buf(), rout5.serial_size(), false);
    fail_if (dset_out.size() != offset);

    fail_if (1 != size_t(dset_out.count()));

    DataSetOut::GatherVector out_bufs;
    out_bufs().reserve (dset_out.page_count());

    size_t min_out_size(0);
    for (size_t i = 0; i < records.size(); ++i)
    {
        min_out_size += records[i]->serial_size();
    }

    size_t const out_size (dset_out.gather (out_bufs));

    fail_if (out_size <= min_out_size || out_size > offset);
    fail_if (out_bufs->size() != static_cast<size_t>(dset_out.page_count()),
             "Expected %zu buffers, got: %zd",
             dset_out.page_count(), out_bufs->size());

    /* concatenate all buffers into one */
    std::vector<gu::byte_t> in_buf;
    in_buf.reserve(out_size);
    mark_point();
    for (size_t i = 0; i < out_bufs->size(); ++i)
    {
        fail_if (0 == out_bufs[i].ptr);

        log_info << "\nadding buf " << i << ": "
                 << gu::Hexdump(out_bufs[i].ptr,
                                std::min<ssize_t>(out_bufs[i].size, 24), true);

        size_t old_size = in_buf.size();

        const gu::byte_t* ptr
            (reinterpret_cast<const gu::byte_t*>(out_bufs[i].ptr));
        in_buf.insert (in_buf.end(), ptr, ptr + out_bufs[i].size);

        fail_if (old_size + out_bufs[i].size != in_buf.size());
    }

    fail_if (in_buf.size() != out_size,
             "Sent buf size: %zu, recvd buf size: %zu",
             out_size, in_buf.size());

    log_info << "Resulting DataSet buffer:\n"
             << gu::Hexdump(in_buf.data(), 32, false) << '\n'
             << gu::Hexdump(in_buf.data(), 32, true);

    galera::DataSetIn const dset_in(dset_out.version(),
                                    in_buf.data(), in_buf.size());

    fail_if (dset_in.size()  != dset_out.size());
    fail_if (dset_in.count() != dset_out.count());

    for (ssize_t i = 0; i < dset_in.count(); ++i)
    {
        gu::Buf data = dset_in.next();
        TestRecord const rin(data.ptr, data.size);
        fail_if (rin != *records[i], "Record %d failed: expected %s, found %s",
                 i, records[i]->c_str(), rin.c_str());
    }

    galera::DataSetIn dset_in_empty;
    dset_in_empty.init(dset_out.version(), in_buf.data(), in_buf.size());

    fail_if (dset_in_empty.size()  != dset_out.size());
    fail_if (dset_in_empty.count() != dset_out.count());

    for (ssize_t i = 0; i < dset_in_empty.count(); ++i)
    {
        gu::Buf data = dset_in_empty.next();
        TestRecord const rin(data.ptr, data.size);
        fail_if (rin != *records[i], "Record %d failed: expected %s, found %s",
                 i, records[i]->c_str(), rin.c_str());
    }
}
END_TEST

Suite* data_set_suite ()
{
    TCase* t = tcase_create ("DataSet");
    tcase_add_test (t, ver0);
    tcase_set_timeout(t, 60);

    Suite* s = suite_create ("DataSet");
    suite_add_tcase (s, t);

    return s;
}
