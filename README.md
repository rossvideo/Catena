# ca·te·na

> /kəˈtēnə/
> a connected series of related things

Catena is a standardization of communication methods between (micro)services and full products – designed for hybrid cloud and on-premises solutions with the goal of making it easy to secure, connect and control a multi-vendor ecosystem of media processing services and microservices no matter where they are.

The Catena specification is currently in discussion with OSA with the goal of SMPTE standardization.

Catena is based on lessons learned and experience with Ross Video’s openGear platform, which is:

* A successful (Emmy-winning) initiative
* In existence since 2006
* An open de-facto standard to the broadcast industry
* Used by over 100 partner companies in the openGear and DashBoard ecosystem

Catena SDK is an open-source project led by Ross Video.

For more on Ross Video’s open-source projects, please see [https://github.com/rossvideo](https://github.com/rossvideo).

Learn more about Ross Video [here](https://www.rossvideo.com/company/about-ross/).

## Getting Started

1. Install [nodejs](https://nodejs.org/en)
2. Run `npm install`
3. Install [clang-format](https://llvm.org/builds/)

### Building SDK and Examples

#### CPP

* Follow the steps in [here](./sdks/cpp/docs/cpp_sdk_main_page.md).

#### Java

* todo

### Breakdown

* `/docs/`: Documentation and images
* `/example_device_models/`: Example json device models 
  * `/import1/`: Device model with external param imports
* `/interface/`: Contains the Catena protobuf source
* `./schema/catena.device_schema.json`: JSON schema for Catena device models
* `/scripts/`: Collection of useful scripts
  * `./validate.js`: Tool to validate device models using schema
* `/sdks/`: SDK source
  * `/sdks/cpp`: C++ implementation
  * `/sdk/java`: Java implementation
* `./package.json`: NPM package with scripts

## Contribution Guide

This project uses the [GitFlow Workflow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) for branching and tagging.

Merge requests should be made TBD.

Run `npm run clang-format` or `npm run clang-format-windows` in root of project before pushing code to MR

Otherwise: work in progress

## Discussions

Work in progress

## License

Catena is made available under the BSD 3-Clause License. See [LICENSE](LICENSE) for details.
<details>
<summary>License Details</summary>

Copyright 2025 Ross Video Ltd

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
</details>


For more information about the BSD 3-Clause License, see [Open Source Initiative](https://opensource.org/licenses/BSD-3-Clause).
