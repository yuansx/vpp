/*
 * Copyright (c) 2017 Cisco Systems, Inc. and others.  All rights reserved.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v1.0 which accompanies this distribution,
 * and is available at http://www.eclipse.org/legal/epl-v10.html
 */

#include <cassert>
#include <iostream>

#include "vom/nat_binding.hpp"
#include "vom/cmd.hpp"

using namespace VOM;

singular_db<const nat_binding::key_t, nat_binding> nat_binding::m_db;

nat_binding::event_handler nat_binding::m_evh;

const nat_binding::zone_t nat_binding::zone_t::INSIDE(0, "inside");
const nat_binding::zone_t nat_binding::zone_t::OUTSIDE(0, "outside");

nat_binding::zone_t::zone_t(int v, const std::string s):
    enum_base(v, s)
{
}

/**
 * Construct a new object matching the desried state
 */
nat_binding::nat_binding(const interface &itf,
                         const direction_t &dir,
                         const l3_proto_t &proto,
                         const zone_t &zone):
    m_binding(false),
    m_itf(itf.singular()),
    m_dir(dir),
    m_proto(proto),
    m_zone(zone)
{
}

nat_binding::nat_binding(const nat_binding& o):
    m_binding(o.m_binding),
    m_itf(o.m_itf),
    m_dir(o.m_dir),
    m_proto(o.m_proto),
    m_zone(o.m_zone)
{
}

nat_binding::~nat_binding()
{
    sweep();
    m_db.release(make_tuple(m_itf->key(), m_dir, m_proto), this);
}

void nat_binding::sweep()
{
    if (m_binding)
    {
        if (direction_t::INPUT == m_dir)
        {
            HW::enqueue(new unbind_44_input_cmd(m_binding, m_itf->handle(), m_zone));
        }
        else
        {
            assert(!"Unimplemented");
        }
    }
    HW::write();
}

void nat_binding::replay()
{
    if (m_binding)
    {
        if (direction_t::INPUT == m_dir)
        {
            HW::enqueue(new bind_44_input_cmd(m_binding, m_itf->handle(), m_zone));
        }
        else
        {
            assert(!"Unimplemented");
        }
    }
}

void nat_binding::update(const nat_binding &desired)
{
    /*
     * the desired state is always that the interface should be created
     */
    if (!m_binding)
    {
        if (direction_t::INPUT == m_dir)
        {
            HW::enqueue(new bind_44_input_cmd(m_binding, m_itf->handle(), m_zone));
        }
        else
        {
            assert(!"Unimplemented");
        }
    }
}

std::string nat_binding::to_string() const
{
    std::ostringstream s;
    s << "nat-binding:[" << m_itf->to_string()
      << " " << m_dir.to_string()
      << " " << m_proto.to_string()
      << " " << m_zone.to_string()
      << "]";

    return (s.str());
}

std::shared_ptr<nat_binding> nat_binding::find_or_add(const nat_binding &temp)
{
    return (m_db.find_or_add(make_tuple(temp.m_itf->key(), temp.m_dir, temp.m_proto), temp));
}

std::shared_ptr<nat_binding> nat_binding::singular() const
{
    return find_or_add(*this);
}

void nat_binding::dump(std::ostream &os)
{
    m_db.dump(os);
}

std::ostream& VOM::operator<<(std::ostream &os, const nat_binding::key_t &key)
{
    os << "["  << std::get<0>(key)
       << ", " << std::get<1>(key)
       << ", " << std::get<2>(key)
       << "]";

    return (os);
}

nat_binding::event_handler::event_handler()
{
    OM::register_listener(this);
    inspect::register_handler({"nat-binding"}, "NAT bindings", this);
}

void nat_binding::event_handler::handle_replay()
{
    m_db.replay();
}

void nat_binding::event_handler::handle_populate(const client_db::key_t &key)
{
    /**
     * This is done while populating the interfaces
     */
}

dependency_t nat_binding::event_handler::order() const
{
    return (dependency_t::BINDING);
}

void nat_binding::event_handler::show(std::ostream &os)
{
    m_db.dump(os);
}
