#ifndef OSMIUM_IO_PBF_INPUT_RANDOMACCESS_HPP
#define OSMIUM_IO_PBF_INPUT_RANDOMACCESS_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2023 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

/**
 * @file
 *
 * Include this file if you want to read OSM PBF files out of order, also called random access.
 *
 * @attention If you include this file, you'll need to link with
 *            `libz`, and enable multithreading.
 */

#include <osmium/io/detail/pbf_input_format.hpp>
#include <osmium/io/detail/pbf_decoder.hpp>
#include <osmium/util/file.hpp>

#include <protozero/pbf_message.hpp>
#include <protozero/types.hpp>

#include <string>

namespace osmium {

    namespace io {

        namespace detail {

            /* BlobHeaders without indexdata are usually only 13-14 bytes. */
            static constexpr uint32_t max_small_blob_header_size {64};

            /* Blocks are usually around 60 KiB - 500 KiB, so anything about 20 MiB is suspicious. */
            static constexpr size_t max_block_size {20 * 1024 * 1024};

            static uint32_t check_small_size(uint32_t size) {
                if (size > static_cast<uint32_t>(max_small_blob_header_size)) {
                    throw osmium::pbf_error{"invalid small BlobHeader size (> max_small_blob_header_size)"};
                }
                return size;
            }

            /* The following functions are largely copied from PBFParser, since these methods are private and cannot be re-used otherwise.
             * TODO: Implement these functions only once. */

            static uint32_t get_size_in_network_byte_order(const char* d) noexcept {
                return (static_cast<uint32_t>(d[3])) |
                       (static_cast<uint32_t>(d[2]) <<  8U) |
                       (static_cast<uint32_t>(d[1]) << 16U) |
                       (static_cast<uint32_t>(d[0]) << 24U);
            }

            /**
             * Read exactly size bytes from fd into buffer.
             *
             * @pre Value in size parameter must fit in unsigned int
             * @returns true if size bytes could be read
             *          false if EOF was encountered
             */
            static bool read_exactly(int fd, char* buffer, std::size_t size) {
                std::size_t to_read = size;

                while (to_read > 0) {
                    auto const read_size = osmium::io::detail::reliable_read(fd, buffer + (size - to_read), static_cast<unsigned int>(to_read));
                    if (read_size == 0) { // EOF
                        return false;
                    }
                    to_read -= read_size;
                }

                return true;
            }

            /**
             * Read 4 bytes in network byte order from file. They contain
             * the length of the following BlobHeader.
             */
            uint32_t read_blob_header_size_from_file(int fd) {
                std::array<char, sizeof(uint32_t)> buffer{};
                if (!detail::read_exactly(fd, buffer.data(), buffer.size())) {
                    throw osmium::pbf_error{"unexpected EOF in blob header size"};
                }
                return detail::check_small_size(detail::get_size_in_network_byte_order(buffer.data()));
            }

            /**
             * Decode the BlobHeader. Make sure it contains the expected
             * type. Return the size of the following Blob.
             */
            static size_t decode_blob_header(const protozero::data_view& data, const char* expected_type) {
                protozero::pbf_message<FileFormat::BlobHeader> pbf_blob_header{data};
                protozero::data_view blob_header_type;
                size_t blob_header_datasize = 0;

                while (pbf_blob_header.next()) {
                    switch (pbf_blob_header.tag_and_type()) {
                        case protozero::tag_and_type(FileFormat::BlobHeader::required_string_type, protozero::pbf_wire_type::length_delimited):
                            blob_header_type = pbf_blob_header.get_view();
                            break;
                        case protozero::tag_and_type(FileFormat::BlobHeader::required_int32_datasize, protozero::pbf_wire_type::varint):
                            blob_header_datasize = pbf_blob_header.get_int32();
                            break;
                        default:
                            pbf_blob_header.skip();
                    }
                }

                if (blob_header_datasize == 0) {
                    throw osmium::pbf_error{"PBF format error: BlobHeader.datasize missing or zero."};
                }

                if (std::strncmp(expected_type, blob_header_type.data(), blob_header_type.size()) != 0) {
                    throw osmium::pbf_error{"blob does not have expected type (OSMHeader in first blob, OSMData in following blobs)"};
                }

                return blob_header_datasize;
            }

        } // namespace detail

        struct pbf_block_start {
            size_t file_offset;
            osmium::object_id_type first_item_id_or_zero;
            uint32_t datasize;
            // "first_item_type_or_zero" and "first_item_id_or_zero" are zero if that block has never been read before.
            osmium::item_type first_item_type_or_zero;
            // The weird order avoids silly padding in the struct (2 bytes instead of 10).
        };

        class PbfBlockIndexTable {
            std::vector<pbf_block_start> m_block_starts;
            int m_fd;

