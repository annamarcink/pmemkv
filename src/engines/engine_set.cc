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

#include "engine_set.h"

#include <iostream>
#include <unistd.h>

namespace pmem
{
namespace kv
{

template <class EngineData>
engine_set<EngineData>::engine_set(std::unique_ptr<internal::config> &cfg)
{
	const char *path = nullptr;
	std::size_t size;
	auto is_path = cfg->get_string("path", &path);

	if (is_path) {
		uint64_t force_create;

		if (!cfg->get_uint64("force_create", &force_create)) {
			force_create = 0;
		}

		pmem::obj::pool<Root> pop;
		if (force_create) {
			if (!cfg->get_uint64("size", &size))
				throw internal::invalid_argument(
					"Config does not contain item with key: \"size\"");

			pop = pmem::obj::pool<Root>::create(path, LAYOUT, size, S_IRWXU);
		} else {
			pop = pmem::obj::pool<Root>::open(path, LAYOUT);
		}

		// pop.root()->set =
		// pmem::obj::make_persistent<std::pair<pmem::obj::experimental::string,
		// PMEMoid>>(); // ??
		pmpool = pop;
	}
}

template <class EngineData>
engine_set<EngineData>::~engine_set()
{
	pmpool.close();

	LOG("Stopped ok");
}

template <class EngineData>
status engine_set<EngineData>::count_all(std::size_t &cnt)
{
	LOG("count_all");

	cnt = engines.size();

	return status::OK;
}

template <class EngineData>
status engine_set<EngineData>::exists(string_view key)
{
	std::string key_string = std::string(key.data(), key.size());
	LOG("exists for key=" << key_string);

	if (engines.find(key_string) == engines.end())
		return status::NOT_FOUND;
	else
		return status::OK;
}

template <class EngineData>
status engine_set<EngineData>::get(string_view key, get_v_callback *callback, void *arg)
{
	std::string key_string = std::string(key.data(), key.size());
	LOG("get key=" << key_string);

	if (engines.find(key_string) == engines.end()) {

		auto el = std::find_if(
			this->set.begin(), this->set.end(),
			[&key_string](const std::pair<pmem::obj::experimental::string,
						      PMEMoid> &element) {
				return element.first == key_string;
			});

		if (el == this->set.end()) {
			LOG("key not found");
			return status::NOT_FOUND;
		} else {
			config cfg;
			db kv;

			status ret = cfg.put_object(key_string, &el.second, nullptr);
			assert(ret == status::OK);

			status s = kv.open(el.first, std::move(cfg));
			assert(s == status::OK);

			engines.insert(std::make_pair(key_string, kv));
		}
	}

	callback(engines.at(key_string), sizeof(db), arg);
	return status::OK;
}

template <class EngineData>
status engine_set<EngineData>::put(string_view key, string_view data)
{
	std::string key_string = std::string(key.data(), key.size());
	LOG("put key=" << key_string);

	config cfg;
	db kv;

	pmem::obj::pool<Root> pop = pmpool;

	if (engines.find(key_string) == engines.end()) {
		pmem::obj::transaction::run(pop, [&] {
			pop.root()->set = pmem::obj::make_persistent<std::vector<
				std::pair<pmem::obj::experimental::string, PMEMoid>>>();
			pop.root()->set->push_back(OID_NULL);

			// pmpool = pop;
		});
	}

	status ret = cfg.put_object(
		key_string, &(pop.root()->set)[this->set.get().size() - 1], nullptr);
	pmpool = pop;
	assert(ret == status::OK);

	status s = kv.open(key_string, std::move(cfg));
	assert(s == status::OK);

	engines.insert(std::make_pair(key_string, kv));

	return status::OK;
}

template <class EngineData>
status engine_set<EngineData>::remove(string_view key)
{
	std::string key_string = std::string(key.data(), key.size());
	LOG("remove key=" << key_string);

	bool erased = engines.erase(key_string);
	return erased ? status::OK : status::NOT_FOUND;
}

} /* namespace kv */
} /* namespace pmem */
