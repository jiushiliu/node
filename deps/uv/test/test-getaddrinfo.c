/* Copyright Joyent, Inc. and other Node contributors. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "uv.h"
#include "task.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* strlen */


#define CONCURRENT_COUNT    10

static const char* name = "localhost";

static uv_getaddrinfo_t getaddrinfo_handle;
static int getaddrinfo_cbs = 0;

/* data used for running multiple calls concurrently */
static uv_getaddrinfo_t getaddrinfo_handles[CONCURRENT_COUNT];
static int callback_counts[CONCURRENT_COUNT];


static void getaddrinfo_basic_cb(uv_getaddrinfo_t* handle,
                                 int status,
                                 struct addrinfo* res) {
  ASSERT(handle == &getaddrinfo_handle);
  getaddrinfo_cbs++;
}


static void getaddrinfo_cuncurrent_cb(uv_getaddrinfo_t* handle,
                                      int status,
                                      struct addrinfo* res) {
  int i;
  int* data = (int*)handle->data;

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    if (&getaddrinfo_handles[i] == handle) {
      ASSERT(i == *data);

      callback_counts[i]++;
      break;
    }
  }
  ASSERT (i < CONCURRENT_COUNT);

  free(data);

  getaddrinfo_cbs++;
}


TEST_IMPL(getaddrinfo_basic) {
  int r;

  r = uv_getaddrinfo(uv_default_loop(),
                     &getaddrinfo_handle,
                     &getaddrinfo_basic_cb,
                     name,
                     NULL,
                     NULL);
  ASSERT(r == 0);

  uv_run(uv_default_loop());

  ASSERT(getaddrinfo_cbs == 1);

  return 0;
}


TEST_IMPL(getaddrinfo_concurrent) {
  int i, r;
  int* data;

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    callback_counts[i] = 0;

    data = (int*)malloc(sizeof(int));
    *data = i;
    getaddrinfo_handles[i].data = data;

    r = uv_getaddrinfo(uv_default_loop(),
                       &getaddrinfo_handles[i],
                       &getaddrinfo_cuncurrent_cb,
                       name,
                       NULL,
                       NULL);
    ASSERT(r == 0);
  }

  uv_run(uv_default_loop());

  for (i = 0; i < CONCURRENT_COUNT; i++) {
    ASSERT(callback_counts[i] == 1);
  }

  return 0;
}