            size_t digest_and_skip_block(size_t current_offset, std::size_t file_size, bool should_index_block) {
                uint32_t blob_header_size = detail::read_blob_header_size_from_file(m_fd);
                current_offset += 4;

                std::string buffer;
                buffer.resize(blob_header_size);
                if (!detail::read_exactly(m_fd, &*buffer.begin(), blob_header_size)) {
                    throw osmium::pbf_error{"unexpected EOF in blob header"};
                }
                current_offset += blob_header_size;

                const char* expected_type = should_index_block ? "OSMData" : "OSMHeader";
                size_t blob_body_size = detail::decode_blob_header(protozero::data_view{buffer.data(), blob_header_size}, expected_type);
                // TODO: Check for "Sort.Type_then_ID" in optional_features, if desired.
                // (Planet has it, most extracts have it, but test data doesn't have it.)
                if (blob_body_size > detail::max_block_size) {
                    throw osmium::pbf_error{"invalid Block size (> max_block_size)"};
                }
                if (should_index_block) {
                    m_block_starts.emplace_back(pbf_block_start {
                        current_offset, // file_offset
                        0, // first_item_id_or_zero
                        static_cast<uint32_t>(blob_body_size), // block_datasize
                        osmium::item_type::undefined // first_item_type_or_zero
                    });
                }

                current_offset += blob_body_size;
                osmium::util::file_seek(m_fd, current_offset);
                return current_offset;
            }

        public:
            /**
             * Open and index the given pbf file for future random access. This reads every block
             * *header* (not body) in the file, and allocates roughly 24 bytes for each data block.
             * Usually this scan is extremely quick. For reference, planet has roughly 50k blocks at
             * the time of writing, which means only roughly 1 MiB of index data.
             *
             * If size_t is only 32 bits, then this will fail for files bigger than 2 GiB.
             *
             * Note that io::Reader cannot be used, since it buffers, insists on parsing and
             * decompressing all blocks, and probably breaks due to seek().
             */
            PbfBlockIndexTable(const std::string& filename)
            {
                /* As we expect a reasonably large amount of entries, avoid unnecessary
                 * reallocations in the beginning: */
                m_block_starts.reserve(1000);
                m_fd = osmium::io::detail::open_for_reading(filename);
                /* TODO: Use a 64-bit interface here */
                /* TODO: At least check whether the file is larger than 2 GB and abort if 32-bit platform. */
                std::size_t file_size = osmium::util::file_size(m_fd);

                std::size_t offset = digest_and_skip_block(0, file_size, false); // HeaderBlock
                /* Data blocks, if any: */
                while (offset < file_size) {
                    offset = digest_and_skip_block(offset, file_size, true);
                }
                /* If this is a 32-bit platform and the file is larger than 2 GiB, then there is a
                 * *chance* we can detect the problem by observing a seeming read past the end, e.g.
                 * if the file size is 4GB + 1234 bytes, which causes file_size to be just 1234:
                 */
                if (offset > file_size) {
                    throw osmium::pbf_error{"file either grew, or otherwise did not have expected size (perhaps 32-bit truncation?)"};
                }
            }

            ~PbfBlockIndexTable() {
                osmium::io::detail::reliable_close(m_fd);
            }

            const std::vector<pbf_block_start>& block_starts() const {
                return m_block_starts;
            }

            /**
             * Reads and parses a block into a given buffer. Note that this class does not cache
             * recently-accessed blocks, and thus cannot be used in parallel.
             *
             * @pre block_index must be a valid index into m_block_starts.
             * @returns The decoded block
             */
            osmium::memory::Buffer get_parsed_block(size_t block_index, const osmium::io::read_meta read_metadata) {
                /* Because we might need to read the block to update m_block_starts, *all* item types must be decoded. This should not be a problem anyway, because the block likely only contains items of the desired type, as items should be sorted first by type, then by ID. */
                const osmium::osm_entity_bits::type read_types = osmium::osm_entity_bits::all;

                auto& block_start = m_block_starts[block_index];
                /* Because of the write-access to m_block_starts and file seeking, this cannot be
                 * easily parallelized. */
                osmium::util::file_seek(m_fd, block_start.file_offset);

                std::string input_buffer;
                input_buffer.resize(block_start.datasize);
                if (!osmium::io::detail::read_exactly(m_fd, &*input_buffer.begin(), block_start.datasize)) {
                    throw osmium::pbf_error{"unexpected EOF"};
                }
                osmium::io::detail::PBFDataBlobDecoder data_blob_parser{std::move(input_buffer), read_types, read_metadata};

                osmium::memory::Buffer buffer = data_blob_parser();
                if (block_start.first_item_type_or_zero == osmium::item_type::undefined) {
                    auto it = buffer.begin<osmium::OSMObject>();
                    if (it != buffer.end<osmium::OSMObject>()) {
                        block_start.first_item_id_or_zero = it->id();
                        block_start.first_item_type_or_zero = it->type();
                    }
                }
                return buffer;
            }
        };

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_PBF_INPUT_RANDOMACCESS_HPP
