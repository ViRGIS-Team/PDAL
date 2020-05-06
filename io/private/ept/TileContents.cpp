/******************************************************************************
 * Copyright (c) 2020, Hobu Inc.
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

#include <io/LasReader.hpp>
#include <pdal/compression/ZstdCompression.hpp>

#include "TileContents.hpp"

    namespace pdal
    {

    void TileContents::read()
    {
        if (m_info->dataType() == EptInfo::DataType::Laszip)
        readLaszip();
    else if (m_info->dataType() == EptInfo::DataType::Binary)
        readBinary();
#ifdef PDAL_HAVE_ZSTD
    else if (m_info->dataType() == EptInfo::DataType::Zstandard)
        readZstandard();
#endif
    else
        throw ept_error("Unrecognized EPT dataType");

    // Read addon information after the native data, we'll possibly
    // overwrite attributes.
    for (const Addon& addon : m_info->addons())
        readAddon(addon, m_view->size());
}

void TileContents::readLaszip()
{
    // If the file is remote (HTTP, S3, Dropbox, etc.), getLocalHandle will
    // download the file and `localPath` will return the location of the
    // downloaded file in a temporary directory.  Otherwise it's a no-op.
    std::string filename = "ept-data/" + m_overlap.m_key.toString() + ".laz";
    auto handle = m_info->endpoint().getLocalHandle(filename);

    m_table.reset(new PointTable);

    Options options;
    options.add("filename", handle.localPath());
    options.add("use_eb_vlr", true);

    LasReader reader;
    reader.setOptions(options);

    static std::mutex s_mutex;
    std::unique_lock<std::mutex> lock(s_mutex);
    reader.prepare(*m_table);  // Geotiff SRS initialization is not thread-safe.
    lock.unlock();

    PointViewSet s = reader.execute(*m_table);
    m_view = *s.begin();
}

void TileContents::readBinary()
{
    std::string filename = "ept-data/" + m_overlap.m_key.toString() + ".bin";
    auto data(m_info->endpoint().getBinary(filename));

    VectorPointTable *vpt = new VectorPointTable(m_info->remoteLayout());
    vpt->buffer() = std::move(data);
    m_table.reset(vpt);
    m_view.reset(new PointView(*m_table, vpt->numPoints()));

    transform();
}

#ifdef PDAL_HAVE_ZSTD
void TileContents::readZstandard()
{
    std::string filename = "ept-data/" + m_overlap.m_key.toString() + ".zst";
    auto compressed(m_info->endpoint().getBinary(filename));
    std::vector<char> data;
    pdal::ZstdDecompressor dec([&data](char* pos, std::size_t size)
    {
        data.insert(data.end(), pos, pos + size);
    });

    dec.decompress(compressed.data(), compressed.size());

    VectorPointTable *vpt = new VectorPointTable(m_info->remoteLayout());
    vpt->buffer() = std::move(data);
    m_table.reset(vpt);
    m_view.reset(new PointView(*m_table, vpt->numPoints()));

    transform();
}
#else
void TileContents::readZstandard() const
{
}
#endif // PDAL_HAVE_ZSTD

void TileContents::readAddon(const Addon& addon, size_t expectedPts)
{
    const Key& key = m_overlap.m_key;
    point_count_t np = m_overlap.m_count;

    if (np == 0)
        return;

    // If the addon hierarchy exists, it must match the EPT data.
    if (np != expectedPts)
        throw pdal_error("Invalid addon hierarchy");

    std::string filename = "ept-data/" + key.toString() + ".bin";
    const auto data(addon.endpoint().getBinary(filename));

    if (np * Dimension::size(addon.type()) != data.size())
        throw pdal_error("Invalid addon content length");

    VectorPointTable *vpt = new VectorPointTable(addon.layout());
    vpt->buffer() = std::move(data);
    m_addonTables[addon.srcId()] = BasePointTablePtr(vpt);
}

void TileContents::transform()
{
    using D = Dimension::Id;

    // Shorten long name.
    const XForm& xf = m_info->dimType(Dimension::Id::X).m_xform;
    const XForm& yf = m_info->dimType(Dimension::Id::Y).m_xform;
    const XForm& zf = m_info->dimType(Dimension::Id::Z).m_xform;

    for (PointRef p : *m_view)
    {
        // Scale the XYZ values.
        p.setField(D::X, p.getFieldAs<double>(D::X) * xf.m_scale.m_val +
            xf.m_offset.m_val);
        p.setField(D::Y, p.getFieldAs<double>(D::Y) * yf.m_scale.m_val +
            yf.m_offset.m_val);
        p.setField(D::Z, p.getFieldAs<double>(D::Z) * zf.m_scale.m_val +
            zf.m_offset.m_val);
    }
}

} // namespace pdal

