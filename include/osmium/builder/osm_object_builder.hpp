#ifndef OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP
#define OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP

/*

This file is part of Osmium (https://osmcode.org/libosmium).

Copyright 2013-2018 Jochen Topf <jochen@topf.org> and others (see README).

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

#include <osmium/builder/builder.hpp>
#include <osmium/memory/item.hpp>
#include <osmium/osm/area.hpp>
#include <osmium/osm/box.hpp>
#include <osmium/osm/changeset.hpp>
#include <osmium/osm/item_type.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/node.hpp>
#include <osmium/osm/node_ref.hpp>
#include <osmium/osm/object.hpp>
#include <osmium/osm/relation.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/osm/timestamp.hpp>
#include <osmium/osm/types.hpp>
#include <osmium/osm/way.hpp>
#include <osmium/util/compatibility.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

namespace osmium {

    namespace memory {
        class Buffer;
    } // namespace memory

    namespace builder {

        class TagListBuilder : public Builder {

        public:

            explicit TagListBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(TagList)) {
                new (&item()) TagList{};
            }

            explicit TagListBuilder(Builder& parent) :
                Builder(parent.buffer(), &parent, sizeof(TagList)) {
                new (&item()) TagList{};
            }

            TagListBuilder(const TagListBuilder&) = delete;
            TagListBuilder& operator=(const TagListBuilder&) = delete;

            TagListBuilder(TagListBuilder&&) = delete;
            TagListBuilder& operator=(TagListBuilder&&) = delete;

            ~TagListBuilder() {
                add_padding();
            }

            /**
             * Add tag to buffer.
             *
             * @param key Tag key (0-terminated string).
             * @param value Tag value (0-terminated string).
             */
            void add_tag(const char* key, const char* value) {
                if (std::strlen(key) > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag key is too long"};
                }
                if (std::strlen(value) > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag value is too long"};
                }
                add_size(append(key));
                add_size(append(value));
            }

            /**
             * Add tag to buffer.
             *
             * @param key Pointer to tag key.
             * @param key_length Length of key (not including the \0 byte).
             * @param value Pointer to tag value.
             * @param value_length Length of value (not including the \0 byte).
             */
            void add_tag(const char* key, const std::size_t key_length, const char* value, const std::size_t value_length) {
                if (key_length > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag key is too long"};
                }
                if (value_length > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag value is too long"};
                }
                add_size(append_with_zero(key,   osmium::memory::item_size_type(key_length)));
                add_size(append_with_zero(value, osmium::memory::item_size_type(value_length)));
            }

            /**
             * Add tag to buffer.
             *
             * @param key Tag key.
             * @param value Tag value.
             */
            void add_tag(const std::string& key, const std::string& value) {
                if (key.size() > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag key is too long"};
                }
                if (value.size() > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM tag value is too long"};
                }
                add_size(append(key.data(),   osmium::memory::item_size_type(key.size())   + 1));
                add_size(append(value.data(), osmium::memory::item_size_type(value.size()) + 1));
            }

            /**
             * Add tag to buffer.
             *
             * @param tag Tag.
             */
            void add_tag(const osmium::Tag& tag) {
                add_size(append(tag.key()));
                add_size(append(tag.value()));
            }

            /**
             * Add tag to buffer.
             *
             * @param tag Pair of key/value 0-terminated strings.
             */
            void add_tag(const std::pair<const char* const, const char* const>& tag) {
                add_tag(tag.first, tag.second);
            }
            void add_tag(const std::pair<const char* const, const char*>& tag) {
                add_tag(tag.first, tag.second);
            }
            void add_tag(const std::pair<const char*, const char* const>& tag) {
                add_tag(tag.first, tag.second);
            }
            void add_tag(const std::pair<const char*, const char*>& tag) {
                add_tag(tag.first, tag.second);
            }

            /**
             * Add tag to buffer.
             *
             * @param tag Pair of std::string references.
             */
            void add_tag(const std::pair<const std::string&, const std::string&>& tag) {
                add_tag(tag.first, tag.second);
            }

        }; // class TagListBuilder

        template <typename T>
        class NodeRefListBuilder : public Builder {

        public:

            explicit NodeRefListBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(T)) {
                new (&item()) T{};
            }

            explicit NodeRefListBuilder(Builder& parent) :
                Builder(parent.buffer(), &parent, sizeof(T)) {
                new (&item()) T{};
            }

            NodeRefListBuilder(const NodeRefListBuilder&) = delete;
            NodeRefListBuilder& operator=(const NodeRefListBuilder&) = delete;

            NodeRefListBuilder(NodeRefListBuilder&&) = delete;
            NodeRefListBuilder& operator=(NodeRefListBuilder&&) = delete;

            ~NodeRefListBuilder() {
                add_padding();
            }

            void add_node_ref(const NodeRef& node_ref) {
                new (reserve_space_for<osmium::NodeRef>()) osmium::NodeRef{node_ref};
                add_size(sizeof(osmium::NodeRef));
            }

            void add_node_ref(const object_id_type ref, const osmium::Location& location = Location{}) {
                add_node_ref(NodeRef{ref, location});
            }

        }; // class NodeRefListBuilder

        using WayNodeListBuilder = NodeRefListBuilder<WayNodeList>;
        using OuterRingBuilder   = NodeRefListBuilder<OuterRing>;
        using InnerRingBuilder   = NodeRefListBuilder<InnerRing>;

        class RelationMemberListBuilder : public Builder {

            /**
             * Add role to buffer.
             *
             * @param member Relation member object where the length of the role
             *               will be set.
             * @param role The role.
             * @param length Length of role (without \0 termination).
             * @throws std:length_error If role is longer than osmium::max_osm_string_length
             */
            void add_role(osmium::RelationMember& member, const char* role, const std::size_t length) {
                if (length > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM relation member role is too long"};
                }
                member.set_role_size(osmium::string_size_type(length) + 1);
                add_size(append_with_zero(role, osmium::memory::item_size_type(length)));
                add_padding(true);
            }

        public:

            explicit RelationMemberListBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(RelationMemberList)) {
                new (&item()) RelationMemberList{};
            }

            explicit RelationMemberListBuilder(Builder& parent) :
                Builder(parent.buffer(), &parent, sizeof(RelationMemberList)) {
                new (&item()) RelationMemberList{};
            }

            RelationMemberListBuilder(const RelationMemberListBuilder&) = delete;
            RelationMemberListBuilder& operator=(const RelationMemberListBuilder&) = delete;

            RelationMemberListBuilder(RelationMemberListBuilder&&) = delete;
            RelationMemberListBuilder& operator=(RelationMemberListBuilder&&) = delete;

            ~RelationMemberListBuilder() {
                add_padding();
            }

            /**
             * Add a member to the relation.
             *
             * @param type The type (node, way, or relation).
             * @param ref The ID of the member.
             * @param role The role of the member.
             * @param role_length Length of the role (without \0 termination).
             * @param full_member Optional pointer to the member object. If it
             *                    is available a copy will be added to the
             *                    relation.
             * @throws std:length_error If role_length is greater than
             *         osmium::max_osm_string_length
             */
            void add_member(osmium::item_type type, object_id_type ref, const char* role, const std::size_t role_length, const osmium::OSMObject* full_member = nullptr) {
                auto* member = reserve_space_for<osmium::RelationMember>();
                new (member) osmium::RelationMember{ref, type, full_member != nullptr};
                add_size(sizeof(RelationMember));
                add_role(*member, role, role_length);
                if (full_member) {
                    add_item(*full_member);
                }
            }

            /**
             * Add a member to the relation.
             *
             * @param type The type (node, way, or relation).
             * @param ref The ID of the member.
             * @param role The role of the member (\0 terminated string).
             * @param full_member Optional pointer to the member object. If it
             *                    is available a copy will be added to the
             *                    relation.
             * @throws std:length_error If role is longer than osmium::max_osm_string_length
             */
            void add_member(osmium::item_type type, object_id_type ref, const char* role, const osmium::OSMObject* full_member = nullptr) {
                add_member(type, ref, role, std::strlen(role), full_member);
            }

            /**
             * Add a member to the relation.
             *
             * @param type The type (node, way, or relation).
             * @param ref The ID of the member.
             * @param role The role of the member.
             * @param full_member Optional pointer to the member object. If it
             *                    is available a copy will be added to the
             *                    relation.
             * @throws std:length_error If role is longer than osmium::max_osm_string_length
             */
            void add_member(osmium::item_type type, object_id_type ref, const std::string& role, const osmium::OSMObject* full_member = nullptr) {
                add_member(type, ref, role.data(), role.size(), full_member);
            }

        }; // class RelationMemberListBuilder

        class ChangesetDiscussionBuilder : public Builder {

            osmium::ChangesetComment* m_comment = nullptr;

            void add_user(osmium::ChangesetComment& comment, const char* user, const std::size_t length) {
                if (length > osmium::max_osm_string_length) {
                    throw std::length_error{"OSM user name is too long"};
                }
                comment.set_user_size(osmium::string_size_type(length) + 1);
                add_size(append_with_zero(user, osmium::memory::item_size_type(length)));
            }

            void add_text(osmium::ChangesetComment& comment, const char* text, const std::size_t length) {
                if (length > std::numeric_limits<osmium::changeset_comment_size_type>::max() - 1) {
                    throw std::length_error{"OSM changeset comment is too long"};
                }
                comment.set_text_size(osmium::changeset_comment_size_type(length) + 1);
                add_size(append_with_zero(text, osmium::memory::item_size_type(length)));
                add_padding(true);
            }

        public:

            explicit ChangesetDiscussionBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(ChangesetDiscussion)) {
                new (&item()) ChangesetDiscussion{};
            }

            explicit ChangesetDiscussionBuilder(Builder& parent) :
                Builder(parent.buffer(), &parent, sizeof(ChangesetDiscussion)) {
                new (&item()) ChangesetDiscussion{};
            }

            ChangesetDiscussionBuilder(const ChangesetDiscussionBuilder&) = delete;
            ChangesetDiscussionBuilder& operator=(const ChangesetDiscussionBuilder&) = delete;

            ChangesetDiscussionBuilder(ChangesetDiscussionBuilder&&) = delete;
            ChangesetDiscussionBuilder& operator=(ChangesetDiscussionBuilder&&) = delete;

            ~ChangesetDiscussionBuilder() {
                assert(!m_comment && "You have to always call both add_comment() and then add_comment_text() in that order for each comment!");
                add_padding();
            }

            void add_comment(osmium::Timestamp date, osmium::user_id_type uid, const char* user) {
                assert(!m_comment && "You have to always call both add_comment() and then add_comment_text() in that order for each comment!");
                m_comment = reserve_space_for<osmium::ChangesetComment>();
                new (m_comment) osmium::ChangesetComment{date, uid};
                add_size(sizeof(ChangesetComment));
                add_user(*m_comment, user, std::strlen(user));
            }

            void add_comment_text(const char* text) {
                assert(m_comment && "You have to always call both add_comment() and then add_comment_text() in that order for each comment!");
                osmium::ChangesetComment& comment = *m_comment;
                m_comment = nullptr;
                add_text(comment, text, std::strlen(text));
            }

            void add_comment_text(const std::string& text) {
                assert(m_comment && "You have to always call both add_comment() and then add_comment_text() in that order for each comment!");
                osmium::ChangesetComment& comment = *m_comment;
                m_comment = nullptr;
                add_text(comment, text.c_str(), text.size());
            }

        }; // class ChangesetDiscussionBuilder

