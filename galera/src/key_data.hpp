//
// Copyright (C) 2013 Codership Oy <info@codership.com>
//

#ifndef GALERA_KEY_DATA_HPP
#define GALERA_KEY_DATA_HPP

#include "wsrep_api.h"

namespace galera
{

struct KeyData
{
    const wsrep_buf_t* const parts;
    long const               parts_num;
    int  const               proto_ver;
    wsrep_key_type_t const   type;
    bool const               copy;

    KeyData (int const pv, const wsrep_buf_t* const k,
             long const kn, wsrep_key_type_t const tp, bool const cp)
        : parts     (k),
          parts_num (kn),
          proto_ver (pv),
          type      (tp),
          copy      (cp)
    {}

    KeyData (const KeyData& kd)
    : parts    (kd.parts),
      parts_num(kd.parts_num),
      proto_ver(kd.proto_ver),
      type     (kd.type),
      copy     (kd.copy)
    {}

    bool shared() const { return type == WSREP_KEY_SHARED; }

private:

    KeyData& operator = (const KeyData&);

}; /* struct KeyData */

} /* namespace galera */

#endif /* GALERA_KEY_DATA_HPP */
