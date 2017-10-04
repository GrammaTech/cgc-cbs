# CGC Challenge Binaries

This repository holds challenge binaries (CBs) and tools from DARPA's
Cyber Grand Challenge (CGC).  Specifically, this is a fork of the
[CGC samples](https://github.com/CyberGrandChallenge/samples/)
repository on GitHub, into which the following CGC tools have been
incorporated: libcgc, cgc2elf, service-launcher, poll-generator,
cb-testing. 

Portions of Trail of Bits' [libcgc implmentation](https://github.com/trailofbits/cb-multios/tree/master/include) have also been incorporated,
along with GrammaTech modifications to provide compatibility across ISAs
(we have successfully built CBs for x86-32, x86-64, and ARMv7).

## Layout

The repository is laid out as follows:

| Path              | Contents                                                        |
|-------------------|-----------------------------------------------------------------|
| `bin/`            | Utility scripts useful for building and running tests           |
| `cqe-challenges/` | CBs from the CGC qualifying event                               |
| `debian/`         | Debian packaging folder (unchanged from DARPA)                  |
| `examples/`       | CB samples and CFE challenges                                   |
| `lib/`            | libcgc implementation                                           |
| `Makefile`        | DECREE build and test (unchanged from DARPA)                    |
| `templates/`      | Templates for writing CBs (unchanged from DARPA)                |
| `tools/`          | Collected CGC tools developed by DARPA.                         |

## Setup

The test tools require that some Python packages be installed:

        sudo pip install pycrypto pyyaml matplotlib defusedxml

And you must build the service launcher (used by `cb-test` to feed inputs into CBs):

        cd tools/service-launcher
        make

## Building and Testing Challenge Binaries

- Set `PATH` and `PYTHONPATH` environment variables to point to scripts needed 
  to build and run pollers:

        . sourceme.sh

- In a challenge directory, run `build.sh <output name> <compiler and flags>`. 
  For example, to build a 64-bit binary using gcc with optimization on a 64-bit machine:

        cd cqe-challenges/CROMU_00003
        build.sh CROMU_00003 gcc -O3

- Each CB is accompanied by a `README.md` file that describes the program, how
  to interact with it, vulnerabilities embedded in the CB, and potential 
  difficulties in analyzing it.

- CGC challenge binaries can be tested for correctness with input/output pairs
  called "pollers". Generate these poller inputs by using the `generate-polls`
  tool. Each challenge is accompanied by a state machine and state graph
  that the tool uses to generate poller inputs. Often there are two sets of
  these specifications, in `poller/for-testing` and `poller/for-release`. The
  differences aren't clear and we tend to use the `for-release` inputs.

        cd poller/for-release
        generate-polls machine.py state-graph.yaml .

- Use the `cb-test` tool to feed test input into the challenge binary and check
  its output. One can use the `--xml` option to specify one or more xml files
  or the `--xml_dir` option to point to an entire directory of pollers (i.e., 
  point to a `for-release` or `for-testing` directory). See documentation in
  `tools/cb-testing` for more details. Examples follow:

        # from inside cgc-cbs/cqe-challenges/CROMU_00003:
        # directory of pollers
        cb-test --directory . --cb CROMU_00003 --xml_dir poller/for-release

        # single poller
        cb-test --directory . --cb CROMU_00003 --xml poller/for-release/GEN_00000_00489.xml

## Imported subtrees

The following subdirectories of this repository were added using git
subtree and may be updated using the following commands.

- tools/cb-testing

        git subtree pull --prefix=tools/cb-testing https://github.com/CyberGrandChallenge/cb-testing.git master

- tools/cgc2elf

        git subtree pull --prefix=tools/cgc2elf https://github.com/CyberGrandChallenge/cgc2elf.git master

- tools/poll-generator

        git subtree pull --prefix=tools/poll-generator https://github.com/CyberGrandChallenge/poll-generator.git master

- tools/service-launcher

        git subtree pull --prefix=tools/service-launcher https://github.com/CyberGrandChallenge/service-launcher.git master

- lib/libcgc

        git subtree pull --prefix=lib/libcgc https://github.com/CyberGrandChallenge/libcgc.git master
