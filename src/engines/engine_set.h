/*
 * Copyright 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "../engine.h"
#include "libpmemkv.h"
#include <libpmemobj++/experimental/string.hpp>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/pool.hpp>
#include <unordered_map>

#define LOG(msg) std::cout << msg << std::endl

namespace pmem
{
namespace kv
{

template <typename EngineData>
class engine_set : public engine_base {
public:
	engine_set(std::unique_ptr<internal::config> &cfg);
	~engine_set();

	status count_all(std::size_t &cnt) final;

	status exists(string_view key) final;

	status get(string_view key, get_v_callback *callback, void *arg) final;

	status put(string_view key, string_view data) final;

	status get(string_view key, get_v_callback *callback, void *arg)
	{
		LOG("get key=" << std::string(key.data(), key.size()));

		if (engines.find(key) == engines.end()) {
			LOG("key not found");

			return status::NOT_FOUND;
		}

		callback(engines.at(key), sizeof(db *), arg);
		return status::OK;
	}

	status put(string_view key, EngineInfo info)
	{
		LOG("put key=" << std::string(key.data(), key.size()));

		config cfg;
		db kv;

		if (engines.find(key) == engines.end()) {

			pmem::obj::transaction::run(pmpool, [&] {
				pmpool.root()->oids = pmem::obj::make_persistent<
					std::vector<std::pair<string_view, PMEMoid>>>();
				pmpool.root()->oids->push_back(OID_NULL);
			}
		}

		status ret = cfg.put_object(
			info.name, &(pmpool.root()->oids)[oids.get().size()], nullptr);
		assert(ret == status::OK);

		status s = kv->open(info.name, std::move(cfg));
		assert(s == status::OK);

		engines[key] = *kv;

		return status::OK;
	}

	status remove(string_view key)
	{
		LOG("remove key=" << std::string(key.data(), key.size()));

		bool erased = engines.erase(key);
		return erased ? status::OK : status::NOT_FOUND;
	}

protected:
	struct Root {
		pmem::obj::persistent_ptr<EngineData> ptr;
		std::vector<std::pair<pmem::obj::experimental::string, PMEMoid>> set;
	};

	pmem::obj::pool_base pmpool;

	std::unordered_map<std::string, db> engines;
};

} /* namespace kv */
} /* namespace pmem */
