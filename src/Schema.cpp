/******************************************************************************
 * $Id$
 *
 * Project:  libLAS - http://liblas.org - A BSD library for LAS format data.
 * Purpose:  LAS Schema implementation for C++ libLAS
 * Author:   Howard Butler, hobu.inc@gmail.com
 *
 ******************************************************************************
 * Copyright (c) 2010, Howard Butler
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following
 * conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of the Martin Isenburg or Iowa Department
 *       of Natural Resources nor the names of its contributors may be
 *       used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 ****************************************************************************/

#include <pdal/Schema.hpp>

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/json_parser.hpp>


#ifdef PDAL_HAVE_LIBXML2
#include <pdal/XMLSchema.hpp>
#endif

namespace pdal
{


Schema::Schema()
{
    return;
}


/// copy constructor
Schema::Schema(Schema const& other) 
    : m_dimensions(other.m_dimensions)
    , m_dimensions_map(other.m_dimensions_map)
{

}


// assignment constructor
Schema& Schema::operator=(Schema const& rhs)
{
    if (&rhs != this)
    {
        m_dimensions = rhs.m_dimensions;
        m_dimensions_map = rhs.m_dimensions_map;
    }

    return *this;
}


bool Schema::operator==(const Schema& other) const
{
    if (m_dimensions == other.m_dimensions &&
        m_dimensions_map == other.m_dimensions_map)
    {
        return true;
    }

    return false;
}


bool Schema::operator!=(const Schema& other) const
{
  return !(*this==other);
}



void Schema::appendDimension(const Dimension& dim)
{
    m_dimensions.push_back(dim);
    std::pair<Dimension::Id, std::size_t> p(dim.getId(), m_dimensions.size()-1);
    m_dimensions_map.insert(p);
    return;
}


const Dimension& Schema::getDimension(std::size_t index) const
{
    return m_dimensions[index];
}


Dimension& Schema::getDimension(std::size_t index)
{
    return m_dimensions[index];
}


const std::vector<Dimension>& Schema::getDimensions() const
{
    return m_dimensions;
}

std::vector<Dimension>& Schema::getDimensions() 
{
    return m_dimensions;
}


int Schema::getDimensionIndex(const Dimension::Id& id) const
{
    std::map<Dimension::Id, std::size_t>::const_iterator i = m_dimensions_map.find(id);
    
    int m = 0;
    
    if (i == m_dimensions_map.end()) 
        m = -1;
    else
        m = i->second;

    return m;
}


int Schema::getDimensionIndex(const Dimension& dim) const
{
    return getDimensionIndex(dim.getId());
}


bool Schema::hasDimension(const Dimension::Id& field) const
{
    int t = getDimensionIndex(field);
    if (t == -1)
        return false;
    return true;
}


const Dimension& Schema::getDimension(const Dimension::Id& field) const
{
    int t = getDimensionIndex(field);
    return m_dimensions[t];
}


Dimension& Schema::getDimension(const Dimension::Id& field)
{
    int t = getDimensionIndex(field);
    return m_dimensions[t];
}


//bool Schema::hasDimension(const Dimension& dim) const
//{
//    return hasDimension(dim.getId());
//}



Schema Schema::from_xml(std::string const& xml, std::string const& xsd)
{
#ifdef PDAL_HAVE_LIBXML2

    pdal::schema::Reader reader(xml, xsd);
    
    pdal::Schema schema = reader.getSchema();
    return schema;

#else
    return Schema();
#endif
}

Schema Schema::from_xml(std::string const& xml)
{
#ifdef PDAL_HAVE_LIBXML2
    
    std::string xsd("");
    
    pdal::schema::Reader reader(xml, xsd);
    
    pdal::Schema schema = reader.getSchema();
    return schema;

#else
    return Schema();
#endif
}

std::string Schema::to_xml(Schema const& schema)
{
#ifdef PDAL_HAVE_LIBXML2

    pdal::schema::Writer writer(schema);
    
    return writer.getXML();

#else
    return std::string("");
#endif
}


boost::property_tree::ptree Schema::toPTree() const
{
    boost::property_tree::ptree tree;

    for (std::vector<Dimension>::const_iterator iter = m_dimensions.begin(); iter != m_dimensions.end(); ++iter)
    {
        const Dimension& dim = *iter;
        tree.add_child("dimension", dim.toPTree());
    }

    return tree;
}


void Schema::dump() const
{
    std::cout << *this;
}


std::ostream& operator<<(std::ostream& os, pdal::Schema const& schema)
{
    boost::property_tree::ptree tree = schema.toPTree();

    boost::property_tree::write_json(os, tree);

    return os;
}


} // namespace pdal
