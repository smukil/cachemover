/**
 *
 *  Copyright 2021 Netflix, Inc.
 *
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use this file except in compliance with the License.
 *     You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 *
 */

#pragma once

#include "utils/status.h"

#include <netinet/in.h>
#include <netdb.h>

#include <cstdint>
#include <string>

namespace memcachedumper {

class Sockaddr {
 public:
  Sockaddr();

  Sockaddr& operator=(const struct sockaddr_in &addr);

  const struct sockaddr_in& raw_struct_ref() const {
    return sockaddr_;
  }

  // Populate our sockaddr with the right resolved hostname and port.
  Status ResolveAndPopulateSockaddr(const std::string& hostname, int port);

 private:
  struct sockaddr_in sockaddr_;
};

} // namespace memcachedumper
