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

/*
 * pmemkv_basic.cpp -- example usage of pmemkv.
 */

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <libpmemkv.hpp>

#define LOG(msg) std::cout << msg << std::endl

using namespace pmem::kv;

int main(int argc, char *argv[])
{
	LOG("Creating config");
	config cfg;

	status ret = cfg.put_string("path", "/dev/dax0");
	assert(ret == status::OK);

	LOG("Starting engine_set");
	db kv;
	status s = kv.open("engine_set", std::move(cfg));
	assert(s == status::OK);

	engine_info info1;
	info1.name = "cmap";

	LOG("Putting engine_1 into engine_set");
	s = kv.put("engine_1", string_view((const char *)&info1, sizeof(info1)));
	assert(s == status::OK);

	engine_info info2;
	info2.name = "stree";

	LOG("Putting engine_2 into engine_set");
	s = kv.put("engine_2", string_view((const char *)&info2, sizeof(info2)));
	assert(s == status::OK);

	LOG("Putting key and value into engine_1");
	s = kv.get("engine_1", [&](string_view value) {
		auto engine_1 = (db *)value.data();
		engine_1->put("key_1", "some_value");
	});
	LOG("Putting engine_2 into engine_set");

	LOG("Stopping engine_set");
	kv.close();

	return 0;
}