#define OSMIUM_FORWARD(setter) \
    template <typename... TArgs> \
    type& setter(TArgs&&... args) { \
        object().setter(std::forward<TArgs>(args)...); \
        return static_cast<type&>(*this); \
    }

        template <typename TDerived, typename T>
        class OSMObjectBuilder : public Builder {

            using type = TDerived;

            enum {
                min_size_for_user = osmium::memory::padded_length(sizeof(string_size_type) + 1)
            };

        public:

            explicit OSMObjectBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(T) + min_size_for_user) {
                new (&item()) T{};
                add_size(min_size_for_user);
                std::fill_n(object().data() + sizeof(T), min_size_for_user, 0);
                object().set_user_size(1);
            }

            /**
             * Get a reference to the object buing built.
             *
             * Note that this reference will be invalidated by every action
             * on the builder that might make the buffer grow. This includes
             * calls to set_user() and any time a new sub-builder is created.
             */
            T& object() noexcept {
                return static_cast<T&>(item());
            }

            /**
             * Get a const reference to the object buing built.
             *
             * Note that this reference will be invalidated by every action
             * on the builder that might make the buffer grow. This includes
             * calls to set_user() and any time a new sub-builder is created.
             */
            const T& cobject() const noexcept {
                return static_cast<const T&>(item());
            }

            /**
             * Set user name.
             *
             * @param user Pointer to user name.
             * @param length Length of user name (without \0 termination).
             */
            TDerived& set_user(const char* user, const string_size_type length) {
                const auto size_of_object = sizeof(T) + sizeof(string_size_type);
                assert(cobject().user_size() == 1 && (size() <= size_of_object + osmium::memory::padded_length(1))
                       && "set_user() must be called at most once and before any sub-builders");
                const auto available_space = min_size_for_user - sizeof(string_size_type) - 1;
                if (length > available_space) {
                    const auto space_needed = osmium::memory::padded_length(length - available_space);
                    std::fill_n(reserve_space(space_needed), space_needed, 0);
                    add_size(static_cast<uint32_t>(space_needed));
                }
                std::copy_n(user, length, object().data() + size_of_object);
                object().set_user_size(length + 1);

                return static_cast<TDerived&>(*this);
            }

            /**
             * Set user name.
             *
             * @param user Pointer to \0-terminated user name.
             *
             * @pre @code strlen(user) < 2^16 - 1 @endcode
             */
            TDerived& set_user(const char* user) {
                const auto len = std::strlen(user);
                assert(len < std::numeric_limits<string_size_type>::max());
                return set_user(user, static_cast<string_size_type>(len));
            }

            /**
             * Set user name.
             *
             * @param user User name.
             *
             * @pre @code user.size() < 2^16 - 1 @endcode
             */
            TDerived& set_user(const std::string& user) {
                assert(user.size() < std::numeric_limits<string_size_type>::max());
                return set_user(user.data(), static_cast<string_size_type>(user.size()));
            }

            /// @deprecated Use set_user(...) instead.
            template <typename... TArgs>
            OSMIUM_DEPRECATED void add_user(TArgs&&... args) {
                set_user(std::forward<TArgs>(args)...);
            }

            OSMIUM_FORWARD(set_id)
            OSMIUM_FORWARD(set_visible)
            OSMIUM_FORWARD(set_deleted)
            OSMIUM_FORWARD(set_version)
            OSMIUM_FORWARD(set_changeset)
            OSMIUM_FORWARD(set_uid)
            OSMIUM_FORWARD(set_uid_from_signed)
            OSMIUM_FORWARD(set_timestamp)
            OSMIUM_FORWARD(set_attribute)
            OSMIUM_FORWARD(set_removed)

            void add_tags(const std::initializer_list<std::pair<const char*, const char*>>& tags) {
                osmium::builder::TagListBuilder tl_builder{buffer(), this};
                for (const auto& p : tags) {
                    tl_builder.add_tag(p.first, p.second);
                }
            }

        }; // class OSMObjectBuilder

        class NodeBuilder : public OSMObjectBuilder<NodeBuilder, Node> {

            using type = NodeBuilder;

        public:

            explicit NodeBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                OSMObjectBuilder<NodeBuilder, Node>(buffer, parent) {
            }

            explicit NodeBuilder(Builder& parent) :
                OSMObjectBuilder<NodeBuilder, Node>(parent.buffer(), &parent) {
            }

            OSMIUM_FORWARD(set_location)

        }; // class NodeBuilder

        class WayBuilder : public OSMObjectBuilder<WayBuilder, Way> {

            using type = WayBuilder;

        public:

            explicit WayBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                OSMObjectBuilder<WayBuilder, Way>(buffer, parent) {
            }

            explicit WayBuilder(Builder& parent) :
                OSMObjectBuilder<WayBuilder, Way>(parent.buffer(), &parent) {
            }

            void add_node_refs(const std::initializer_list<osmium::NodeRef>& nodes) {
                osmium::builder::WayNodeListBuilder builder{buffer(), this};
                for (const auto& node_ref : nodes) {
                    builder.add_node_ref(node_ref);
                }
            }

        }; // class WayBuilder

        class RelationBuilder : public OSMObjectBuilder<RelationBuilder, Relation> {

            using type = RelationBuilder;

        public:

            explicit RelationBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                OSMObjectBuilder<RelationBuilder, Relation>(buffer, parent) {
            }

            explicit RelationBuilder(Builder& parent) :
                OSMObjectBuilder<RelationBuilder, Relation>(parent.buffer(), &parent) {
            }

        }; // class RelationBuilder

        class AreaBuilder : public OSMObjectBuilder<AreaBuilder, Area> {

            using type = AreaBuilder;

        public:

            explicit AreaBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                OSMObjectBuilder<AreaBuilder, Area>(buffer, parent) {
            }

            explicit AreaBuilder(Builder& parent) :
                OSMObjectBuilder<AreaBuilder, Area>(parent.buffer(), &parent) {
            }

            /**
             * Initialize area attributes from the attributes of the given object.
             */
            void initialize_from_object(const osmium::OSMObject& source) {
                set_id(osmium::object_id_to_area_id(source.id(), source.type()));
                set_version(source.version());
                set_changeset(source.changeset());
                set_timestamp(source.timestamp());
                set_visible(source.visible());
                set_uid(source.uid());
                set_user(source.user());
            }

        }; // class AreaBuilder

        class ChangesetBuilder : public Builder {

            using type = ChangesetBuilder;

            enum {
                min_size_for_user = osmium::memory::padded_length(1)
            };

        public:

            explicit ChangesetBuilder(osmium::memory::Buffer& buffer, Builder* parent = nullptr) :
                Builder(buffer, parent, sizeof(Changeset) + min_size_for_user) {
                new (&item()) Changeset{};
                add_size(min_size_for_user);
                std::fill_n(object().data() + sizeof(Changeset), min_size_for_user, 0);
                object().set_user_size(1);
            }

            /**
             * Get a reference to the changeset buing built.
             *
             * Note that this reference will be invalidated by every action
             * on the builder that might make the buffer grow. This includes
             * calls to set_user() and any time a new sub-builder is created.
             */
            Changeset& object() noexcept {
                return static_cast<Changeset&>(item());
            }

            /**
             * Get a const reference to the changeset buing built.
             *
             * Note that this reference will be invalidated by every action
             * on the builder that might make the buffer grow. This includes
             * calls to set_user() and any time a new sub-builder is created.
             */
            const Changeset& cobject() const noexcept {
                return static_cast<const Changeset&>(item());
            }

            OSMIUM_FORWARD(set_id)
            OSMIUM_FORWARD(set_uid)
            OSMIUM_FORWARD(set_uid_from_signed)
            OSMIUM_FORWARD(set_created_at)
            OSMIUM_FORWARD(set_closed_at)
            OSMIUM_FORWARD(set_num_changes)
            OSMIUM_FORWARD(set_num_comments)
            OSMIUM_FORWARD(set_attribute)
            OSMIUM_FORWARD(set_removed)

            // @deprecated Use set_bounds() instead.
            OSMIUM_DEPRECATED osmium::Box& bounds() noexcept {
                return object().bounds();
            }

            ChangesetBuilder& set_bounds(const osmium::Box& box) noexcept {
                object().bounds() = box;
                return *this;
            }

            /**
             * Set user name.
             *
             * @param user Pointer to user name.
             * @param length Length of user name (without \0 termination).
             */
            ChangesetBuilder& set_user(const char* user, const string_size_type length) {
                assert(cobject().user_size() == 1 && (size() <= sizeof(Changeset) + osmium::memory::padded_length(1))
                       && "set_user() must be called at most once and before any sub-builders");
                const auto available_space = min_size_for_user - 1;
                if (length > available_space) {
                    const auto space_needed = osmium::memory::padded_length(length - available_space);
                    std::fill_n(reserve_space(space_needed), space_needed, 0);
                    add_size(static_cast<uint32_t>(space_needed));
                }
                std::copy_n(user, length, object().data() + sizeof(Changeset));
                object().set_user_size(length + 1);

                return *this;
            }

            /**
             * Set user name.
             *
             * @param user Pointer to \0-terminated user name.
             *
             * @pre @code strlen(user) < 2^16 - 1 @endcode
             */
            ChangesetBuilder& set_user(const char* user) {
                const auto len = std::strlen(user);
                assert(len <= std::numeric_limits<string_size_type>::max());
                return set_user(user, static_cast<string_size_type>(len));
            }

            /**
             * Set user name.
             *
             * @param user User name.
             *
             * @pre @code user.size() < 2^16 - 1 @endcode
             */
            ChangesetBuilder& set_user(const std::string& user) {
                assert(user.size() < std::numeric_limits<string_size_type>::max());
                return set_user(user.data(), static_cast<string_size_type>(user.size()));
            }

            /// @deprecated Use set_user(...) instead.
            template <typename... TArgs>
            OSMIUM_DEPRECATED void add_user(TArgs&&... args) {
                set_user(std::forward<TArgs>(args)...);
            }

        }; // class ChangesetBuilder

#undef OSMIUM_FORWARD

    } // namespace builder

} // namespace osmium

#endif // OSMIUM_BUILDER_OSM_OBJECT_BUILDER_HPP
