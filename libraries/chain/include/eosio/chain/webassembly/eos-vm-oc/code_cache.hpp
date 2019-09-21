#pragma once

#include <eosio/chain/webassembly/eos-vm-oc/eos-vm-oc.hpp>
#include <eosio/chain/webassembly/eos-vm-oc/ipc_helpers.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/key_extractors.hpp>

#include <boost/interprocess/segment_manager.hpp> //XXX no we just want the rbtree thing
#include <boost/asio/local/datagram_protocol.hpp>

namespace eosio { namespace chain { namespace eosvmoc {

using namespace boost::multi_index;
using namespace boost::asio;

namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;

using allocator_t = bip::rbtree_best_fit<bip::null_mutex_family, bip::offset_ptr<void>, 16>;

struct config;

class code_cache {
   public:
      code_cache(const bfs::path data_dir, const eosvmoc::config& eosvmoc_config, const chainbase::database& db);
      ~code_cache();

      const int& fd() const { return _cache_fd; }

      //If code is in cache: returns pointer & bumps to front of MRU list
      //If code is not in cache, and not blacklisted, and not currently compiling: return nullptr and kick off compile
      //otherwise: return nullptr
      const code_descriptor* const get_descriptor_for_code(const digest_type& code_id, const uint8_t& vm_version);

      void free_code(const digest_type& code_id, const uint8_t& vm_version);

   private:
      struct by_hash;

      typedef boost::multi_index_container<
         code_descriptor,
         indexed_by<
            sequenced<>,
            ordered_unique<tag<by_hash>,
               composite_key< code_descriptor,
                  member<code_descriptor, digest_type, &code_descriptor::code_hash>,
                  member<code_descriptor, uint8_t,     &code_descriptor::vm_version>
               >
            >
         >
      > code_cache_index;
      code_cache_index _cache_index;

      const chainbase::database& _db;

      bfs::path _cache_file_path;
      int _cache_fd;

      io_context _ctx;
      local::datagram_protocol::socket _compile_monitor_write_socket{_ctx};
      local::datagram_protocol::socket _compile_monitor_read_socket{_ctx};

      size_t _free_bytes_eviction_threshold;
      void check_eviction_threshold(size_t free_bytes);

      void set_on_disk_region_dirty(bool);

      template <typename T>
      void serialize_cache_index(fc::datastream<T>& ds);
};

}}}