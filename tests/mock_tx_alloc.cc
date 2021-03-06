/*
 * Copyright 2017-2019, Intel Corporation
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

#include <libpmemobj/tx_base.h>
#include <dlfcn.h>
#include <cstdlib>
#include <errno.h>

#include "mock_tx_alloc.h"

thread_local bool tx_alloc_should_fail;

extern "C" PMEMoid pmemobj_tx_alloc(size_t size, uint64_t type_num);

PMEMoid pmemobj_tx_alloc(size_t size, uint64_t type_num) {
    static auto real = (decltype(pmemobj_tx_alloc)*)dlsym(RTLD_NEXT, "pmemobj_tx_alloc");

    if (real == nullptr)
        abort();

    if (tx_alloc_should_fail) {
        errno = ENOMEM;
        return OID_NULL;
    }

    return real(size, type_num);
}

PMEMoid pmemobj_tx_xalloc(size_t size, uint64_t type_num, uint64_t flags) {
    static auto real = (decltype(pmemobj_tx_xalloc)*)dlsym(RTLD_NEXT, "pmemobj_tx_xalloc");

    if (real == nullptr)
        abort();

    if (tx_alloc_should_fail) {
        errno = ENOMEM;
        return OID_NULL;
    }

    return real(size, type_num, flags);
}
